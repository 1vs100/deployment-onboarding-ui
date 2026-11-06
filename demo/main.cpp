// Renders the pages against a fixed set of file states and writes them as
// PNGs. This is how the screenshots in docs/screenshots are made, and how a
// layout change is reviewed.
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "core/catalog.h"
#include "core/manifest.h"
#include "ui/layout.h"
#include "ui/scene.h"
#include "ui/screen.h"
#include "ui/view.h"

namespace {

using namespace onboarding;

constexpr const char* kFlavor = ONBOARDING_UI_FLAVOR_NAME;

Header MakeHeader() { return Header{"1 vs 100", "2.0.1149.0", "0.1.151.0"}; }

std::vector<std::string> MakeUrls() { return {"http://192.168.1.42:8080/"}; }

void SetAll(Catalog& catalog, FileStatus status) {
  for (FileIndex i = 0; i < catalog.manifest().entries().size(); ++i) {
    catalog.SetStatus(i, status);
  }
}

void PrintUsage(const char* program) {
  std::printf("usage: %s --capture <dir>\n", program);
  std::printf("  flavor: %s\n", kFlavor);
}

}  // namespace

int main(int argc, char** argv) {
  std::filesystem::path output;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--capture" && i + 1 < argc) {
      output = argv[++i];
    } else {
      PrintUsage(argv[0]);
      return 2;
    }
  }
  if (output.empty()) {
    PrintUsage(argv[0]);
    return 2;
  }

  ui::Session session;
  std::string error;
  if (!ui::OpenSession({"1 vs 100 Setup"}, &session, &error)) {
    std::fprintf(stderr, "cannot open a session: %s\n", error.c_str());
    return 1;
  }

  Catalog catalog(Manifest::Embedded());
  const auto& entries = catalog.manifest().entries();
  const FileIndex kInvalid = 2;
  const FileIndex kMissing = 9;
  const FileIndex kTransfer = 9;
  const FileIndex kVerifying = 5;

  auto shoot = [&](const std::string& name, ui::Screen screen, int focused, bool pickers) {
    ui::Page page;
    page.screen = screen;
    page.header = MakeHeader();
    page.urls = MakeUrls();
    page.pickers_available = pickers;
    page.focused = focused;
    page.snapshot = catalog.Take();
    const std::filesystem::path path = output / kFlavor / (name + ".png");
    std::filesystem::create_directories(path.parent_path());
    if (!ui::CaptureFrame(session, ui::BuildScene(page), path, &error)) {
      std::fprintf(stderr, "capture %s failed: %s\n", path.string().c_str(), error.c_str());
      return false;
    }
    std::printf("%s %s\n", kFlavor, path.c_str());
    return true;
  };

  bool ok = true;

  // First run: every file is being hashed, one at a time.
  SetAll(catalog, FileStatus::kUnchecked);
  for (FileIndex i = 0; i < 4; ++i) catalog.SetStatus(i, FileStatus::kOk);
  catalog.SetProgress(kVerifying, FileStatus::kVerifying, entries[kVerifying].size / 3);
  catalog.SetChecking(true);
  ok = ok && shoot("checking", ui::Screen::kChecking, -1, true);

  // Missing and invalid files, nothing running.
  catalog.SetChecking(false);
  SetAll(catalog, FileStatus::kOk);
  catalog.SetStatus(kInvalid, FileStatus::kInvalid);
  catalog.SetStatus(kMissing, FileStatus::kMissing);
  ok = ok && shoot("get-files", ui::Screen::kGetFiles, 0, true);

  // The same page while a file is arriving from the phone.
  catalog.SetProgress(kTransfer, FileStatus::kTransferring, entries[kTransfer].size * 42 / 100);
  ok = ok && shoot("get-files-uploading", ui::Screen::kGetFiles, 0, true);

  // Everything verified.
  SetAll(catalog, FileStatus::kOk);
  ok = ok && shoot("ready", ui::Screen::kReady, 0, true);
  ok = ok && shoot("done", ui::Screen::kDone, -1, true);

  ui::CloseSession(&session);
  return ok ? 0 : 1;
}
