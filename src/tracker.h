#ifndef BT_TRACKER_H
#define BT_TRACKER_H
#include "torrent.h"
size_t tracker_get_peers(const Torrent*,const unsigned char[20],PeerAddress*,size_t);
#endif
