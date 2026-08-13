#include "peer.h"

static int io(SOCKET s,void*b,size_t n,int sendit){size_t x=0;while(x<n){int r=sendit?send(s,(char*)b+x,(int)(n-x),0):recv(s,(char*)b+x,(int)(n-x),0);if(r<=0)return 0;x+=r;}return 1;}
static int send_port(SOCKET s,uint16_t port){unsigned char m[7];uint32_t n=htonl(3);memcpy(m,&n,4);m[4]=9;uint16_t p=htons(port);memcpy(m+5,&p,2);return io(s,m,sizeof m,1);}

int peer_download_piece(const Torrent*t,const PeerAddress*p,const unsigned char id[20],size_t i,unsigned char**out,size_t*n){
    SOCKET s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(s==INVALID_SOCKET)return 0;
    struct sockaddr_in a={0};a.sin_family=AF_INET;a.sin_port=htons(p->port);if(inet_pton(AF_INET,p->ip,&a.sin_addr)!=1||connect(s,(struct sockaddr*)&a,sizeof a)){closesocket(s);return 0;}
    unsigned char h[68]={19};memcpy(h+1,"BitTorrent protocol",19);memcpy(h+28,t->info_hash,20);memcpy(h+48,id,20);
    if(!io(s,h,68,1)||!io(s,h,68,0)||memcmp(h+28,t->info_hash,20)){closesocket(s);return 0;}
    /* BEP 5 PORT message: tell the peer which UDP port our DHT listener uses. */
    send_port(s,DHT_PORT);
    unsigned char in[5]={0,0,0,1,2};if(!io(s,in,5,1)){closesocket(s);return 0;}
    size_t z=(size_t)torrent_piece_length(t,i);unsigned char*d=malloc(z);if(!d){closesocket(s);return 0;}
    for(size_t off=0;off<z;){uint32_t bl=(uint32_t)((z-off)>BLOCK_SIZE?BLOCK_SIZE:z-off);unsigned char q[17];uint32_t x=htonl(13);memcpy(q,&x,4);q[4]=6;x=htonl((uint32_t)i);memcpy(q+5,&x,4);x=htonl((uint32_t)off);memcpy(q+9,&x,4);x=htonl(bl);memcpy(q+13,&x,4);if(!io(s,q,17,1))break;unsigned char mh[4];if(!io(s,mh,4,0))break;uint32_t ml=ntohl(*(uint32_t*)mh);if(ml<9||ml>BLOCK_SIZE+9)break;unsigned char*m=malloc(ml);if(!m||!io(s,m,ml,0)){free(m);break;}uint32_t pi=ntohl(*(uint32_t*)(m+1)),po=ntohl(*(uint32_t*)(m+5));if(m[0]==7&&pi==i&&po==off){memcpy(d+off,m+9,ml-9);off+=ml-9;}free(m);}
    closesocket(s);if(!torrent_verify_piece(t,i,d,z)){free(d);return 0;}*out=d;*n=z;return 1;
}
