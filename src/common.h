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
#define BT_VERSION "0.1.0"
#define BT_PORT 6882
#define BLOCK_SIZE 16384
#define MAX_PEERS 512
typedef struct { char ip[INET6_ADDRSTRLEN]; uint16_t port; } PeerAddress;
#endif
