#include "lpd.h"
#include <iphlpapi.h>
#pragma comment(lib,"iphlpapi.lib")

int lpd_announce(const char *infohash_hex,const char *peer_name,uint16_t port){
    if(!infohash_hex||strlen(infohash_hex)!=40||!peer_name)return 0;
    SOCKET s=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);if(s==INVALID_SOCKET)return 0;
    BOOL yes=TRUE;setsockopt(s,SOL_SOCKET,SO_BROADCAST,(const char*)&yes,sizeof(yes));
    struct sockaddr_in a;memset(&a,0,sizeof(a));a.sin_family=AF_INET;a.sin_port=htons(6771);a.sin_addr.s_addr=inet_addr("239.192.152.143");
    char body[2048];int n=_snprintf_s(body,sizeof(body),_TRUNCATE,"BT-SEARCH * HTTP/1.1\r\nHost: 239.192.152.143:6771\r\nPort: %u\r\nInfohash: %s\r\nN: %s\r\n\r\n",(unsigned)port,infohash_hex,peer_name);
    int ok=n>0&&sendto(s,body,n,0,(struct sockaddr*)&a,sizeof(a))==n;closesocket(s);return ok;
}
