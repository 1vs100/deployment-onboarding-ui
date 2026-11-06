// The phone surface: the page a browser gets, and the JSON its script polls.
#pragma once

#include <string>
#include <string_view>

#include "core/catalog.h"

namespace onboarding {

std::string StatusJson(const Snapshot& snapshot);
// The embedded upload page.
std::string_view WebPage();

}  // namespace onboarding
