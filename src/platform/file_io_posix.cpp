#include "platform/file_io.h"

#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <vector>

namespace onboarding {
namespace {

void SetError(Win32Error* error, Win32Error value) {
  if (error) *error = value;
}

// The header keeps a void* handle for the Windows build; here it carries the
// descriptor so both implementations share one declaration.
void* Pack(int fd) { return reinterpret_cast<void*>(static_cast<intptr_t>(fd)); }
int Unpack(void* handle) { return static_cast<int>(reinterpret_cast<intptr_t>(handle)); }

}  // namespace

bool IsDiskFull(Win32Error error) { return error == ENOSPC || error == EDQUOT; }

File::File(File&& other) noexcept : handle_(other.handle_) { other.handle_ = nullptr; }

File& File::operator=(File&& other) noexcept {
  if (this != &other) {
    Close();
    handle_ = other.handle_;
    other.handle_ = nullptr;
  }
  return *this;
}

File::~File() { Close(); }

File File::OpenRead(const std::filesystem::path& path, Win32Error* error) {
  int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
  if (fd < 0) {
    SetError(error, static_cast<Win32Error>(errno));
    return File();
  }
  SetError(error, 0);
  return File(Pack(fd));
}

File File::Create(const std::filesystem::path& path, Win32Error* error) {
  int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
  if (fd < 0) {
    SetError(error, static_cast<Win32Error>(errno));
    return File();
  }
  SetError(error, 0);
  return File(Pack(fd));
}

std::optional<size_t> File::Read(void* buffer, size_t size, Win32Error* error) {
  ssize_t read = ::read(Unpack(handle_), buffer, size);
  if (read < 0) {
    SetError(error, static_cast<Win32Error>(errno));
    return std::nullopt;
  }
  SetError(error, 0);
  return static_cast<size_t>(read);
}

bool File::Write(const void* data, size_t size, Win32Error* error) {
  const auto* bytes = static_cast<const uint8_t*>(data);
  while (size > 0) {
    ssize_t written = ::write(Unpack(handle_), bytes, size);
    if (written < 0) {
      SetError(error, static_cast<Win32Error>(errno));
      return false;
    }
    bytes += written;
    size -= static_cast<size_t>(written);
  }
  SetError(error, 0);
  return true;
}

bool File::Flush(Win32Error* error) {
  if (::fsync(Unpack(handle_)) != 0) {
    SetError(error, static_cast<Win32Error>(errno));
    return false;
  }
  SetError(error, 0);
  return true;
}

void File::Close() {
  if (handle_) {
    ::close(Unpack(handle_));
    handle_ = nullptr;
  }
}

std::optional<FileStamp> StatFile(const std::filesystem::path& path) {
  struct stat data {};
  if (::stat(path.c_str(), &data) != 0 || S_ISDIR(data.st_mode)) return std::nullopt;
  uint64_t write_time = static_cast<uint64_t>(data.st_mtim.tv_sec) * 10000000ull +
                        static_cast<uint64_t>(data.st_mtim.tv_nsec) / 100ull;
  return FileStamp{static_cast<uint64_t>(data.st_size), write_time};
}

bool CreateDirectories(const std::filesystem::path& path) {
  if (path.empty()) return true;
  std::error_code ignored;
  std::filesystem::create_directories(path, ignored);
  return std::filesystem::is_directory(path, ignored);
}

bool DeleteFileIfPresent(const std::filesystem::path& path) {
  return ::unlink(path.c_str()) == 0 || errno == ENOENT;
}

bool MoveIntoPlace(const std::filesystem::path& from, const std::filesystem::path& to,
                   Win32Error* error) {
  if (!DeleteFileIfPresent(to) || ::rename(from.c_str(), to.c_str()) != 0) {
    SetError(error, static_cast<Win32Error>(errno));
    return false;
  }
  SetError(error, 0);
  return true;
}

}  // namespace onboarding
