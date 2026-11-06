#include "onboarding/gate.h"

#include <fstream>
#include <utility>

#include "core/app.h"
#include "ui/view.h"

namespace onboarding {

bool EnsureGameFiles(const std::filesystem::path& user_root, Header header, Platform platform) {
  // Fast path without any window: everything already verified.
  {
    App probe(user_root, header, platform);
    probe.CheckNow();
    if (probe.AllReady()) return true;
  }

  ui::Session session;
  std::string error;
  if (!ui::OpenSession({}, &session, &error)) {
    // The game log is not open yet, so the reason goes to the user folder.
    std::ofstream(user_root / "onboarding-error.txt")
        << "window creation failed: " << error << "\n";
    return false;
  }

  // Never destroyed: worker threads and the upload server may still reference
  // it, as upstream does.
  auto* app = new App(user_root, std::move(header), std::move(platform));
  app->Start();

  const bool ready = ui::Run(*app, session);
  ui::CloseSession(&session);
  return ready;
}

}  // namespace onboarding
