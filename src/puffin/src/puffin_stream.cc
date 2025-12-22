// Copyright 2017 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "puffin/src/puffin_stream.h"
#include <fcntl.h>
#include <sys/stat.h>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <system_error>
#include <utility>
#include <vector>

#include "puffin/src/bit_reader.h"
#include "puffin/src/bit_writer.h"
#include "puffin/src/include/puffin/common.h"
#include "puffin/src/include/puffin/huffer.h"
#include "puffin/src/include/puffin/puffer.h"
#include "puffin/src/include/puffin/stream.h"
#include "puffin/src/logging.h"
#include "puffin/src/puff_reader.h"
#include "puffin/src/puff_writer.h"

using std::shared_ptr;
using std::unique_ptr;
using std::vector;

namespace puffin {

namespace {

bool CheckArgsIntegrity(uint64_t puff_size,
                        const vector<BitExtent>& deflates,
                        const vector<ByteExtent>& puffs) {
  TEST_AND_RETURN_FALSE(puffs.size() == deflates.size());
  // Check if the |puff_size| is actually greater than the last byte of the last
  // puff in |puffs|.
  if (!puffs.empty()) {
    TEST_AND_RETURN_FALSE(puff_size >=
                          puffs.back().offset + puffs.back().length);
  }

  // Check to make sure |puffs| and |deflates| are sorted and non-overlapping.
  auto is_overlap = [](const auto& a, const auto& b) {
    return (a.offset + a.length) > b.offset;
  };
  TEST_AND_RETURN_FALSE(deflates.end() == std::adjacent_find(deflates.begin(),
                                                             deflates.end(),
                                                             is_overlap));
  TEST_AND_RETURN_FALSE(puffs.end() == std::adjacent_find(puffs.begin(),
                                                          puffs.end(),
                                                          is_overlap));
  return true;
}

}  // namespace

UniqueStreamPtr PuffinStream::CreateForPuff(UniqueStreamPtr stream,
                                            shared_ptr<Puffer> puffer,
                                            uint64_t puff_size,
                                            const vector<BitExtent>& deflates,
                                            const vector<ByteExtent>& puffs,
                                            size_t max_cache_size) {
  TEST_AND_RETURN_VALUE(CheckArgsIntegrity(puff_size, deflates, puffs),
                        nullptr);
  TEST_AND_RETURN_VALUE(stream->Seek(0), nullptr);

  UniqueStreamPtr puffin_stream(new PuffinStream(std::move(stream), puffer,
                                                 nullptr, puff_size, deflates,
                                                 puffs, max_cache_size));
  TEST_AND_RETURN_VALUE(puffin_stream->Seek(0), nullptr);
  return puffin_stream;
}

UniqueStreamPtr PuffinStream::CreateForHuff(UniqueStreamPtr stream,
                                            shared_ptr<Huffer> huffer,
                                            uint64_t puff_size,
                                            const vector<BitExtent>& deflates,
                                            const vector<ByteExtent>& puffs) {
  TEST_AND_RETURN_VALUE(CheckArgsIntegrity(puff_size, deflates, puffs),
                        nullptr);
  TEST_AND_RETURN_VALUE(stream->Seek(0), nullptr);

  UniqueStreamPtr puffin_stream(new PuffinStream(
      std::move(stream), nullptr, huffer, puff_size, deflates, puffs, 0));
  TEST_AND_RETURN_VALUE(puffin_stream->Seek(0), nullptr);
  return puffin_stream;
}

PuffinStream::PuffinStream(UniqueStreamPtr stream,
                           shared_ptr<Puffer> puffer,
                           shared_ptr<Huffer> huffer,
                           uint64_t puff_size,
                           const vector<BitExtent>& deflates,
                           const vector<ByteExtent>& puffs,
                           size_t max_cache_size)
    : stream_(std::move(stream)),
      puffer_(puffer),
      huffer_(huffer),
      puff_stream_size_(puff_size),
      deflates_(deflates),
      puffs_(puffs),
      puff_pos_(0),
      skip_bytes_(0),
      deflate_bit_pos_(0),
      last_byte_(0),
      extra_byte_(0),
      is_for_puff_(puffer_ ? true : false),
      closed_(false),
      lru_cache_(max_cache_size) {
  // Building upper bounds for faster seek.
  upper_bounds_.reserve(puffs.size());
  for (const auto& puff : puffs) {
    upper_bounds_.emplace_back(puff.offset + puff.length);
  }
  upper_bounds_.emplace_back(puff_stream_size_ + 1);

  // We can pass the size of the deflate stream too, but it is not necessary
  // yet. We cannot get the size of stream from itself, because we might be
  // writing into it and its size is not defined yet.
  uint64_t deflate_stream_size = puff_stream_size_;
  if (!puffs.empty()) {
    deflate_stream_size =
        ((deflates.back().offset + deflates.back().length) / 8) +
        puff_stream_size_ - (puffs.back().offset + puffs.back().length);
  }

  deflates_.emplace_back(deflate_stream_size * 8, 0);
  puffs_.emplace_back(puff_stream_size_, 0);

  // Look for the largest puff and deflate extents and get proper size buffers.
  uint64_t max_puff_length = 0;
  for (const auto& puff : puffs) {
    max_puff_length = std::max(max_puff_length, puff.length);
  }
  puff_buffer_.reset(new Buffer(max_puff_length + 1));

  uint64_t max_deflate_length = 0;
  for (const auto& deflate : deflates) {
    max_deflate_length = std::max(max_deflate_length, deflate.length * 8);
  }
  deflate_buffer_.reset(new Buffer(max_deflate_length + 2));
}

bool PuffinStream::GetSize(uint64_t* size) const {
  *size = puff_stream_size_;
  return true;
}

bool PuffinStream::GetOffset(uint64_t* offset) const {
  *offset = puff_pos_ + skip_bytes_;
  return true;
}

bool PuffinStream::Seek(uint64_t offset) {
  TEST_AND_RETURN_FALSE(!closed_);
  if (!is_for_puff_) {
    // For huffing we should not seek, only seek to zero is accepted.
    TEST_AND_RETURN_FALSE(offset == 0);
  }

  TEST_AND_RETURN_FALSE(offset <= puff_stream_size_);

  // We are searching first available puff which either includes the |offset| or
  // it is the next available puff after |offset|.
  auto next_puff_iter =
      std::upper_bound(upper_bounds_.begin(), upper_bounds_.end(), offset);
  TEST_AND_RETURN_FALSE(next_puff_iter != upper_bounds_.end());
  auto next_puff_idx = std::distance(upper_bounds_.begin(), next_puff_iter);
  cur_puff_ = std::next(puffs_.begin(), next_puff_idx);
  cur_deflate_ = std::next(deflates_.begin(), next_puff_idx);

  if (offset < cur_puff_->offset) {
    // between two puffs.
    puff_pos_ = offset;
    auto back_track_bytes = cur_puff_->offset - puff_pos_;
    deflate_bit_pos_ = ((cur_deflate_->offset + 7) / 8 - back_track_bytes) * 8;
    if (cur_puff_ != puffs_.begin()) {
      auto prev_deflate = std::prev(cur_deflate_);
      if (deflate_bit_pos_ < prev_deflate->offset + prev_deflate->length) {
        deflate_bit_pos_ = prev_deflate->offset + prev_deflate->length;
      }
    }
  } else {
    // Inside a puff.
    puff_pos_ = cur_puff_->offset;
    deflate_bit_pos_ = cur_deflate_->offset;
  }
  skip_bytes_ = offset - puff_pos_;
  if (!is_for_puff_ && offset == 0) {
    TEST_AND_RETURN_FALSE(stream_->Seek(0));
    TEST_AND_RETURN_FALSE(SetExtraByte());
  }
  return true;
}

bool PuffinStream::Close() {
  closed_ = true;
  return stream_->Close();
}

bool PuffinStream::Read(void* buffer, size_t count) {
  TEST_AND_RETURN_FALSE(!closed_);
  TEST_AND_RETURN_FALSE(is_for_puff_);
  if (cur_puff_ == puffs_.end()) {
    TEST_AND_RETURN_FALSE(count == 0);
  }
  auto bytes = static_cast<uint8_t*>(buffer);
  uint64_t length = count;
  uint64_t bytes_read = 0;
  while (bytes_read < length) {
    if (puff_pos_ < cur_puff_->offset) {
      // Reading between two deflates. We also read bytes that have at least one
      // bit of a deflate bit stream. The byte which has both deflate and raw
      // data will be shifted or masked off the deflate bits and the remaining
      // value will be saved in the puff stream as an byte integer.
      uint64_t start_byte = (deflate_bit_pos_ / 8);
      uint64_t end_byte = (cur_deflate_->offset + 7) / 8;
      auto bytes_to_read = std::min(length - bytes_read, end_byte - start_byte);
      TEST_AND_RETURN_FALSE(bytes_to_read >= 1);

      TEST_AND_RETURN_FALSE(stream_->Seek(start_byte));
      TEST_AND_RETURN_FALSE(stream_->Read(bytes + bytes_read, bytes_to_read));

      // If true, we read the first byte of the curret deflate. So we have to
      // mask out the deflate bits (which are most significant bits.)
      if ((start_byte + bytes_to_read) * 8 > cur_deflate_->offset) {
        bytes[bytes_read + bytes_to_read - 1] &=
            (1 << (cur_deflate_->offset & 7)) - 1;
      }

      // If true, we read the last byte of the previous deflate and we have to
      // shift it out. The least significat bits belongs to the deflate
      // stream. The order of these last two conditions are important because a
      // byte can contain a finishing deflate and a starting deflate with some
      // bits between them so we have to modify correctly. Keep in mind that in
      // this situation both are modifying the same byte.
      if (start_byte * 8 < deflate_bit_pos_) {
        bytes[bytes_read] >>= deflate_bit_pos_ & 7;
      }

      // Pass |deflate_bit_pos_| for all the read bytes.
      deflate_bit_pos_ -= deflate_bit_pos_ & 7;
      deflate_bit_pos_ += bytes_to_read * 8;
      if (deflate_bit_pos_ > cur_deflate_->offset) {
        // In case it reads into the first byte of the current deflate.
        deflate_bit_pos_ = cur_deflate_->offset;
      }

      bytes_read += bytes_to_read;
      puff_pos_ += bytes_to_read;
      TEST_AND_RETURN_FALSE(puff_pos_ <= cur_puff_->offset);
    } else {
      // Reading the deflate itself. We read all bytes including the first and
      // last byte (which may partially include a deflate bit). Here we keep the
      // |puff_pos_| point to the first byte of the puffed stream and
      // |skip_bytes_| shows how many bytes in the puff we have copied till now.
      auto start_byte = (cur_deflate_->offset / 8);
      auto end_byte = (cur_deflate_->offset + cur_deflate_->length + 7) / 8;
      auto bytes_to_read = end_byte - start_byte;
      // Puff directly to buffer if it has space.
      const bool puff_directly_into_buffer =
          lru_cache_.capacity() == 0 && (skip_bytes_ == 0) &&
          (length - bytes_read >= cur_puff_->length);

      auto cur_puff_idx = std::distance(puffs_.begin(), cur_puff_);
      if (lru_cache_.capacity() == 0 ||
          !GetPuffCache(cur_puff_idx, cur_puff_->length, &puff_buffer_)) {
        // Did not find the puff buffer in cache. We have to build it.
        deflate_buffer_->resize(bytes_to_read);
        TEST_AND_RETURN_FALSE(stream_->Seek(start_byte));
        TEST_AND_RETURN_FALSE(
            stream_->Read(deflate_buffer_->data(), bytes_to_read));
        BufferBitReader bit_reader(deflate_buffer_->data(), bytes_to_read);

        BufferPuffWriter puff_writer(puff_directly_into_buffer
                                         ? bytes + bytes_read
                                         : puff_buffer_->data(),
                                     cur_puff_->length);

        // Drop the first unused bits.
        size_t extra_bits_len = cur_deflate_->offset & 7;
        TEST_AND_RETURN_FALSE(bit_reader.CacheBits(extra_bits_len));
        bit_reader.DropBits(extra_bits_len);

        TEST_AND_RETURN_FALSE(
            puffer_->PuffDeflate(&bit_reader, &puff_writer, nullptr));
        TEST_AND_RETURN_FALSE(bytes_to_read == bit_reader.Offset());
        TEST_AND_RETURN_FALSE(cur_puff_->length == puff_writer.Size());
      } else {
        // Just seek to proper location.
        TEST_AND_RETURN_FALSE(stream_->Seek(start_byte + bytes_to_read));
      }
      // Copy from puff buffer to output if needed.
      auto bytes_to_copy =
          std::min(length - bytes_read, cur_puff_->length - skip_bytes_);
      if (!puff_directly_into_buffer) {
        memcpy(bytes + bytes_read, puff_buffer_->data() + skip_bytes_,
               bytes_to_copy);
      }

      skip_bytes_ += bytes_to_copy;
      bytes_read += bytes_to_copy;

      // Move to next puff.
      if (puff_pos_ + skip_bytes_ == cur_puff_->offset + cur_puff_->length) {
        puff_pos_ += skip_bytes_;
        skip_bytes_ = 0;
        deflate_bit_pos_ = cur_deflate_->offset + cur_deflate_->length;
        cur_puff_++;
        cur_deflate_++;
        if (cur_puff_ == puffs_.end()) {
          break;
        }
      }
    }
  }

  TEST_AND_RETURN_FALSE(bytes_read == length);
  return true;
}

bool PuffinStream::Write(const void* buffer, size_t count) {
  TEST_AND_RETURN_FALSE(!closed_);
  TEST_AND_RETURN_FALSE(!is_for_puff_);
  auto bytes = static_cast<const uint8_t*>(buffer);
  uint64_t length = count;
  uint64_t bytes_wrote = 0;
  while (bytes_wrote < length) {
    if (deflate_bit_pos_ < (cur_deflate_->offset & ~7ull)) {
      // Between two puffs or before the first puff. We know that we are
      // starting from the byte boundary because we have already processed the
      // non-deflate bits of the last byte of the last deflate. Here we don't
      // process any byte that has deflate bit.
      TEST_AND_RETURN_FALSE((deflate_bit_pos_ & 7) == 0);
      auto copy_len =
          std::min((cur_deflate_->offset / 8) - (deflate_bit_pos_ / 8),
                   length - bytes_wrote);
      TEST_AND_RETURN_FALSE(stream_->Write(bytes + bytes_wrote, copy_len));
      bytes_wrote += copy_len;
      puff_pos_ += copy_len;
      deflate_bit_pos_ += copy_len * 8;
    } else {
      // We are in a puff. We have to buffer incoming bytes until we reach the
      // size of the current puff so we can huff :). If the last bit of the
      // current deflate does not end in a byte boundary, then we have to read
      // one more byte to fill up the last byte of the deflate stream before
      // doing anything else.

      // |deflate_bit_pos_| now should be in the same byte as
      // |cur_deflate->offset|.
      if (deflate_bit_pos_ < cur_deflate_->offset) {
        last_byte_ |= bytes[bytes_wrote++] << (deflate_bit_pos_ & 7);
        skip_bytes_ = 0;
        deflate_bit_pos_ = cur_deflate_->offset;
        puff_pos_++;
        TEST_AND_RETURN_FALSE(puff_pos_ == cur_puff_->offset);
      }

      auto copy_len = std::min(length - bytes_wrote,
                               cur_puff_->length + extra_byte_ - skip_bytes_);
      TEST_AND_RETURN_FALSE(puff_buffer_->size() >= skip_bytes_ + copy_len);
      memcpy(puff_buffer_->data() + skip_bytes_, bytes + bytes_wrote, copy_len);
      skip_bytes_ += copy_len;
      bytes_wrote += copy_len;

      if (skip_bytes_ == cur_puff_->length + extra_byte_) {
        // |puff_buffer_| is full, now huff into the |deflate_buffer_|.
        auto start_byte = cur_deflate_->offset / 8;
        auto end_byte = (cur_deflate_->offset + cur_deflate_->length + 7) / 8;
        auto bytes_to_write = end_byte - start_byte;

        deflate_buffer_->resize(bytes_to_write);
        BufferBitWriter bit_writer(deflate_buffer_->data(), bytes_to_write);
        BufferPuffReader puff_reader(puff_buffer_->data(), cur_puff_->length);

        // Write last byte if it has any.
        TEST_AND_RETURN_FALSE(
            bit_writer.WriteBits(cur_deflate_->offset & 7, last_byte_));
        last_byte_ = 0;

        TEST_AND_RETURN_FALSE(huffer_->HuffDeflate(&puff_reader, &bit_writer));
        TEST_AND_RETURN_FALSE(bit_writer.Size() == bytes_to_write);
        TEST_AND_RETURN_FALSE(puff_reader.BytesLeft() == 0);

        deflate_bit_pos_ = cur_deflate_->offset + cur_deflate_->length;
        if (extra_byte_ == 1) {
          deflate_buffer_->data()[bytes_to_write - 1] |=
              puff_buffer_->data()[cur_puff_->length] << (deflate_bit_pos_ & 7);
          deflate_bit_pos_ = (deflate_bit_pos_ + 7) & ~7ull;
        } else if ((deflate_bit_pos_ & 7) != 0) {
          // This happens if current and next deflate finish and end on the same
          // byte, then we cannot write into output until we have huffed the
          // next puff buffer, so untill then we cache it into |last_byte_| and
          // we won't write it out.
          last_byte_ = deflate_buffer_->data()[bytes_to_write - 1];
          bytes_to_write--;
        }

        // Write |deflate_buffer_| into output.
        TEST_AND_RETURN_FALSE(
            stream_->Write(deflate_buffer_->data(), bytes_to_write));

        // Move to the next deflate/puff.
        puff_pos_ += skip_bytes_;
        skip_bytes_ = 0;
        cur_puff_++;
        cur_deflate_++;
        if (cur_puff_ == puffs_.end()) {
          break;
        }
        // Find if need an extra byte to cache at the end.
        TEST_AND_RETURN_FALSE(SetExtraByte());
      }
    }
  }

  TEST_AND_RETURN_FALSE(bytes_wrote == length);
  return true;
}

bool PuffinStream::SetExtraByte() {
  TEST_AND_RETURN_FALSE(cur_deflate_ != deflates_.end());
  if ((cur_deflate_ + 1) == deflates_.end()) {
    extra_byte_ = 0;
    return true;
  }
  uint64_t end_bit = cur_deflate_->offset + cur_deflate_->length;
  if ((end_bit & 7) && ((end_bit + 7) & ~7ull) <= (cur_deflate_ + 1)->offset) {
    extra_byte_ = 1;
  } else {
    extra_byte_ = 0;
  }
  return true;
}

bool PuffinStream::GetPuffCache(int puff_id,
                                uint64_t puff_size,
                                shared_ptr<Buffer>* buffer) {
  // Search for it.
  shared_ptr<Buffer> cache = lru_cache_.get(puff_id);
  const bool found = cache != nullptr;

  // If not found, either create one or get one from the list.
  if (!found) {
    // If we have not populated the cache yet, create one.
    lru_cache_.EnsureSpace(puff_size);
    cache = std::make_shared<Buffer>(puff_size);

    lru_cache_.put(puff_id, cache);
  }

  *buffer = std::move(cache);
  return found;
}

const LRUCache::Value LRUCache::get(const Key& key) {
  auto it = items_map_.find(key);
  if (it == items_map_.end()) {
    if (ondisk_items_.count(key) > 0) {
      auto&& data = ReadFromDisk(key);
      put(key, data);
      return data;
    }
    return nullptr;
  }
  items_list_.splice(items_list_.begin(), items_list_, it->second);
  return it->second->second;
}

LRUCache::Value LRUCache::ReadFromDisk(const LRUCache::Key& key) {
  const auto path = tmpdir_ / std::to_string(key);
  int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC);
  if (fd < 0) {
    PLOG(ERROR) << "Failed to open " << path;
    return {};
  }
  auto fd_delete = [](int* fd) { close(*fd); };
  std::unique_ptr<int, decltype(fd_delete)> guard(&fd, fd_delete);
  struct stat st {};
  const int ret = stat(path.c_str(), &st);
  if (ret < 0) {
    PLOG(ERROR) << "Failed to stat " << path << " ret: " << ret;
    return {};
  }
  LRUCache::Value data = std::make_shared<std::vector<uint8_t>>(st.st_size);
  const auto bytes_read =
      TEMP_FAILURE_RETRY(pread(fd, data->data(), data->size(), 0));
  if (static_cast<size_t>(bytes_read) != data->size()) {
    PLOG(ERROR) << "Failed to read " << data->size() << " bytes data from "
                << path;
    return {};
  }
  return data;
}

