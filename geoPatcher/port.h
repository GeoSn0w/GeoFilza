// <3 nedwill 2019
#ifndef PORT_H_
#define PORT_H_

#include <mach/mach.h>

class Port {
public:
  Port();
  ~Port();

  mach_port_t get() const { return port_; }

protected:
  mach_port_t port_;
};

class OOLHoldingPort : public Port {
public:
  OOLHoldingPort();
  ~OOLHoldingPort();
};

#endif /* PORT_H_ */
