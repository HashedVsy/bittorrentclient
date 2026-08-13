#include "dht.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <time.h>

#define DHT_ID_LEN 20
#define TXN_LEN 2
#define MAX_PACKET 4096
#define MAX_RESULTS 512

typedef struct { unsigned char id[20]; struct sockaddr_in addr; unsigned long long last_seen; int good; } Node;
typedef struct { Node n[DHT_K]; size_t count; } Bucket;
typedef struct { unsigned char *p; size_t n, cap; } Buf;
typedef struct { unsigned char tx[2]; size_t node_index; int active; } Pending;

static int put(Buf*b,const void*p,size_t n){if(b->n+n>b->cap){size_t c=b->cap?b->cap*2:512;while(c<b->n+n)c*=2;void*q=realloc(b->p,c);if(!q)return 0;b->p=q;b->cap=c;}memcpy(b->p+b->n,p,n);b->n+=n;return 1;}
static int bs(Buf*b,const void*p,size_t n){char x[32];int m=snprintf(x,sizeof x,"%zu:",n);return m>0&&put(b,x,(size_t)m)&&put(b,p,n);}
static int be(Buf*b,const char*s){return put(b,s,strlen(s));}
static void random_id(unsigned char id[20]){static int seeded; if(!seeded){seeded=1;srand((unsigned)time(NULL)^GetCurrentProcessId());}for(int i=0;i<20;i++)id[i]=(unsigned char)(rand()&255);}
static void txid(unsigned char t[2]){t[0]=(unsigned char)(rand()&255);t[1]=(unsigned char)(rand()&255);}
static int cmpdist(const unsigned char a[20],const unsigned char b[20],const unsigned char target[20]){for(int i=0;i<20;i++){unsigned char x=a[i]^target[i],y=b[i]^target[i];if(x<y)return -1;if(x>y)return 1;}return 0;}
static int sameaddr(const struct sockaddr_in*a,const struct sockaddr_in*b){return a->sin_addr.s_addr==b->sin_addr.s_addr&&a->sin_port==b->sin_port;}
static int bucket_index(const unsigned char self[20],const unsigned char id[20]){for(int i=0;i<160;i++){int bit=i;unsigned char x=self[bit/8]^id[bit/8];if(x&(0x80>>(bit%8)))return 159-i;}return 0;}
static void bucket_add(Bucket*b,const Node*n){for(size_t i=0;i<b->count;i++)if(sameaddr(&b->n[i].addr,&n->addr)){b->n[i]=*n;return;}if(b->count<DHT_K){b->n[b->count++]=*n;return;}memmove(&b->n[0],&b->n[1],(DHT_K-1)*sizeof(Node));b->n[DHT_K-1]=*n;}
static void table_add(Bucket buckets[160],const unsigned char self[20],const Node*n){if(!n->good||memcmp(n->id,self,20)==0)return;bucket_add(&buckets[bucket_index(self,n->id)],n);}
static int query(Buf*b,const unsigned char id[20],const char*m,const unsigned char target[20],const unsigned char tx[2],uint16_t port){
    if(!be(b,"d1:ad2:id20:")||!put(b,id,20))return 0;
    if(strcmp(m,"get_peers")==0){if(!be(b,"9:info_hash20:")||!put(b,target,20))return 0;}
    else if(strcmp(m,"find_node")==0){if(!be(b,"6:target20:")||!put(b,target,20))return 0;}
    else if(strcmp(m,"announce_peer")==0){if(!be(b,"8:info_hash20:")||!put(b,target,20)||!be(b,"5:porti")||!put(b,&port,0))return 0;}
    else return 0;
    if(strcmp(m,"announce_peer")!=0){};
    /* Rebuild announce_peer separately because its integer fields are network-independent. */
    return be(b,"e1:q")&&bs(b,m,strlen(m))&&be(b,"1:t2:")&&put(b,tx,2)&&be(b,"1:y1:qe");
}
static int make_get(Buf*b,const unsigned char id[20],const char*m,const unsigned char target[20],const unsigned char tx[2]){
    if(!be(b,"d1:ad2:id20:")||!put(b,id,20))return 0;
    if(strcmp(m,"get_peers")==0){if(!be(b,"9:info_hash20:")||!put(b,target,20))return 0;}
    else {if(!be(b,"6:target20:")||!put(b,target,20))return 0;}
    return be(b,"e1:q")&&bs(b,m,strlen(m))&&be(b,"1:t2:")&&put(b,tx,2)&&be(b,"1:y1:qe");
}
static int make_announce(Buf*b,const unsigned char id[20],const unsigned char info[20],const unsigned char tx[2],uint16_t port){
    char ip[64];snprintf(ip,sizeof ip,"%u",(unsigned)port);
    return be(b,"d1:ad2:id20:")&&put(b,id,20)&&be(b,"8:info_hash20:")&&put(b,info,20)&&be(b,"7:peer id20:")&&put(b,id,20)&&be(b,"4:porti")&&be(b,ip)&&be(b,"e")&&be(b,"1:q13:announce_peer1:t2:")&&put(b,tx,2)&&be(b,"1:y1:qe");
}
static int send_buf(SOCKET s,const struct sockaddr_in*a,Buf*b){return sendto(s,(const char*)b->p,(int)b->n,0,(const struct sockaddr*)a,sizeof*a)==(int)b->n;}
static int add_compact(BValue*v,Node*nodes,size_t*count){if(!v||v->type!=B_BYTES)return 0;for(size_t i=0;i+26<=v->bytes.length&&*count<DHT_MAX_NODES;i+=26){Node n={0};memcpy(n.id,v->bytes.data+i,20);n.addr.sin_family=AF_INET;memcpy(&n.addr.sin_addr.s_addr,v->bytes.data+i+20,4);n.addr.sin_port=htons((uint16_t)(((uint16_t)v->bytes.data[i+24]<<8)|v->bytes.data[i+25]));n.good=1;if(!n.addr.sin_port)continue;int dup=0;for(size_t j=0;j<*count;j++)if(sameaddr(&nodes[j].addr,&n.addr)){dup=1;break;}if(!dup)nodes[(*count)++]=n;}return 1;}
static void add_values(BValue*v,PeerAddress*out,size_t*count,size_t max){if(!v||v->type!=B_LIST)return;for(size_t i=0;i<v->list.count&&*count<max;i++){BValue*x=v->list.items[i];if(!x||x->type!=B_BYTES||x->bytes.length!=6)continue;struct in_addr a;memcpy(&a.s_addr,x->bytes.data,4);char ip[INET_ADDRSTRLEN];if(!inet_ntop(AF_INET,&a,ip,sizeof ip))continue;uint16_t p=(uint16_t)(((uint16_t)x->bytes.data[4]<<8)|x->bytes.data[5]);if(!p)continue;int dup=0;for(size_t j=0;j<*count;j++)if(out[j].port==p&&!strcmp(out[j].ip,ip)){dup=1;break;}if(!dup){strncpy(out[*count].ip,ip,sizeof out[*count].ip-1);out[*count].ip[sizeof out[*count].ip-1]=0;out[*count].port=p;(*count)++;}}}
static int valid_response(BValue*r,const unsigned char tx[2]){BValue*t=bdict_get(r,"t");BValue*y=bdict_get(r,"y");return t&&y&&t->type==B_BYTES&&t->bytes.length==2&&!memcmp(t->bytes.data,tx,2)&&y->type==B_BYTES&&y->bytes.length==1&&y->bytes.data[0]=='r';}
static void save_nodes(const Bucket buckets[160],const char*file){FILE*f=fopen(file,"wb");if(!f)return;uint32_t magic=0x31485444;fwrite(&magic,4,1,f);for(int b=0;b<160;b++)for(size_t i=0;i<buckets[b].count;i++){Node*n=(Node*)&buckets[b].n[i];fwrite(n->id,1,20,f);fwrite(&n->addr.sin_addr.s_addr,4,1,f);fwrite(&n->addr.sin_port,2,1,f);}fclose(f);}
static void load_nodes(Bucket buckets[160],const unsigned char self[20],const char*file){FILE*f=fopen(file,"rb");if(!f)return;uint32_t magic;if(fread(&magic,4,1,f)!=1||magic!=0x31485444){fclose(f);return;}Node n;while(fread(n.id,1,20,f)==20&&fread(&n.addr.sin_addr.s_addr,4,1,f)==1&&fread(&n.addr.sin_port,2,1,f)==1){n.addr.sin_family=AF_INET;n.good=1;table_add(buckets,self,&n);}fclose(f);}
static void bootstrap(SOCKET s,Bucket buckets[160],const unsigned char self[20],Node*seed,size_t*sc){const char*hosts[]={"router.bittorrent.com","router.utorrent.com","dht.transmissionbt.com"};for(size_t i=0;i<3&&*sc<DHT_MAX_NODES;i++){struct addrinfo h={0},*r=NULL;h.ai_family=AF_INET;h.ai_socktype=SOCK_DGRAM;if(getaddrinfo(hosts[i],"6881",&h,&r)==0&&r){Node n={0};n.addr=*(struct sockaddr_in*)r->ai_addr;n.good=1;seed[(*sc)++]=n;freeaddrinfo(r);}}(void)s;(void)buckets;(void)self;}

