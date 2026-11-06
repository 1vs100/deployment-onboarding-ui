#include "ui/layout.h"

#include <algorithm>
#include <cstdio>
#include <string>

#include "onboarding/qr.h"
#include "ui/design.h"

namespace onboarding::ui {
namespace {

constexpr size_t kMissingLines = 3;
constexpr float kRightX = 400;
constexpr float kRightW = 808;

std::string FormatBytes(uint64_t bytes) {
  static const char* kUnits[] = {"B", "KB", "MB", "GB", "TB"};
  double value = static_cast<double>(bytes);
  int unit = 0;
  while (value >= 1024 && unit < 4) {
    value /= 1024;
    ++unit;
  }
  char text[32];
  std::snprintf(text, sizeof(text), unit ? "%.1f %s" : "%.0f %s", value, kUnits[unit]);
  return text;
}

// Keeps the end of a path, which is the informative part.
std::string Shorten(const std::string& text, size_t max_chars) {
  if (text.size() <= max_chars) return text;
  return "..." + text.substr(text.size() - (max_chars - 3));
}

// Bytes in flight count towards both bars, so a slow upload does not look
// stalled while it is being written.
uint64_t InFlight(const Snapshot& snapshot) {
  uint64_t bytes = 0;
  for (const auto& file : snapshot.files) {
    if (file.status == FileStatus::kTransferring || file.status == FileStatus::kVerifying) {
      bytes += file.done;
    }
  }
  return bytes;
}

float OverallFraction(const Snapshot& snapshot) {
  const uint64_t in_flight = InFlight(snapshot);
  if (snapshot.checking) {
    uint64_t decided = 0, all = 0;
    for (const auto& file : snapshot.files) {
      all += file.entry->size;
      if (file.status != FileStatus::kUnchecked && file.status != FileStatus::kVerifying) {
        decided += file.entry->size;
      }
    }
    return all ? static_cast<float>(double(decided + in_flight) / all) : 1.0f;
  }
  return snapshot.total_bytes ? static_cast<float>(double(snapshot.ready_bytes + in_flight) /
                                                  snapshot.total_bytes)
                              : 1.0f;
}

void AddText(Scene& scene, Rect box, std::string text, TextSize size, Color color,
             TextAlign align = TextAlign::kLeft) {
  scene.texts.push_back({box, std::move(text), size, color, align});
}

void AddBar(Scene& scene, Rect rect, float fraction) {
  scene.bars.push_back({rect, color::kSurfaceRaised, color::kAccent,
                        std::clamp(fraction, 0.0f, 1.0f)});
}

void AddHeader(Scene& scene, const Header& header) {
  std::string text = header.title;
  for (const std::string* part : {&header.game_build, &header.package_version}) {
    if (!part->empty()) text += "  " + *part;
  }
  AddText(scene, {kMargin, 24, kCanvasWidth - 2 * kMargin, 20}, std::move(text), TextSize::kSmall,
          color::kTextMuted);
}

void AddButtons(Scene& scene, const Page& page) {
  if (page.screen == Screen::kGetFiles) {
    const bool enabled = page.pickers_available && !page.importing;
    scene.buttons.push_back({{kMargin, 640, 288, 56}, "Pick a folder", enabled});
    scene.buttons.push_back({{kMargin + 304, 640, 288, 56}, "Pick files", enabled});
  } else if (page.screen == Screen::kReady) {
    scene.buttons.push_back({{kCanvasWidth / 2 - 144, 420, 288, 56}, "Continue", true});
  }
  if (!scene.buttons.empty()) {
    scene.focused = std::clamp(page.focused, 0, static_cast<int>(scene.buttons.size()) - 1);
  }
}

void AddProgressPanel(Scene& scene, const Snapshot& snapshot, Rect panel) {
  scene.panels.push_back({panel, color::kSurface, true});
  const float x = panel.x + 24;
  const float w = panel.w - 48;
  float y = panel.y + 16;
  if (snapshot.current) {
    const FileView& file = snapshot.files[*snapshot.current];
    AddText(scene, {x, y, w, 26},
            std::string(file.status == FileStatus::kVerifying ? "Verifying " : "Receiving ") +
                Shorten(file.entry->path, 44) + "  " + FormatBytes(file.done) + " of " +
                FormatBytes(file.entry->size),
            TextSize::kSmall, color::kTextMuted);
    float fraction = file.entry->size ? static_cast<float>(double(file.done) / file.entry->size) : 1;
    AddBar(scene, {x, y + 28, w, 8}, fraction);
    y += 46;
  }
  AddText(scene, {x, y + 16, w, 24}, "Overall", TextSize::kSmall, color::kTextMuted);
  AddBar(scene, {x, y + 42, w, 10}, OverallFraction(snapshot));
}

void AddChecking(Scene& scene, const Page& page) {
  AddText(scene, {kMargin, 120, kCanvasWidth - 2 * kMargin, 56}, "Checking your game files",
          TextSize::kTitle, color::kText);
  AddText(scene, {kMargin, 188, kCanvasWidth - 2 * kMargin, 36},
          "Every game file is hashed before the game starts.", TextSize::kBody, color::kTextMuted);
  AddProgressPanel(scene, page.snapshot, {kMargin, 280, kCanvasWidth - 2 * kMargin, 128});
}

void AddGetFiles(Scene& scene, const Page& page) {
  const Snapshot& snapshot = page.snapshot;
  AddText(scene, {kMargin, 84, kCanvasWidth - 2 * kMargin, 56}, "Get your game files",
          TextSize::kTitle, color::kText);
  AddText(scene, {kMargin, 148, kCanvasWidth - 2 * kMargin, 36},
          "This device has no game files yet. Send them from a phone, or copy them from a USB "
          "drive.",
          TextSize::kBody, color::kTextMuted);

  // Phone surface: a QR code the camera can read, with the URL spelled out for
  // anyone who would rather type it.
  if (!page.urls.empty()) {
    scene.panels.push_back({{kMargin, 208, 288, 316}, color::kSurface, true});
    if (QrMatrix encoded = EncodeQr(page.urls.front()); encoded.modules > 0) {
      QrBlock qr;
      qr.box = {kMargin + 24, 232, 240, 240};
      qr.modules = encoded.modules;
      qr.bits = std::move(encoded.bits);
      scene.qr = std::move(qr);
    }
    AddText(scene, {kMargin, 488, 288, 26}, "Scan with a phone", TextSize::kSmall, color::kTextMuted,
            TextAlign::kCenter);
  }

  AddText(scene, {kRightX, 208, kRightW, 26}, "Open in a browser on the same network:",
          TextSize::kSmall, color::kTextMuted);
  if (page.urls.empty()) {
    AddText(scene, {kRightX, 238, kRightW, 36},
            page.server_error.empty() ? "No network connection"
                                      : Shorten(page.server_error, 60),
            TextSize::kBody, color::kError);
  }
  for (size_t i = 0; i < page.urls.size() && i < 3; ++i) {
    AddText(scene, {kRightX, 238 + static_cast<float>(i) * 36, kRightW, 36}, page.urls[i],
            TextSize::kBody, color::kText);
  }

  if (!page.urls.empty()) {
    AddText(scene, {kRightX, 356, kRightW, 26},
            page.pickers_available ? "Or copy from a folder or USB drive:"
                                   : "Or insert a USB drive to copy the files from it.",
            TextSize::kSmall, color::kTextMuted);
  }

  if (snapshot.busy() || page.importing) {
    AddProgressPanel(scene, snapshot, {kRightX, 400, kRightW, 130});
  } else {
    AddText(scene, {kRightX, 392, kRightW, 36},
            std::to_string(snapshot.ready_count) + " of " + std::to_string(snapshot.needed_count) +
                " files ready",
            TextSize::kBody, color::kText);
    AddText(scene, {kRightX, 428, kRightW, 26},
            FormatBytes(snapshot.ready_bytes) + " of " + FormatBytes(snapshot.total_bytes),
            TextSize::kSmall, color::kTextMuted);

    std::vector<const FileView*> missing;
    for (const auto& file : snapshot.files) {
      if (file.status != FileStatus::kOk && !file.covered) missing.push_back(&file);
    }
    float y = 470;
    if (!missing.empty()) {
      AddText(scene, {kRightX, y, kRightW, 26}, "Missing or invalid:", TextSize::kSmall,
              color::kTextMuted);
      for (size_t i = 0; i < missing.size() && i < kMissingLines; ++i) {
        y += 26;
        const bool invalid = missing[i]->status == FileStatus::kInvalid;
        std::string line = Shorten(missing[i]->entry->path, 44);
        if (invalid) {
          line += "  (invalid)";
        } else if (missing[i]->entry->alternative()) {
          line += "  (or the other avatar file)";
        }
        AddText(scene, {kRightX, y, kRightW, 26}, std::move(line), TextSize::kSmall,
                invalid ? color::kError : color::kText);
      }
      if (missing.size() > kMissingLines) {
        y += 26;
        AddText(scene, {kRightX, y, kRightW, 26},
                "and " + std::to_string(missing.size() - kMissingLines) + " more", TextSize::kSmall,
                color::kTextMuted);
      }
    }
  }

  std::string error = !snapshot.error.empty() ? snapshot.error : page.server_error;
  if (!error.empty()) {
    AddText(scene, {kRightX, 656, kRightW, 24}, Shorten(error, 60), TextSize::kSmall, color::kError);
  }
}

void AddReady(Scene& scene) {
  AddText(scene, {kMargin, 240, kCanvasWidth - 2 * kMargin, 56}, "Everything is ready",
          TextSize::kTitle, color::kText, TextAlign::kCenter);
  AddText(scene, {kMargin, 316, kCanvasWidth - 2 * kMargin, 36},
          "All game files are on this device and verified.", TextSize::kBody, color::kTextMuted,
          TextAlign::kCenter);
}

void AddDone(Scene& scene) {
  AddText(scene, {kMargin, 300, kCanvasWidth - 2 * kMargin, 56}, "Starting the game",
          TextSize::kTitle, color::kText, TextAlign::kCenter);
  AddText(scene, {kMargin, 376, kCanvasWidth - 2 * kMargin, 36}, "Setup is complete.",
          TextSize::kBody, color::kTextMuted, TextAlign::kCenter);
}

}  // namespace

Scene BuildScene(const Page& page) {
  Scene scene;
  AddHeader(scene, page.header);
  switch (page.screen) {
    case Screen::kChecking: AddChecking(scene, page); break;
    case Screen::kGetFiles: AddGetFiles(scene, page); break;
    case Screen::kReady: AddReady(scene); break;
    case Screen::kDone: AddDone(scene); break;
  }
  AddButtons(scene, page);
  return scene;
}

}  // namespace onboarding::ui
