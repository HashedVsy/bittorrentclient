#ifndef BT_PEER_H
#define BT_PEER_H
#include "torrent.h"
int peer_download_piece(const Torrent*,const PeerAddress*,const unsigned char[20],size_t,unsigned char**,size_t*);
#endif
