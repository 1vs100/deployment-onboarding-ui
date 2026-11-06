// QR symbol data for hosts that paint their own UI, without vendoring a
// generator.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace onboarding {

// One encoded QR symbol. Modules run row-major, 1 is dark. The painter adds
// the quiet zone. On success bits holds modules * modules entries of 0 or 1.
struct QrMatrix {
  int modules = 0;
  std::vector<uint8_t> bits;
};

// Encodes text with at least medium error correction (the generator may use a
// higher level when it fits the same version). Returns an empty matrix
// (modules == 0) when the text fits no QR version.
QrMatrix EncodeQr(const std::string& text);

}  // namespace onboarding
