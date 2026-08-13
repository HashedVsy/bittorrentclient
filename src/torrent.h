#ifndef BT_TORRENT_H
#define BT_TORRENT_H
#include "bencode.h"
#include "common.h"
typedef struct {char*name;unsigned char info_hash[20];uint32_t piece_length;unsigned char*pieces;size_t pieces_length;uint64_t total_length;char**trackers;size_t tracker_count;char**webseeds;size_t webseed_count;BValue*root;BValue*info;} Torrent;
int torrent_load(const char*,Torrent*);void torrent_free(Torrent*);size_t torrent_piece_count(const Torrent*);uint64_t torrent_piece_length(const Torrent*,size_t);int torrent_verify_piece(const Torrent*,size_t,const unsigned char*,size_t);
#endif
