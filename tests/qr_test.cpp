// EncodeQr: short texts encode, oversized texts fail empty.
#include <cstdint>
#include <cstdio>
#include <string>

#include "onboarding/qr.h"

namespace {

int failures = 0;

void Check(bool condition, const std::string& what) {
  if (condition) return;
  std::fprintf(stderr, "FAIL %s\n", what.c_str());
  ++failures;
}

}  // namespace

int main() {
  const onboarding::QrMatrix qr = onboarding::EncodeQr("http://192.168.1.2:8080/");
  Check(qr.modules > 0, "short url encodes");
  Check(qr.bits.size() == static_cast<size_t>(qr.modules) * qr.modules, "bits match modules");
  bool any_dark = false;
  for (uint8_t bit : qr.bits) {
    Check(bit <= 1, "matrix bits are binary");
    any_dark = any_dark || bit;
  }
  Check(any_dark, "matrix has dark modules");
  const onboarding::QrMatrix empty_text = onboarding::EncodeQr("");
  Check(empty_text.modules > 0, "empty text still encodes");
  Check(empty_text.bits.size() == static_cast<size_t>(empty_text.modules) * empty_text.modules,
        "empty bits match modules");
  const onboarding::QrMatrix too_long = onboarding::EncodeQr(std::string(5000, 'x'));
  Check(too_long.modules == 0, "oversized text fails empty");
  Check(too_long.bits.empty(), "oversized text has no bits");
  if (failures == 0) std::printf("qr_test ok\n");
  return failures == 0 ? 0 : 1;
}
