#include "common.h"
#include "torrent.h"
#include "tracker.h"
#include "dht.h"
#include "webseed.h"
#include "piece.h"
#include "peer.h"
#include <curl/curl.h>
int main(int argc,char**argv){if(argc!=3){printf("BitTorrent Client %s\nUsage: bittorrentclient <torrent> <output>\n",BT_VERSION);return 1;}WSADATA w;if(WSAStartup(MAKEWORD(2,2),&w)){fprintf(stderr,"WSAStartup failed\n");return 1;}curl_global_init(CURL_GLOBAL_DEFAULT);Torrent t;if(!torrent_load(argv[1],&t)){fprintf(stderr,"Failed to load torrent\n");curl_global_cleanup();WSACleanup();return 1;}printf("\n=== BitTorrent Client %s ===\nName: %s\nSize: %.2f MiB\nPieces: %zu\nPiece size: %u KiB\nTrackers: %zu\nWebSeeds: %zu\n",BT_VERSION,t.name?t.name:"?",(double)t.total_length/1048576.0,torrent_piece_count(&t),t.piece_length/1024,t.tracker_count,t.webseed_count);FILE*f=fopen(argv[2],"ab");if(!f){torrent_free(&t);curl_global_cleanup();WSACleanup();return 1;}fclose(f);unsigned char id[20]="-BTCL01-000000000000";PeerAddress peers[MAX_PEERS];size_t np=tracker_get_peers(&t,id,peers,MAX_PEERS);printf("[TRACKER] %zu peers\n",np);np+=dht_get_peers(&t,peers+np,MAX_PEERS-np);for(size_t i=0;i<torrent_piece_count(&t);i++){if(piece_is_complete(argv[2],&t,i))continue;int ok=0;for(size_t s=0;s<t.webseed_count&&!ok;s++){unsigned char*d=NULL;size_t n=0;if(webseed_download_piece(&t,t.webseeds[s],i,&d,&n)){ok=piece_write(argv[2],&t,i,d,n);free(d);}}for(size_t p=0;p<np&&!ok;p++){unsigned char*d=NULL;size_t n=0;if(peer_download_piece(&t,&peers[p],id,i,&d,&n)){ok=piece_write(argv[2],&t,i,d,n);free(d);}}printf("[%s] piece %zu/%zu\n",ok?"OK":"FAILED",i+1,torrent_piece_count(&t));}torrent_free(&t);curl_global_cleanup();WSACleanup();return 0;}
