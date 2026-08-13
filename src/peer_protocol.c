#include "peer_protocol.h"
static void be16(unsigned char*p,uint16_t x){p[0]=(unsigned char)(x>>8);p[1]=(unsigned char)x;}
static void be32(unsigned char*p,uint32_t x){p[0]=(unsigned char)(x>>24);p[1]=(unsigned char)(x>>16);p[2]=(unsigned char)(x>>8);p[3]=(unsigned char)x;}
size_t bt_msg(uint8_t id,const void *payload,size_t n,unsigned char *out,size_t cap){if(n>0xffffffffu||cap<n+5)return 0;be32(out,(uint32_t)n+1);out[4]=id;if(n)memcpy(out+5,payload,n);return n+5;}
size_t bt_request(uint32_t p,uint32_t b,uint32_t l,unsigned char*out,size_t c){unsigned char x[12];be32(x,p);be32(x+4,b);be32(x+8,l);return bt_msg(BT_REQUEST,x,12,out,c);}
size_t bt_cancel(uint32_t p,uint32_t b,uint32_t l,unsigned char*out,size_t c){unsigned char x[12];be32(x,p);be32(x+4,b);be32(x+8,l);return bt_msg(BT_CANCEL,x,12,out,c);}
size_t bt_have(uint32_t p,unsigned char*out,size_t c){unsigned char x[4];be32(x,p);return bt_msg(BT_HAVE,x,4,out,c);}
size_t bt_port(uint16_t port,unsigned char*out,size_t c){unsigned char x[2];be16(x,port);return bt_msg(BT_PORT,x,2,out,c);}
int bt_parse_header(const unsigned char*p,size_t n,uint32_t*l,uint8_t*id){if(n<4)return 0;*l=((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];if(*l==0){*id=255;return 1;}if(*l<1||n<5)return 0;*id=p[4];return 1;}
