// <3 nedwill 2019
#ifndef PIPE_H_
#define PIPE_H_

#include <memory>

class Pipe {
public:
  static std::unique_ptr<Pipe> Create(size_t buffer_size, uint64_t fd_ofiles);
  Pipe(size_t buffer_size, uint64_t fd_ofiles);
  ~Pipe();

  Pipe(const Pipe &pipe) = delete;

  bool Read();
  bool Write();

  void ClearBuffer();

  uint8_t *buffer() { return buffer_.get(); }

  int read_fd() { return read_fd_; }

  uint64_t const buffer_kaddr() { return buffer_kaddr_; }

  uint64_t const buffer_ptr_kaddr() { return buffer_ptr_kaddr_; }

  bool const Valid() { return valid_; }

private:
  Pipe(size_t buffer_size, int read_fd, int write_fd, uint64_t buffer_ptr_kaddr,
       uint64_t buffer_kaddr);

  // Whether the object is successfully constructed.
  bool valid_;

  // We have to write to the pipe before we can read from it.
  // This flag helps us keep track of that state.
  bool should_read_;

  int read_fd_;
  int write_fd_;

  std::unique_ptr<uint8_t[]> buffer_;
  size_t buffer_size_;

  // The pointer to the pointer so we can clear the pointer after freeing
  uint64_t buffer_ptr_kaddr_;

  // The original pointer to the kalloc'd buffer
  uint64_t buffer_kaddr_;
};

#endif /* PIPE_H_ */
