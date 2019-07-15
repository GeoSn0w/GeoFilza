// <3 nedwill 2019
#include "util.h"

bool LooksLikeKaddr(uint64_t addr) {
  if ((addr >> 32) == 0xffffffe0) {
    return true;
  }

  return false;
}
