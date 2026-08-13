#ifndef BT_PEER_PROTOCOL_H
#define BT_PEER_PROTOCOL_H
#include "common.h"

enum { BT_CHOKE=0, BT_UNCHOKE=1, BT_INTERESTED=2, BT_NOT_INTERESTED=3, BT_HAVE=4, BT_BITFIELD=5, BT_REQUEST=6, BT_PIECE=7, BT_CANCEL=8, BT_PORT=9, BT_SUGGEST=13, BT_HAVE_ALL=14, BT_HAVE_NONE=15, BT_REJECT_REQUEST=16, BT_ALLOWED_FAST=17, BT_EXTENDED=20 };
size_t bt_msg(uint8_t id,const void *payload,size_t n,unsigned char *out,size_t cap);
size_t bt_request(uint32_t piece,uint32_t begin,uint32_t length,unsigned char *out,size_t cap);
size_t bt_cancel(uint32_t piece,uint32_t begin,uint32_t length,unsigned char *out,size_t cap);
size_t bt_have(uint32_t piece,unsigned char *out,size_t cap);
size_t bt_port(uint16_t port,unsigned char *out,size_t cap);
int bt_parse_header(const unsigned char *p,size_t n,uint32_t *length,uint8_t *id);
#endif
