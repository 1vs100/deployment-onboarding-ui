// What the onboarding view needs from the program hosting it. Everything the
// view cannot do itself without a platform API lives behind this struct, so the
// view builds and runs unchanged on a desktop with no pickers and no console.
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace onboarding {

class Catalog;
class FileStore;

// The page the phone surface is served from, and the upload endpoint it posts
// to. The host owns the transport: sockets, capabilities and TLS are platform
// concerns, while the page and the JSON contract belong to the view.
class UploadServer {
 public:
  virtual ~UploadServer() = default;
  // Binds and starts accepting. Blocks until the bind is decided, so it must
  // not run on the UI thread.
  virtual bool Start(uint16_t port, std::string* error) = 0;
};

struct Platform {
  // Runs background work; may block. Worker lifetime is the caller's problem.
  std::function<void(std::function<void()>)> run_worker;
  // Pickers must start on the UI thread and copy on a worker. Both return
  // immediately and report through the catalog.
  std::function<void()> pick_folder;
  std::function<void()> pick_files;
  // False on a console with no USB drive attached, where the pickers cannot
  // reach anything.
  std::function<bool()> pickers_available;
  // A picker is open, or a copy it started is still running.
  std::function<bool()> transfer_active;
  // Addresses other devices on the network can reach the phone surface at.
  std::function<std::vector<std::string>()> lan_urls;
  // Null when the platform serves no phone surface.
  std::function<std::unique_ptr<UploadServer>(FileStore&, Catalog&)> make_upload_server;
  std::function<void(const std::string&)> log;
};

}  // namespace onboarding
