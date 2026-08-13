#ifndef BT_DHT_H
#define BT_DHT_H
#include "torrent.h"
size_t dht_get_peers(const Torrent*,PeerAddress*,size_t);
#endif
