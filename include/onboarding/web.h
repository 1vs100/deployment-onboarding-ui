// The phone surface: the page a browser gets, and the JSON its script polls.
#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>

#include "core/catalog.h"

namespace onboarding {

using PathMatcher = std::function<std::optional<FileIndex>(std::string_view path)>;

std::string StatusJson(const Snapshot& snapshot);
// Answers POST /match: `paths` holds one picked path per line, the result is
// a JSON array with the matched manifest path, or null, for each line.
std::string MatchJson(const Manifest& manifest, std::string_view paths, const PathMatcher& match);
// The embedded upload page.
std::string_view WebPage();

}  // namespace onboarding
