/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "android-base/mapped_file.h"

#include <utility>

#include <errno.h>

namespace android::base {

static constexpr char kEmptyBuffer[] = {'0'};

static off64_t initPageSize() noexcept {
#if defined(_WIN32)
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  return si.dwAllocationGranularity;
#else
  return sysconf(_SC_PAGE_SIZE);
#endif
}

std::optional<MappedFile> MappedFile::Create(borrowed_fd fd, off64_t offset, size_t length,
                                             int prot) noexcept {
#if defined(_WIN32)
  return Create(reinterpret_cast<HANDLE>(_get_osfhandle(fd.get())), offset, length, prot);
#else
  return Create(fd.get(), offset, length, prot);
#endif
}

std::optional<MappedFile> MappedFile::Create(os_handle h, off64_t offset, size_t length,
                                             int prot) noexcept {
  static const off64_t page_size = initPageSize();
  size_t slop = offset % page_size;
  off64_t file_offset = offset - slop;
  off64_t file_length = length + slop;

#if defined(_WIN32)
  HANDLE handle = CreateFileMappingW(
      h, nullptr, (prot & PROT_WRITE) ? PAGE_READWRITE : PAGE_READONLY, 0, 0, nullptr);
  if (handle == nullptr) {
    // http://b/119818070 "app crashes when reading asset of zero length".
    // Return a MappedFile that's only valid for reading the size.
    if (length == 0 && ::GetLastError() == ERROR_FILE_INVALID) {
      return MappedFile(const_cast<char*>(kEmptyBuffer), 0, 0, nullptr);
    }
    return {};
  }
  void* base = MapViewOfFile(handle, (prot & PROT_WRITE) ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ,
                             (file_offset >> 32), file_offset, file_length);
  if (base == nullptr) {
    CloseHandle(handle);
    return {};
  }
  return MappedFile(static_cast<char*>(base), length, slop, handle);
#else
  void* base = mmap(nullptr, file_length, prot, MAP_SHARED, h, file_offset);
  if (base == MAP_FAILED) {
    // http://b/119818070 "app crashes when reading asset of zero length".
    // mmap fails with EINVAL for a zero length region.
    if (errno == EINVAL && length == 0) {
      return MappedFile(const_cast<char*>(kEmptyBuffer), 0, 0);
    }
    return {};
  }
  return MappedFile(static_cast<char*>(base), length, slop);
#endif
}

std::unique_ptr<MappedFile> MappedFile::FromFd(borrowed_fd fd, off64_t offset, size_t length,
                                               int prot) {
  auto mf = Create(fd, offset, length, prot);
  return std::unique_ptr<MappedFile>(mf ? new MappedFile(std::move(*mf)) : nullptr);
}

MappedFile::MappedFile(MappedFile&& other) noexcept
    : base_(std::exchange(other.base_, const_cast<char*>(kEmptyBuffer))),
      size_(std::exchange(other.size_, 0)),
      offset_(std::exchange(other.offset_, 0))
#ifdef _WIN32
      ,
      handle_(std::exchange(other.handle_, nullptr))
#endif
{
}

MappedFile& MappedFile::operator=(MappedFile&& other) noexcept {
  close();
  base_ = std::exchange(other.base_, const_cast<char*>(kEmptyBuffer));
  size_ = std::exchange(other.size_, 0);
  offset_ = std::exchange(other.offset_, 0);
#ifdef _WIN32
  handle_ = std::exchange(other.handle_, nullptr);
#endif
  return *this;
}

MappedFile::~MappedFile() noexcept {
  close();
}

void MappedFile::close() noexcept {
#if defined(_WIN32)
  if (base_ != nullptr && size_ != 0) UnmapViewOfFile(base_);
  if (handle_ != nullptr) CloseHandle(handle_);
  handle_ = nullptr;
#else
  if (base_ != nullptr && size_ != 0) munmap(base_, size_ + offset_);
#endif

  base_ = nullptr;
  offset_ = size_ = 0;
}

}  // namespace android::base
