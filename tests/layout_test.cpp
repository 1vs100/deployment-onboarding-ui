// Layout invariants: the things a flavor cannot fix, checked without a
// renderer.
#include <cstdio>
#include <string>
#include <vector>

#include "core/catalog.h"
#include "core/manifest.h"
#include "ui/layout.h"
#include "ui/scene.h"
#include "ui/screen.h"

namespace {

using namespace onboarding;

int failures = 0;

void Check(bool condition, const std::string& what) {
  if (condition) return;
  std::fprintf(stderr, "FAIL %s\n", what.c_str());
  ++failures;
}

void CheckInside(const ui::Rect& rect, const std::string& what) {
  Check(rect.x >= 0 && rect.y >= 0, what + ": starts outside the canvas");
  Check(rect.right() <= ui::kCanvasWidth, what + ": runs past the right edge");
  Check(rect.bottom() <= ui::kCanvasHeight, what + ": runs past the bottom edge");
}

void CheckScene(const ui::Scene& scene, const std::string& name) {
  for (size_t i = 0; i < scene.panels.size(); ++i) {
    CheckInside(scene.panels[i].rect, name + " panel " + std::to_string(i));
  }
  for (size_t i = 0; i < scene.texts.size(); ++i) {
    CheckInside(scene.texts[i].box, name + " text " + std::to_string(i));
    Check(!scene.texts[i].text.empty(), name + " text " + std::to_string(i) + ": empty");
  }
  for (size_t i = 0; i < scene.bars.size(); ++i) {
    CheckInside(scene.bars[i].rect, name + " bar " + std::to_string(i));
    Check(scene.bars[i].fraction >= 0 && scene.bars[i].fraction <= 1,
          name + " bar " + std::to_string(i) + ": fraction outside 0..1");
  }
  for (size_t i = 0; i < scene.buttons.size(); ++i) {
    CheckInside(scene.buttons[i].rect, name + " button " + std::to_string(i));
  }
  if (scene.qr) CheckInside(scene.qr->box, name + " qr");
  if (scene.buttons.empty()) {
    Check(scene.focused == -1, name + ": focus without buttons");
  } else {
    Check(scene.focused >= 0 && scene.focused < static_cast<int>(scene.buttons.size()),
          name + ": focus outside the buttons");
  }
}

ui::Page MakePage(const Catalog& catalog, ui::Screen screen, bool urls) {
  ui::Page page;
  page.screen = screen;
  page.header = Header{"1 vs 100", "2.0.1149.0", "0.1.151.0"};
  page.pickers_available = true;
  page.snapshot = catalog.Take();
  if (urls) page.urls = {"http://192.168.1.42:8080/"};
  return page;
}

}  // namespace

int main() {
  Catalog catalog(Manifest::Embedded());
  for (FileIndex i = 0; i < catalog.manifest().entries().size(); ++i) {
    catalog.SetStatus(i, i % 4 == 1 ? FileStatus::kMissing : FileStatus::kOk);
  }
  catalog.SetChecking(false);

  const ui::Scene get_files = ui::BuildScene(MakePage(catalog, ui::Screen::kGetFiles, true));
  Check(get_files.buttons.size() == 2, "get-files: two pickers");
  Check(get_files.qr.has_value(), "get-files: a QR code when there is an address");
  Check(get_files.texts.size() > 5, "get-files: explains where the files go");
  CheckScene(get_files, "get-files");

  const ui::Scene no_address = ui::BuildScene(MakePage(catalog, ui::Screen::kGetFiles, false));
  Check(!no_address.qr.has_value(), "get-files: no QR code without an address");
  CheckScene(no_address, "get-files without address");

  const ui::Scene ready = ui::BuildScene(MakePage(catalog, ui::Screen::kReady, true));
  Check(ready.buttons.size() == 1, "ready: one button");
  Check(ready.buttons[0].label == "Continue", "ready: the button continues");
  CheckScene(ready, "ready");

  const ui::Scene done = ui::BuildScene(MakePage(catalog, ui::Screen::kDone, true));
  Check(done.buttons.empty(), "done: no buttons");
  CheckScene(done, "done");

  catalog.SetChecking(true);
  const ui::Scene idle_checking = ui::BuildScene(MakePage(catalog, ui::Screen::kChecking, true));
  Check(idle_checking.bars.size() == 1, "checking: overall while nothing is hashed");
  CheckScene(idle_checking, "checking");

  catalog.SetProgress(0, FileStatus::kVerifying, 1024);
  const ui::Scene checking = ui::BuildScene(MakePage(catalog, ui::Screen::kChecking, true));
  Check(checking.bars.size() == 2, "checking: current file and overall");
  Check(checking.buttons.empty(), "checking: no buttons");
  CheckScene(checking, "checking");

  // A picker that cannot reach anything disables both buttons but keeps them
  // visible, so the page still says what to do.
  catalog.SetChecking(false);
  ui::Page blocked = MakePage(catalog, ui::Screen::kGetFiles, true);
  blocked.pickers_available = false;
  const ui::Scene no_usb = ui::BuildScene(blocked);
  Check(no_usb.buttons.size() == 2, "no USB: both buttons stay");
  Check(!no_usb.buttons[0].enabled && !no_usb.buttons[1].enabled, "no USB: both buttons disabled");
  CheckScene(no_usb, "no USB");

  if (failures) std::fprintf(stderr, "%d layout check(s) failed\n", failures);
  return failures ? 1 : 0;
}
