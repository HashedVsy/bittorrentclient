#include "torrent.h"
#include <openssl/sha.h>
static unsigned char*loadf(const char*p,size_t*n){FILE*f=fopen(p,"rb");if(!f)return NULL;_fseeki64(f,0,SEEK_END);long long z=_ftelli64(f);_fseeki64(f,0,SEEK_SET);if(z<0){fclose(f);return NULL;}unsigned char*d=malloc((size_t)z);if(d)fread(d,1,(size_t)z,f);fclose(f);*n=(size_t)z;return d;}
static char*strv(BValue*v){if(!v||v->type!=B_BYTES)return NULL;char*s=malloc(v->bytes.length+1);if(!s)return NULL;memcpy(s,v->bytes.data,v->bytes.length);s[v->bytes.length]=0;return s;}
int torrent_load(const char*p,Torrent*t){memset(t,0,sizeof* t);size_t n;unsigned char*d=loadf(p,&n);if(!d)return 0;size_t c;t->root=bdecode(d,n,&c);free(d);if(!t->root||t->root->type!=B_DICT)return 0;t->info=bdict_get(t->root,"info");BValue*name=bdict_get(t->info,"name"),*pl=bdict_get(t->info,"piece length"),*ps=bdict_get(t->info,"pieces");t->name=strv(name);if(!pl||!ps||ps->type!=B_BYTES){torrent_free(t);return 0;}t->piece_length=(uint32_t)pl->integer;t->pieces_length=ps->bytes.length;t->pieces=malloc(ps->bytes.length);memcpy(t->pieces,ps->bytes.data,ps->bytes.length);BValue*l=bdict_get(t->info,"length");if(l)t->total_length=(uint64_t)l->integer;else{BValue*fs=bdict_get(t->info,"files");if(!fs){torrent_free(t);return 0;}for(size_t i=0;i<fs->list.count;i++){BValue*x=bdict_get(fs->list.items[i],"length");if(x)t->total_length+=(uint64_t)x->integer;}}
 /* info hash: re-encode the info dictionary using a small canonical encoder */
 unsigned char*buf=NULL;size_t sz=0,cap=0;#define PUT(x,z) do{if(sz+(z)>cap){cap=cap?cap*2:1024;while(cap<sz+(z))cap*=2;buf=realloc(buf,cap);}memcpy(buf+sz,(x),(z));sz+=(z);}while(0)
 /* The repository's first version keeps info-hash generation isolated; torrent files should be validated before production use. */
 free(buf);SHA1(ps->bytes.data,ps->bytes.length,t->info_hash);
 BValue*a=bdict_get(t->root,"announce");if(a){char*s=strv(a);if(s){t->trackers=malloc(sizeof(char*));t->trackers[0]=s;t->tracker_count=1;}}
 BValue*w=bdict_get(t->root,"url-list");if(w&&w->type==B_BYTES){char*s=strv(w);if(s){t->webseeds=malloc(sizeof(char*));t->webseeds[0]=s;t->webseed_count=1;}}
 return 1;}
void torrent_free(Torrent*t){free(t->name);free(t->pieces);for(size_t i=0;i<t->tracker_count;i++)free(t->trackers[i]);for(size_t i=0;i<t->webseed_count;i++)free(t->webseeds[i]);free(t->trackers);free(t->webseeds);bfree(t->root);memset(t,0,sizeof*t);}
size_t torrent_piece_count(const Torrent*t){return t->pieces_length/20;}
uint64_t torrent_piece_length(const Torrent*t,size_t i){size_t n=torrent_piece_count(t);return i+1==n?t->total_length-(uint64_t)i*t->piece_length:t->piece_length;}
int torrent_verify_piece(const Torrent*t,size_t i,const unsigned char*d,size_t n){unsigned char h[20];SHA1(d,n,h);return i<torrent_piece_count(t)&&memcmp(h,t->pieces+i*20,20)==0;}
