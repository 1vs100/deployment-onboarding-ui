// The onboarding flow: which page is showing, what the pages say, and which
// button does what. Owns the manifest, the per-file state and the store, so a
// host can serve the phone surface from the same objects.
#pragma once

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "core/catalog.h"
#include "core/file_store.h"
#include "onboarding/gate.h"
#include "onboarding/platform.h"
#include "ui/layout.h"
#include "ui/scene.h"
#include "ui/screen.h"

namespace onboarding {

constexpr uint16_t kHttpPort = 8080;

class App {
 public:
  App(std::filesystem::path root, Header header, Platform platform);

  // Runs the file check synchronously on the calling thread, so an embedder
  // can skip the view entirely when everything is already in place.
  void CheckNow();
  bool AllReady() const;
  bool done() const { return screen_ == ui::Screen::kDone; }

  // Starts the check and the upload server on worker threads.
  void Start();
  // Moves to the next page when the snapshot allows it.
  void Advance();
  ui::Scene BuildScene(int focused);
  // Runs the action of button `button` on the current page.
  void Activate(int button);

 private:
  ui::Page MakePage(int focused) const;

  std::filesystem::path root_;
  Header header_;
  Platform platform_;
  Catalog catalog_;
  FileStore store_;
  std::unique_ptr<UploadServer> server_;

  ui::Screen screen_ = ui::Screen::kChecking;
  std::atomic<bool> check_done_{false};
  mutable std::mutex urls_mutex_;
  std::vector<std::string> urls_;
  std::string server_error_;
};

}  // namespace onboarding
