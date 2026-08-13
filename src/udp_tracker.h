#ifndef BT_UDP_TRACKER_H
#define BT_UDP_TRACKER_H
#include "common.h"
int udp_tracker_announce(const char *url,const unsigned char info_hash[20],const unsigned char peer_id[20],uint16_t port,uint32_t downloaded,uint32_t left,uint32_t uploaded,PeerAddress *peers,size_t cap,size_t *count);
#endif