LRUCache::~LRUCache() {
  std::error_code ec;
  std::filesystem::remove_all(tmpdir_, ec);
  if (ec) {
    LOG(ERROR) << "Failed to rm -rf " << tmpdir_ << " " << ec.message();
  }
}

bool LRUCache::WriteToDisk(const LRUCache::Key& key,
                           const LRUCache::Value& value) {
  if (tmpdir_.empty()) {
    return false;
  }
  const auto path = tmpdir_ / std::to_string(key);
  int fd = open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
  if (fd < 0) {
    PLOG(ERROR) << "Failed to open " << path;
    return false;
  }
  auto fd_delete = [](int* fd) { close(*fd); };
  std::unique_ptr<int, decltype(fd_delete)> guard(&fd, fd_delete);
  const auto written =
      TEMP_FAILURE_RETRY(write(fd, value->data(), value->size()));
  if (static_cast<size_t>(written) != value->size()) {
    PLOG(ERROR) << "Failed to write " << value->size() << " bytes data to "
                << path;
    return false;
  }
  close(fd);
  guard.release();
  ondisk_items_.insert(key);
  return true;
}

void LRUCache::put(const LRUCache::Key& key, const LRUCache::Value& value) {
  auto it = items_map_.find(key);
  if (it != items_map_.end()) {
    cache_size_ -= it->second->second->capacity();
    items_list_.erase(it->second);
    items_map_.erase(it);
  }
  EnsureSpace(value->capacity());
  cache_size_ += value->capacity();
  items_list_.push_front(key_value_pair_t(key, value));
  items_map_[key] = items_list_.begin();
}

