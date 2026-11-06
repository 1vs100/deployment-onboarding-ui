#include "core/app.h"

#include <thread>
#include <utility>

namespace onboarding {

App::App(std::filesystem::path root, Header header, Platform platform)
    : root_(std::move(root)),
      header_(std::move(header)),
      platform_(std::move(platform)),
      catalog_(Manifest::Embedded()),
      store_(root_, catalog_) {}

void App::CheckNow() {
  store_.CheckAll();
  check_done_ = true;
}

bool App::AllReady() const { return catalog_.Take().all_ready(); }

void App::Start() {
  auto worker = [this](std::function<void()> work) {
    if (platform_.run_worker) {
      platform_.run_worker(std::move(work));
    } else {
      std::thread(std::move(work)).detach();
    }
  };

  worker([this] {
    store_.CheckAll();
    check_done_ = true;
  });

  if (platform_.make_upload_server) {
    server_ = platform_.make_upload_server(store_, catalog_);
  }
  worker([this] {
    std::string error;
    bool ok = server_ && server_->Start(kHttpPort, &error);
    std::vector<std::string> urls;
    if (ok && platform_.lan_urls) urls = platform_.lan_urls();
    for (auto& url : urls) url += ":" + std::to_string(kHttpPort) + "/";
    std::lock_guard lock(urls_mutex_);
    urls_ = std::move(urls);
    server_error_ = ok ? "" : "Upload server failed to start: " + error;
  });
}

void App::Advance() {
  const Snapshot snapshot = catalog_.Take();
  switch (screen_) {
    case ui::Screen::kChecking:
      if (check_done_) screen_ = snapshot.all_ready() ? ui::Screen::kReady : ui::Screen::kGetFiles;
      break;
    case ui::Screen::kGetFiles:
      if (snapshot.all_ready() && !snapshot.busy() && !(platform_.transfer_active && platform_.transfer_active())) {
        screen_ = ui::Screen::kReady;
      }
      break;
    case ui::Screen::kReady:
    case ui::Screen::kDone:
      break;
  }
}

ui::Page App::MakePage(int focused) const {
  ui::Page page;
  page.screen = screen_;
  page.snapshot = catalog_.Take();
  page.header = header_;
  page.pickers_available = platform_.pickers_available ? platform_.pickers_available() : true;
  page.importing = platform_.transfer_active ? platform_.transfer_active() : false;
  page.focused = focused;
  {
    std::lock_guard lock(urls_mutex_);
    page.urls = urls_;
    page.server_error = server_error_;
  }
  return page;
}

ui::Scene App::BuildScene(int focused) { return ui::BuildScene(MakePage(focused)); }

void App::Activate(int button) {
  switch (screen_) {
    case ui::Screen::kGetFiles:
      if (button == 0 && platform_.pick_folder) platform_.pick_folder(store_, catalog_);
      if (button == 1 && platform_.pick_files) platform_.pick_files(store_, catalog_);
      break;
    case ui::Screen::kReady:
      if (button == 0) screen_ = ui::Screen::kDone;
      break;
    case ui::Screen::kChecking:
    case ui::Screen::kDone:
      break;
  }
}

}  // namespace onboarding
