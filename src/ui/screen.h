// Which page the view is showing. Separate from the scene so layout and the
// flow can both name it without a dependency cycle.
#pragma once

namespace onboarding::ui {

enum class Screen {
  kChecking,
  kGetFiles,
  kReady,
  kDone,
};

}  // namespace onboarding::ui