bool LRUCache::EvictLRUItem() {
  if (items_list_.empty()) {
    return false;
  }
  const auto last = items_list_.back();
  cache_size_ -= last.second->capacity();
  // Only write puffs large enough to disk, as disk writes have latency.
  if (last.second->size() > 16 * 1024) {
    WriteToDisk(last.first, last.second);
  }
  items_map_.erase(last.first);
  items_list_.pop_back();
  return true;
}

void LRUCache::EnsureSpace(size_t size) {
  while (cache_size_ + size > max_size_) {
    if (!EvictLRUItem()) {
      return;
    }
  }
}

const char* GetTempDir() {
  const char* tmpdir = getenv("TMPDIR");
  if (tmpdir != nullptr) {
    return tmpdir;
  }
  return "/tmp";
}

LRUCache::LRUCache(size_t max_size) : max_size_(max_size) {
  std::error_code ec;
  auto buffer = GetTempDir() + std::string("/lru.XXXXXX");
  if (ec) {
    LOG(ERROR) << "Failed to get temp directory for LRU cache " << ec.message()
               << " " << ec.value();
    return;
  }
  const char* dirname = mkdtemp(buffer.data());
  if (dirname == nullptr) {
    LOG(ERROR) << "Failed to mkdtemp " << buffer;
    return;
  }

  tmpdir_ = dirname;
}

}  // namespace puffin
