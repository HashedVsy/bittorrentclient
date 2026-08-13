#include "peer.h"
#include "dht.h"
#include <stdint.h>

static int wait_socket(SOCKET s, int writeable, int ms) {
    fd_set f; FD_ZERO(&f); FD_SET(s, &f);
    struct timeval tv = { ms / 1000, (ms % 1000) * 1000 };
    int r = select(0, writeable ? NULL : &f, writeable ? &f : NULL, NULL, &tv);
    return r > 0;
}
static int recv_all(SOCKET s, void *buf, size_t n) {
    size_t off=0; unsigned char *p=buf;
    while(off<n) { if(!wait_socket(s,0,15000)) return 0; int r=recv(s,(char*)p+off,(int)(n-off),0); if(r<=0)return 0; off+=(size_t)r; }
    return 1;
}
static int send_all(SOCKET s, const void *buf, size_t n) {
    size_t off=0; const unsigned char *p=buf;
    while(off<n) { if(!wait_socket(s,1,15000)) return 0; int r=send(s,(const char*)p+off,(int)(n-off),0); if(r<=0)return 0; off+=(size_t)r; }
    return 1;
}
static int msg(SOCKET s, uint8_t id, const void *payload, size_t n) {
    if(n+1>16*1024*1024)return 0; uint32_t l=htonl((uint32_t)n+1); unsigned char h[5]; memcpy(h,&l,4); h[4]=id;
    return send_all(s,h,5) && (!n || send_all(s,payload,n));
}
static int request_block(SOCKET s,uint32_t piece,uint32_t off,uint32_t len,unsigned char *dst) {
    unsigned char p[12]; uint32_t x;
    x=htonl(piece);memcpy(p,&x,4);x=htonl(off);memcpy(p+4,&x,4);x=htonl(len);memcpy(p+8,&x,4);
    if(!msg(s,6,p,12))return 0;
    for(;;){
        unsigned char h[4]; if(!recv_all(s,h,4))return 0; uint32_t ml=ntohl(*(uint32_t*)h);
        if(ml==0)continue; if(ml>16*1024*1024)return 0;
        unsigned char *m=malloc(ml);if(!m)return 0;if(!recv_all(s,m,ml)){free(m);return 0;}
        uint8_t id=m[0];
        if(id==0){free(m);continue;} /* choke: wait for the peer to unchoke */
        if(id==4 || id==5 || id==8 || id==9 || id==13 || id==14 || id==15 || id==16 || id==17){free(m);continue;}
        if(id==7 && ml>=9){uint32_t pi=ntohl(*(uint32_t*)(m+1)),po=ntohl(*(uint32_t*)(m+5));if(pi==piece&&po==off){size_t got=ml-9;if(got==len)memcpy(dst,m+9,len);free(m);return got==len;}}
        free(m);
    }
}

int peer_download_piece(const Torrent*t,const PeerAddress*p,const unsigned char id[20],size_t piece,unsigned char**out,size_t*n){
    *out=NULL;*n=0; if(piece>=torrent_piece_count(t))return 0;
    SOCKET s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(s==INVALID_SOCKET)return 0;
    struct sockaddr_in a={0};a.sin_family=AF_INET;a.sin_port=htons(p->port);if(inet_pton(AF_INET,p->ip,&a.sin_addr)!=1){closesocket(s);return 0;}
    if(!wait_socket(s,1,1)){/* connect is blocking below */}
    u_long nb=1;ioctlsocket(s,FIONBIO,&nb);connect(s,(struct sockaddr*)&a,sizeof(a));
    if(!wait_socket(s,1,10000)){closesocket(s);return 0;} int err=0;int el=sizeof(err);getsockopt(s,SOL_SOCKET,SO_ERROR,(char*)&err,&el);if(err){closesocket(s);return 0;}
    nb=0;ioctlsocket(s,FIONBIO,&nb);
    unsigned char h[68]={19};memcpy(h+1,"BitTorrent protocol",19);memcpy(h+28,t->info_hash,20);memcpy(h+48,id,20);
    if(!send_all(s,h,68)||!recv_all(s,h,68)||memcmp(h+28,t->info_hash,20)){closesocket(s);return 0;}
    /* We support the basic peer wire protocol plus the BEP 10 extension bit. */
    unsigned char ext[20]={0}; ext[5]=0x10; msg(s,20,ext,20);
    msg(s,2,NULL,0); /* interested */
    uint64_t z=torrent_piece_length(t,piece);unsigned char*d=malloc((size_t)z);if(!d){closesocket(s);return 0;}
    for(uint64_t off=0;off<z;off+=16384){uint32_t len=(uint32_t)((z-off)>16384?16384:z-off);if(!request_block(s,(uint32_t)piece,(uint32_t)off,len,d+off)){free(d);closesocket(s);return 0;}}
    closesocket(s);if(!torrent_verify_piece(t,piece,d,(size_t)z)){free(d);return 0;}*out=d;*n=(size_t)z;return 1;
}
