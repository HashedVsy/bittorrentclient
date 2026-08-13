#include "bencode.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
static BValue *val(BType t){BValue*v=calloc(1,sizeof(*v));if(v)v->type=t;return v;}
static BValue *parse(const unsigned char*d,size_t n,size_t*p){
 if(*p>=n)return NULL; unsigned char c=d[*p];
 if(c=='i'){(*p)++;size_t s=*p;while(*p<n&&d[*p]!='e')(*p)++;if(*p>=n)return NULL;char b[64];size_t z=*p-s;if(!z||z>=sizeof b)return NULL;memcpy(b,d+s,z);b[z]=0;BValue*v=val(B_INT);if(!v)return NULL;v->integer=strtoll(b,NULL,10);(*p)++;return v;}
 if(c=='l'||c=='d'){BType t=c=='l'?B_LIST:B_DICT;(*p)++;BValue*v=val(t);if(!v)return NULL;while(*p<n&&d[*p]!='e'){BValue*k=parse(d,n,p);if(!k){bfree(v);return NULL;}if(t==B_LIST){BValue**x=realloc(v->list.items,(v->list.count+1)*sizeof*x);if(!x){bfree(k);bfree(v);return NULL;}v->list.items=x;v->list.items[v->list.count++]=k;}else{if(k->type!=B_BYTES){bfree(k);bfree(v);return NULL;}char*key=malloc(k->bytes.length+1);if(!key){bfree(k);bfree(v);return NULL;}memcpy(key,k->bytes.data,k->bytes.length);key[k->bytes.length]=0;bfree(k);BValue*x=parse(d,n,p);if(!x){free(key);bfree(v);return NULL;}char**ks=realloc(v->dict.keys,(v->dict.count+1)*sizeof*ks);BValue**vs=realloc(v->dict.values,(v->dict.count+1)*sizeof*vs);if(!ks||!vs){free(key);bfree(x);free(ks);free(vs);bfree(v);return NULL;}v->dict.keys=ks;v->dict.values=vs;v->dict.keys[v->dict.count]=key;v->dict.values[v->dict.count++]=x;}}
 if(*p>=n||d[*p]!='e'){bfree(v);return NULL;}(*p)++;return v;}
 if(isdigit(c)){size_t s=*p;while(*p<n&&isdigit(d[*p]))(*p)++;if(*p>=n||d[*p]!=':')return NULL;char b[32];size_t z=*p-s;if(!z||z>=sizeof b)return NULL;memcpy(b,d+s,z);b[z]=0;size_t len=(size_t)strtoull(b,NULL,10);(*p)++;if(len>n-*p)return NULL;BValue*v=val(B_BYTES);if(!v)return NULL;v->bytes.data=malloc(len);if(len&&!v->bytes.data){free(v);return NULL;}memcpy(v->bytes.data,d+*p,len);v->bytes.length=len;*p+=len;return v;}
 return NULL; }
BValue*bdecode(const unsigned char*d,size_t n,size_t*c){size_t p=0;BValue*v=parse(d,n,&p);if(c)*c=p;return v;}
void bfree(BValue*v){if(!v)return;if(v->type==B_BYTES)free(v->bytes.data);else if(v->type==B_LIST){for(size_t i=0;i<v->list.count;i++)bfree(v->list.items[i]);free(v->list.items);}else if(v->type==B_DICT){for(size_t i=0;i<v->dict.count;i++){free(v->dict.keys[i]);bfree(v->dict.values[i]);}free(v->dict.keys);free(v->dict.values);}free(v);}
BValue*bdict_get(BValue*d,const char*k){if(!d||d->type!=B_DICT)return NULL;for(size_t i=0;i<d->dict.count;i++)if(!strcmp(d->dict.keys[i],k))return d->dict.values[i];return NULL;}
