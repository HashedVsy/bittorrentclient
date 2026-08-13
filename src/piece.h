#ifndef BT_PIECE_H
#define BT_PIECE_H
#include "torrent.h"
int piece_is_complete(const char*,const Torrent*,size_t);int piece_write(const char*,const Torrent*,size_t,const unsigned char*,size_t);
#endif
