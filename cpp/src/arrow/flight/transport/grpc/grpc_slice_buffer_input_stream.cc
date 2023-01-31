#include "grpc_slice_buffer_input_stream.h"
#include <grpc/slice.h>
#include <grpcpp/support/slice.h>
#include <cassert>
#include <cstddef>
#include <memory>
#include <numeric>

namespace grpc {
ByteBufferZeroCopyInputStream::ByteBufferZeroCopyInputStream(std::vector<Slice> slices)
    : slices_(std::move(slices)),
      cur_slice_idx_(0),
      bytes_read_in_cur_slice_(0),
      last_returned_size_(0),
      total_bytes_read_(0) {}

ByteBufferZeroCopyInputStream::ByteBufferZeroCopyInputStream(ByteBuffer* buffer)
    : cur_slice_idx_(0),
      bytes_read_in_cur_slice_(0),
      last_returned_size_(0),
      total_bytes_read_(0) {
  assert(buffer->Dump(&slices_).ok());
}

bool ByteBufferZeroCopyInputStream::Next(const void** data, int* size) {
  if (cur_slice_idx_ == slices_.size()) {
    return false;
  }

  const Slice& slice = slices_[cur_slice_idx_];
  *data = slice.begin() + bytes_read_in_cur_slice_;
  *size = static_cast<int>(slice.size() - bytes_read_in_cur_slice_);

  total_bytes_read_ += *size;
  last_returned_size_ = *size;
  cur_slice_idx_++;
  bytes_read_in_cur_slice_ = 0;
  return true;
}

void ByteBufferZeroCopyInputStream::BackUp(int count) {
  // Next() should be called before backup
  assert(last_returned_size_ > 0);
  assert(static_cast<size_t>(count) <= last_returned_size_);

  total_bytes_read_ -= count;
  cur_slice_idx_--;
  const Slice& last_returned_slice = slices_[cur_slice_idx_];
  bytes_read_in_cur_slice_ = last_returned_slice.size() - count;
  last_returned_size_ = 0;  // Don't let caller back up further.
}

bool ByteBufferZeroCopyInputStream::Skip(int count) {
  assert(count > 0);
  last_returned_size_ = 0;  // Don't let caller back up further.
  if (cur_slice_idx_ == slices_.size()) {
    return false;
  }

  size_t remaining = count;
  while (cur_slice_idx_ < slices_.size() && remaining > 0) {
    size_t bytes_skipped = std::min(remaining, slices_[cur_slice_idx_].size());
    total_bytes_read_ += bytes_skipped;
    remaining -= bytes_skipped;
    bytes_read_in_cur_slice_ = bytes_skipped;
    cur_slice_idx_ += (bytes_read_in_cur_slice_ == slices_[cur_slice_idx_].size());
  }

  return cur_slice_idx_ < slices_.size();
}

int64_t ByteBufferZeroCopyInputStream::ByteCount() const { return total_bytes_read_; }

int64_t ByteBufferZeroCopyInputStream::Length() const {
  return std::accumulate(
      slices_.begin(), slices_.end(), 0,
      [](const int64_t& acc, const Slice& slice) { return acc + slice.size(); });
}
}  // namespace grpc
