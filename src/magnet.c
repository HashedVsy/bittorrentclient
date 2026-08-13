#include "magnet.h"
#include <ctype.h>

static int hex(unsigned char c) { if(c>='0'&&c<='9')return c-'0'; if(c>='a'&&c<='f')return c-'a'+10; if(c>='A'&&c<='F')return c-'A'+10; return -1; }
static int pct(const char *s, char **out) {
    size_t n=strlen(s); char *p=malloc(n+1); if(!p)return 0; size_t j=0;
    for(size_t i=0;i<n;i++) { if(s[i]=='%' && i+2<n) { int a=hex((unsigned char)s[i+1]),b=hex((unsigned char)s[i+2]); if(a<0||b<0){free(p);return 0;} p[j++]=(char)((a<<4)|b); i+=2; } else if(s[i]=='+') p[j++]=' '; else p[j++]=s[i]; }
    p[j]=0; *out=p; return 1;
}
static void add_tracker(Magnet *m,const char *s) { char *x=NULL; if(!pct(s,&x))return; char **p=realloc(m->trackers,(m->tracker_count+1)*sizeof(*p)); if(!p){free(x);return;} m->trackers=p;m->trackers[m->tracker_count++]=x; }
int magnet_parse(const char *uri, Magnet *m) {
    memset(m,0,sizeof(*m)); if(strncmp(uri,"magnet:?",8)!=0)return 0;
    char *copy=_strdup(uri+8); if(!copy)return 0;
    for(char *tok=strtok(copy,"&");tok;tok=strtok(NULL,"&")) {
        char *eq=strchr(tok,'='); if(!eq)continue; *eq=0; const char *k=tok,*v=eq+1;
        if(strcmp(k,"xt")==0 && strncmp(v,"urn:btih:",9)==0) {
            const char *h=v+9; size_t n=strlen(h);
            if(n==40) { int ok=1; for(int i=0;i<20;i++){int a=hex((unsigned char)h[i*2]),b=hex((unsigned char)h[i*2+1]);if(a<0||b<0){ok=0;break;}m->info_hash[i]=(unsigned char)((a<<4)|b);}m->has_info_hash=ok; }
        } else if(strcmp(k,"dn")==0) { free(m->display_name); pct(v,&m->display_name); }
        else if(strcmp(k,"tr")==0) add_tracker(m,v);
    }
    free(copy); return m->has_info_hash;
}
void magnet_free(Magnet *m) { if(!m)return;free(m->display_name);for(size_t i=0;i<m->tracker_count;i++)free(m->trackers[i]);free(m->trackers);memset(m,0,sizeof(*m)); }
