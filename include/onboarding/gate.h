// First-run gate: makes sure the game files from the embedded manifest are on
// disk (phone upload, folder or file pickers) before the game starts.
#pragma once

#include <filesystem>
#include <string>

#include "onboarding/platform.h"

namespace onboarding {

// Shown at the top of every page so the user can tell which game and build it
// belongs to.
struct Header {
  std::string title;
  std::string game_build;
  std::string package_version;
};

// Runs the flow on the calling (UI) thread with its own window and renderer,
// torn down before returning so the game can set up its own. Returns true when
// every file verified and the player continued, false when they quit. Skips the
// window entirely when the files are already in place.
bool EnsureGameFiles(const std::filesystem::path& user_root, Header header, Platform platform);

}  // namespace onboarding
