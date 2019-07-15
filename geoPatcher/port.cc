// <3 nedwill 2019
#include "port.h"

extern "C" {
#include "kernel_alloc.h"
}

#include <stdio.h>

Port::Port() {
  mach_port_options_t options = {};
  port_ = MACH_PORT_NULL;
  kern_return_t kr = mach_port_construct(mach_task_self(), &options, 0, &port_);
  if (kr != KERN_SUCCESS) {
    printf("Port::Port: failed to make port\n");
  }
}

Port::~Port() {
  kern_return_t kr = mach_port_destroy(mach_task_self(), port_);
  if (kr != KERN_SUCCESS) {
    printf("Port::~Port: returned %d: %s\n", kr, mach_error_string(kr));
  }
  port_ = MACH_PORT_DEAD;
}

OOLHoldingPort::OOLHoldingPort() { port_increase_queue_limit(port_); }

OOLHoldingPort::~OOLHoldingPort() {}
