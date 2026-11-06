#include "core/sha256.h"

#include <windows.h>
#include <bcrypt.h>

#include <stdexcept>

namespace onboarding {
namespace {

void Check(NTSTATUS status, const char* what) {
  if (!BCRYPT_SUCCESS(status)) throw std::runtime_error(what);
}

struct WindowsHash {
  BCRYPT_ALG_HANDLE algorithm = nullptr;
  BCRYPT_HASH_HANDLE hash = nullptr;
};

}  // namespace

struct Sha256::Impl {
  WindowsHash state;
};

Sha256::Sha256() : impl_(std::make_unique<Impl>()) {
  Check(BCryptOpenAlgorithmProvider(&impl_->state.algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0),
        "BCryptOpenAlgorithmProvider");
  Check(BCryptCreateHash(impl_->state.algorithm, &impl_->state.hash, nullptr, 0, nullptr, 0, 0),
        "BCryptCreateHash");
}

Sha256::~Sha256() {
  if (impl_->state.hash) BCryptDestroyHash(impl_->state.hash);
  if (impl_->state.algorithm) BCryptCloseAlgorithmProvider(impl_->state.algorithm, 0);
}

void Sha256::Update(const void* data, size_t size) {
  const auto* bytes = static_cast<const uint8_t*>(data);
  while (size > 0) {
    ULONG chunk = static_cast<ULONG>(size > 0x40000000 ? 0x40000000 : size);
    Check(BCryptHashData(impl_->state.hash, const_cast<PUCHAR>(bytes), chunk, 0), "BCryptHashData");
    bytes += chunk;
    size -= chunk;
  }
}

Sha256Digest Sha256::Finish() {
  Sha256Digest digest{};
  Check(BCryptFinishHash(impl_->state.hash, digest.data(), static_cast<ULONG>(digest.size()), 0),
        "BCryptFinishHash");
  return digest;
}

}  // namespace onboarding
