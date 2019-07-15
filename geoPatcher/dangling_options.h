// <3 nedwill 2019
#ifndef DANGLING_OPTIONS_H_
#define DANGLING_OPTIONS_H_

#include <netinet/in.h>
#include <netinet/ip6.h>

#define IPV6_USE_MIN_MTU 42 /* bool; send packets at the minimum MTU */

#define IPV6_3542PKTINFO 46  /* in6_pktinfo; send if, src addr */
#define IPV6_3542HOPLIMIT 47 /* int; send hop limit */
#define IPV6_3542NEXTHOP 48  /* sockaddr; next hop addr */
#define IPV6_3542HOPOPTS 49  /* ip6_hbh; send hop-by-hop option */
#define IPV6_3542DSTOPTS 50  /* ip6_dest; send dst option befor rthdr */
#define IPV6_3542RTHDR 51    /* ip6_rthdr; send routing header */

#define IPV6_PREFER_TEMPADDR 63

#define IPV6_PKTINFO IPV6_3542PKTINFO
#define IPV6_HOPLIMIT IPV6_3542HOPLIMIT
#define IPV6_NEXTHOP IPV6_3542NEXTHOP
#define IPV6_HOPOPTS IPV6_3542HOPOPTS
#define IPV6_DSTOPTS IPV6_3542DSTOPTS
#define IPV6_RTHDR IPV6_3542RTHDR

class DanglingOptions {
public:
  DanglingOptions();
  ~DanglingOptions();

  // The following getters/setters on the ipv6 options
  // are based on ip6_setpktopt and ip6_getpcbopt.

  // IPV6_PKTINFO
  bool GetPktinfo(struct in6_pktinfo *pktinfo);
  bool SetPktinfo(struct in6_pktinfo *pktinfo);

  // IPV6_RTHDR
  bool GetRthdr(struct ip6_rthdr *rthdr);
  bool SetRthdr(struct ip6_rthdr *rthdr);

  // IPV6_HOPOPTS, IPV6_DSTOPTS, IPV6_RTHDRDSTOPTS, IPV6_NEXTHOP must be root

  // IPV6_TCLASS: int from -1 to 255 inclusive
  bool GetTclass(int *tclass);
  bool SetTclass(int *tclass);

  // IPV6_DONTFRAG: int, used as bool

  // IPV6_USE_MIN_MTU: int in {-1, 0, 1}
  bool GetMinmtu(int *minmtu);
  bool SetMinmtu(int *minmtu);

  // IPV6_PREFER_TEMPADDR: int in {-1. 0, 1}
  bool GetPreferTempaddr(int *prefer_tempaddr);
  bool SetPreferTempaddr(int *prefer_tempaddr);

  int GetFd() { return s_; }

private:
  bool SetIPv6Opt(int option_name, void *data, socklen_t size);
  bool GetIPv6Opt(int option_name, void *data, socklen_t size);

  // Free the ipv6 socket options. Subsequent
  // getter/setter calls will be UaFs. Can only
  // be called once, calling repeatedly is a nop.
  bool FreeOptions();

  int s_;
  bool dangling_;
};

#endif /* DANGLING_OPTIONS_H_ */
