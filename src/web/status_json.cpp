#include "onboarding/web.h"

#include <cstdio>
#include <string>

#include "web_page.h"

namespace onboarding {
namespace {

std::string JsonString(std::string_view text) {
  std::string out = "\"";
  for (char c : text) {
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (static_cast<unsigned char>(c) < 0x20) {
          char escaped[8];
          std::snprintf(escaped, sizeof(escaped), "\\u%04x", c);
          out += escaped;
        } else {
          out += c;
        }
    }
  }
  return out + "\"";
}

}  // namespace

std::string StatusJson(const Snapshot& snapshot) {
  std::string json = "{\"files\":[";
  for (size_t i = 0; i < snapshot.files.size(); ++i) {
    const FileView& file = snapshot.files[i];
    if (i) json += ',';
    json += "{\"path\":" + JsonString(file.entry->path) +
            ",\"size\":" + std::to_string(file.entry->size) +
            ",\"state\":" + JsonString(StatusName(file.status)) +
            ",\"done\":" + std::to_string(file.done) +
            ",\"alternative\":" + (file.entry->alternative() ? "true" : "false") +
            ",\"covered\":" + (file.covered ? "true" : "false") + "}";
  }
  json += "],\"ready\":" + std::to_string(snapshot.ready_count) +
          ",\"count\":" + std::to_string(snapshot.needed_count) +
          ",\"bytes_ready\":" + std::to_string(snapshot.ready_bytes) +
          ",\"bytes_total\":" + std::to_string(snapshot.total_bytes) +
          ",\"checking\":" + (snapshot.checking ? "true" : "false") +
          ",\"error\":" + JsonString(snapshot.error) + "}";
  return json;
}

std::string_view WebPage() {
  return std::string_view(reinterpret_cast<const char*>(kWebPage), sizeof(kWebPage));
}

}  // namespace onboarding
