// Turns page state into a Scene. All spacing and wording lives here, so both
// flavors draw the same layout by construction.
#pragma once

#include <string>
#include <vector>

#include "core/catalog.h"
#include "onboarding/gate.h"
#include "ui/scene.h"
#include "ui/screen.h"

namespace onboarding::ui {

struct Page {
  Screen screen = Screen::kChecking;
  Snapshot snapshot;
  Header header;
  std::vector<std::string> urls;
  std::string server_error;
  bool pickers_available = false;
  // A picker or a copy the UI started is still running.
  bool importing = false;
  int focused = 0;
};

Scene BuildScene(const Page& page);

}  // namespace onboarding::ui
