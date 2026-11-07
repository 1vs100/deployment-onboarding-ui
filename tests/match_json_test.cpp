// MatchJson: one answer per picked path, in request order.
#include <cstdio>
#include <string>

#include "core/manifest.h"
#include "onboarding/web.h"

namespace {

int failures = 0;

void Expect(const std::string& what, const std::string& actual, const std::string& expected) {
  if (actual == expected) return;
  std::fprintf(stderr, "FAIL %s: got %s, want %s\n", what.c_str(), actual.c_str(), expected.c_str());
  ++failures;
}

}  // namespace

int main() {
  const onboarding::Manifest manifest = onboarding::Manifest::Parse(
      "0000000000000000000000000000000000000000000000000000000000000000 1 game content/GAME\n"
      "1111111111111111111111111111111111111111111111111111111111111111 2 show content/SHOW\n");
  const auto match = [&](std::string_view path) { return manifest.Match(path); };
  Expect("empty", onboarding::MatchJson(manifest, "", match), "[]");
  Expect("paths", onboarding::MatchJson(manifest, "dump/game\nother.bin\nSHOW", match),
         "[\"content/GAME\",null,\"content/SHOW\"]");
  Expect("host rule", onboarding::MatchJson(manifest, "a\"b", [](std::string_view) {
           return std::optional<onboarding::FileIndex>(1);
         }),
         "[\"content/SHOW\"]");
  if (failures == 0) std::printf("match_json_test ok\n");
  return failures == 0 ? 0 : 1;
}
