// Incremental SHA-256. Windows uses CNG (BCrypt), which store apps may use;
// other platforms use the implementation in sha256_posix.cpp.
#pragma once

#include <cstddef>
#include <memory>

#include "core/manifest.h"

namespace onboarding {

class Sha256 {
 public:
  Sha256();
  ~Sha256();
  Sha256(const Sha256&) = delete;
  Sha256& operator=(const Sha256&) = delete;

  void Update(const void* data, size_t size);
  // Ends the hash. The object cannot be reused.
  Sha256Digest Finish();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace onboarding