size_t dht_get_peers(const Torrent*t,PeerAddress*peers,size_t max_peers){if(!t||!peers||!max_peers)return 0;SOCKET s=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);if(s==INVALID_SOCKET)return 0;struct sockaddr_in local={0};local.sin_family=AF_INET;local.sin_addr.s_addr=htonl(INADDR_ANY);local.sin_port=htons(DHT_PORT);if(bind(s,(struct sockaddr*)&local,sizeof local)!=0){local.sin_port=0;if(bind(s,(struct sockaddr*)&local,sizeof local)!=0){closesocket(s);return 0;}}
    unsigned char self[20];random_id(self);Bucket buckets[160]={0};load_nodes(buckets,self,DHT_NODE_FILE);Node queue[DHT_MAX_NODES];size_t qn=0;bootstrap(s,buckets,self,queue,&qn);
    /* Add persisted routing-table nodes to the lookup frontier. */
    for(int b=0;b<160&&qn<DHT_MAX_NODES;b++)for(size_t i=0;i<buckets[b].count&&qn<DHT_MAX_NODES;i++)queue[qn++]=buckets[b].n[i];
    size_t found=0,rounds=0;
    while(rounds++<64&&found<max_peers&&qn){
        Node next[DHT_ALPHA];size_t nn=0;int used[DHT_MAX_NODES]={0};
        /* Pick the closest known nodes to the info hash. */
        for(int a=0;a<DHT_ALPHA;a++){size_t best=(size_t)-1;for(size_t i=0;i<qn;i++)if(!used[i]&&(best==(size_t)-1||cmpdist(queue[i].id,queue[best].id,t->info_hash)<0))best=i;if(best==(size_t)-1)break;used[best]=1;next[nn++]=queue[best];}
        if(!nn)break;
        Pending pend[DHT_ALPHA]={0};
        for(size_t i=0;i<nn;i++){Buf b={0};txid(pend[i].tx);pend[i].active=make_get(&b,self,"get_peers",t->info_hash,pend[i].tx)&&send_buf(s,&next[i].addr,&b);free(b.p);}
        int pending=0;for(size_t i=0;i<nn;i++)pending+=pend[i].active;
        while(pending){fd_set set;FD_ZERO(&set);FD_SET(s,&set);struct timeval tv={DHT_TIMEOUT_MS/1000,(DHT_TIMEOUT_MS%1000)*1000};if(select(0,&set,NULL,NULL,&tv)<=0)break;unsigned char pkt[MAX_PACKET];struct sockaddr_in from;int fl=sizeof from;int z=recvfrom(s,(char*)pkt,sizeof pkt,0,(struct sockaddr*)&from,&fl);if(z<=0)continue;size_t usedbytes=0;BValue*r=bdecode(pkt,(size_t)z,&usedbytes);if(!r){continue;}int hit=-1;for(size_t i=0;i<nn;i++)if(pend[i].active&&valid_response(r,pend[i].tx)){hit=(int)i;break;}if(hit>=0){pend[hit].active=0;pending--;BValue*rd=bdict_get(r,"r");if(rd&&rd->type==B_DICT){BValue*id=bdict_get(rd,"id");if(id&&id->type==B_BYTES&&id->bytes.length==20){next[hit].good=1;memcpy(next[hit].id,id->bytes.data,20);table_add(buckets,self,&next[hit]);}size_t before=found;add_values(bdict_get(rd,"values"),peers,&found,max_peers);if(found==before){Node add[DHT_MAX_NODES];size_t ac=0;add_compact(bdict_get(rd,"nodes"),add,&ac);for(size_t k=0;k<ac&&qn<DHT_MAX_NODES;k++){int dup=0;for(size_t j=0;j<qn;j++)if(sameaddr(&queue[j].addr,&add[k].addr)){dup=1;break;}if(!dup){queue[qn++]=add[k];table_add(buckets,self,&add[k]);}}}}}bfree(r);}
    }
    /* Announce to a few responsive nodes so future lookups can discover this client. */
    uint16_t announce_port=BT_PORT;for(int b=0;b<160;b++)for(size_t i=0;i<buckets[b].count;i++){if(!buckets[b].n[i].good)continue;unsigned char tx[2];txid(tx);Buf x={0};if(make_announce(&x,self,t->info_hash,tx,announce_port)){send_buf(s,&buckets[b].n[i].addr,&x);}free(x.p);if(--announce_port==0)announce_port=BT_PORT;}
    save_nodes(buckets,DHT_NODE_FILE);closesocket(s);return found;
}
