#pragma once

#include <google/protobuf/io/zero_copy_stream.h>
#include <grpcpp/support/status.h>

#ifdef GRPCPP_PP_INCLUDE
#include <grpcpp/support/byte_buffer.h>
#include <grpcpp/support/slice.h>
#else
#include <grpc++/support/byte_buffer.h>
#include <grpc++/support/slice.h>
#endif

namespace grpc {
class ByteBufferZeroCopyInputStream : public google::protobuf::io::ZeroCopyInputStream {
 public:
  explicit ByteBufferZeroCopyInputStream(std::vector<Slice> slices);
  ByteBufferZeroCopyInputStream(const ByteBufferZeroCopyInputStream& other);
  void operator=(const ByteBufferZeroCopyInputStream& other);

  static Status Create(ByteBuffer* buffer,
                       std::unique_ptr<::grpc::ByteBufferZeroCopyInputStream>* out);

  bool Next(const void** data, int* size) override;
  void BackUp(int count) override;
  bool Skip(int count) override;
  int64_t ByteCount() const override;

 private:
  std::vector<Slice> slices_;
  size_t cur_slice_idx_;
  size_t bytes_read_in_cur_slice_;
  size_t last_returned_size_;
  int64_t total_bytes_read_;
};
}  // namespace grpc