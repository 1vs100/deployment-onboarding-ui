#include "onboarding/qr.h"

#include "thirdparty/qrcodegen/qrcodegen.hpp"

namespace onboarding {

QrMatrix EncodeQr(const std::string& text) {
  QrMatrix out;
  try {
    const qrcodegen::QrCode code =
        qrcodegen::QrCode::encodeText(text.c_str(), qrcodegen::QrCode::Ecc::MEDIUM);
    out.modules = code.getSize();
    out.bits.resize(static_cast<size_t>(out.modules) * out.modules);
    for (int row = 0; row < out.modules; ++row) {
      for (int col = 0; col < out.modules; ++col) {
        out.bits[static_cast<size_t>(row) * out.modules + col] = code.getModule(col, row) ? 1 : 0;
      }
    }
  } catch (const qrcodegen::data_too_long&) {
    out = QrMatrix{};
  }
  return out;
}

}  // namespace onboarding
