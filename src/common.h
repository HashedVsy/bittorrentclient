#ifndef BT_COMMON_H
#define BT_COMMON_H
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define BT_VERSION "0.2.0"
#define BT_PORT 6882
#define DHT_PORT 6881
#define DHT_K 8
#define DHT_ALPHA 3
#define DHT_TIMEOUT_MS 1500
#define DHT_MAX_NODES 512
#define DHT_MAX_BUCKETS 160
#define DHT_NODE_FILE "dht.nodes"
#define BLOCK_SIZE 16384
#define MAX_PEERS 512
typedef struct { char ip[INET6_ADDRSTRLEN]; uint16_t port; } PeerAddress;
#endif
