#ifndef BT_WEBSEED_H
#define BT_WEBSEED_H
#include "torrent.h"
int webseed_download_piece(const Torrent*,const char*,size_t,unsigned char**,size_t*);
#endif
