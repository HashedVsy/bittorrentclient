#include "dht.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <openssl/sha.h>

#define DHT_K 8
#define DHT_MAX_NODES 256
#define DHT_TXN_LEN 2
#define DHT_PACKET_MAX 2048

typedef struct {
    unsigned char id[20];
    struct sockaddr_in addr;
    int responded;
} DhtNode;

typedef struct {
    unsigned char *data;
    size_t len;
    size_t cap;
} Buf;

static int buf_put(Buf *b, const void *p, size_t n) {
    if (b->len + n > b->cap) {
        size_t c = b->cap ? b->cap * 2 : 512;
        while (c < b->len + n) {
            if (c > SIZE_MAX / 2) return 0;
            c *= 2;
        }
        unsigned char *q = realloc(b->data, c);
        if (!q) return 0;
        b->data = q;
        b->cap = c;
    }
    memcpy(b->data + b->len, p, n);
    b->len += n;
    return 1;
}

static int bstr(Buf *b, const void *p, size_t n) {
    char x[32];
    int m = snprintf(x, sizeof(x), "%zu:", n);
    return m > 0 && buf_put(b, x, (size_t)m) && buf_put(b, p, n);
}

static int str(Buf *b, const char *s) {
    return bstr(b, s, strlen(s));
}

static int integer(Buf *b, int64_t v) {
    char x[64];
    int n = snprintf(x, sizeof(x), "i%llde", (long long)v);
    return n > 0 && buf_put(b, x, (size_t)n);
}

static int make_query(Buf *b, const unsigned char id[20], const char *method,
                      const unsigned char *target, const unsigned char *txn) {
    if (!buf_put(b, "d1:ad2:id20:", 13)) return 0;
    if (!buf_put(b, id, 20)) return 0;

    if (strcmp(method, "get_peers") == 0) {
        if (!buf_put(b, "9:info_hash20:", 15)) return 0;
        if (!buf_put(b, target, 20)) return 0;
    } else if (strcmp(method, "find_node") == 0) {
        if (!buf_put(b, "6:target20:", 12)) return 0;
        if (!buf_put(b, target, 20)) return 0;
    } else {
        return 0;
    }

    if (!buf_put(b, "e1:q", 4)) return 0;
    if (!bstr(b, method, strlen(method))) return 0;
    if (!buf_put(b, "1:t2:", 5)) return 0;
    if (!buf_put(b, txn, DHT_TXN_LEN)) return 0;
    if (!buf_put(b, "1:y1:qe", 7)) return 0;
    return 1;
}

static int send_query(SOCKET s, const struct sockaddr_in *addr,
                      const unsigned char id[20], const char *method,
                      const unsigned char *target, unsigned char txn[2]) {
    txn[0] = (unsigned char)(rand() & 0xff);
    txn[1] = (unsigned char)(rand() & 0xff);

    Buf b = {0};
    int ok = make_query(&b, id, method, target, txn);
    if (ok) {
        int sent = sendto(s, (const char *)b.data, (int)b.len, 0,
                          (const struct sockaddr *)addr, sizeof(*addr));
        ok = sent == (int)b.len;
    }
    free(b.data);
    return ok;
}

static int same_txn(BValue *root, const unsigned char txn[2]) {
    BValue *t = bdict_get(root, "t");
    return t && t->type == B_BYTES && t->bytes.length == 2 &&
           memcmp(t->bytes.data, txn, 2) == 0;
}

static void add_peer(PeerAddress *peers, size_t *count, size_t max,
                     const unsigned char *v, size_t n) {
    if (n != 6 || *count >= max) return;
    struct in_addr a;
    memcpy(&a.s_addr, v, 4);
    char ip[INET_ADDRSTRLEN];
    if (!inet_ntop(AF_INET, &a, ip, sizeof(ip))) return;
    uint16_t port = (uint16_t)(((uint16_t)v[4] << 8) | v[5]);
    if (!port) return;
    for (size_t i = 0; i < *count; ++i)
        if (peers[i].port == port && strcmp(peers[i].ip, ip) == 0) return;
    strncpy(peers[*count].ip, ip, sizeof(peers[*count].ip) - 1);
    peers[*count].ip[sizeof(peers[*count].ip) - 1] = 0;
    peers[*count].port = port;
    (*count)++;
}

static void parse_nodes(BValue *v, DhtNode *nodes, size_t *count) {
    if (!v || v->type != B_BYTES) return;
    for (size_t i = 0; i + 26 <= v->bytes.length && *count < DHT_MAX_NODES; i += 26) {
        struct in_addr a;
        memcpy(&a.s_addr, v->bytes.data + i + 20, 4);
        uint16_t port = (uint16_t)(((uint16_t)v->bytes.data[i + 24] << 8) | v->bytes.data[i + 25]);
        if (!port) continue;
        int duplicate = 0;
        for (size_t j = 0; j < *count; ++j)
            if (memcmp(nodes[j].id, v->bytes.data + i, 20) == 0) { duplicate = 1; break; }
        if (duplicate) continue;
        memcpy(nodes[*count].id, v->bytes.data + i, 20);
        memset(&nodes[*count].addr, 0, sizeof(nodes[*count].addr));
        nodes[*count].addr.sin_family = AF_INET;
        nodes[*count].addr.sin_addr = a;
        nodes[*count].addr.sin_port = htons(port);
        nodes[*count].responded = 0;
        (*count)++;
    }
}

static void parse_values(BValue *v, PeerAddress *peers, size_t *count, size_t max) {
    if (!v || v->type != B_LIST) return;
    for (size_t i = 0; i < v->list.count; ++i) {
        BValue *x = v->list.items[i];
        if (x && x->type == B_BYTES) add_peer(peers, count, max, x->bytes.data, x->bytes.length);
    }
}

static void bootstrap_dns(SOCKET s, DhtNode *nodes, size_t *count) {
    static const char *hosts[] = {
        "router.bittorrent.com",
        "router.utorrent.com",
        "dht.transmissionbt.com"
    };
    for (size_t i = 0; i < sizeof(hosts) / sizeof(hosts[0]) && *count < DHT_MAX_NODES; ++i) {
        struct addrinfo hints = {0}, *res = NULL;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        if (getaddrinfo(hosts[i], "6881", &hints, &res) != 0 || !res) continue;
        struct sockaddr_in *a = (struct sockaddr_in *)res->ai_addr;
        int dup = 0;
        for (size_t j = 0; j < *count; ++j)
            if (nodes[j].addr.sin_addr.s_addr == a->sin_addr.s_addr && nodes[j].addr.sin_port == a->sin_port) dup = 1;
        if (!dup) {
            memset(&nodes[*count], 0, sizeof(nodes[*count]));
            nodes[*count].addr = *a;
            ++*count;
        }
        freeaddrinfo(res);
    }
    (void)s;
}

static int wait_packet(SOCKET s, unsigned char *packet, int cap, int timeout_ms,
                       struct sockaddr_in *from, int *received) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(s, &set);
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    int r = select(0, &set, NULL, NULL, &tv);
    if (r <= 0) return 0;
    int flen = sizeof(*from);
    int n = recvfrom(s, (char *)packet, cap, 0, (struct sockaddr *)from, &flen);
    if (n <= 0) return 0;
    *received = n;
    return 1;
}

size_t dht_get_peers(const Torrent *torrent, PeerAddress *peers, size_t max_peers) {
    if (!torrent || !peers || !max_peers) return 0;

    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) {
        printf("[DHT] UDP socket failed.\n");
        return 0;
    }

    struct sockaddr_in local = {0};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons(DHT_PORT);
    if (bind(s, (struct sockaddr *)&local, sizeof(local)) != 0) {
        local.sin_port = 0;
        if (bind(s, (struct sockaddr *)&local, sizeof(local)) != 0) {
            closesocket(s);
            printf("[DHT] Could not bind UDP socket.\n");
            return 0;
        }
    }

    unsigned char node_id[20];
    random_bytes(node_id, sizeof(node_id));

    DhtNode nodes[DHT_MAX_NODES];
    size_t node_count = 0;
    bootstrap_dns(s, nodes, &node_count);

    /* A BEP 5 torrent may supply its own initial node list. */
    BValue *torrent_nodes = bdict_get(torrent->root, "nodes");
    if (torrent_nodes && torrent_nodes->type == B_LIST) {
        for (size_t i = 0; i < torrent_nodes->list.count && node_count < DHT_MAX_NODES; ++i) {
            BValue *pair = torrent_nodes->list.items[i];
            if (!pair || pair->type != B_LIST || pair->list.count != 2) continue;
            BValue *host = pair->list.items[0], *port = pair->list.items[1];
            if (!host || host->type != B_BYTES || !port || port->type != B_INT) continue;
            char h[256];
            if (host->bytes.length >= sizeof(h)) continue;
            memcpy(h, host->bytes.data, host->bytes.length); h[host->bytes.length] = 0;
            struct sockaddr_in a = {0};
            a.sin_family = AF_INET;
            if (inet_pton(AF_INET, h, &a.sin_addr) != 1) {
                struct addrinfo hints = {0}, *res = NULL;
                hints.ai_family = AF_INET; hints.ai_socktype = SOCK_DGRAM;
                if (getaddrinfo(h, NULL, &hints, &res) != 0 || !res) continue;
                a.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
                freeaddrinfo(res);
            }
            if (port->integer <= 0 || port->integer > 65535) continue;
            a.sin_port = htons((uint16_t)port->integer);
            nodes[node_count].addr = a;
            memset(nodes[node_count].id, 0, 20);
            ++node_count;
        }
    }

    printf("[DHT] Bootstrapped with %zu nodes.\n", node_count);

    size_t peer_count = 0;
    size_t cursor = 0;
    int rounds = 0;

    while (cursor < node_count && rounds++ < 96 && peer_count < max_peers) {
        DhtNode *n = &nodes[cursor++];
        unsigned char txn[2];
        if (!send_query(s, &n->addr, node_id, "get_peers", torrent->info_hash, txn)) continue;

        unsigned char packet[DHT_PACKET_MAX];
        struct sockaddr_in from;
        int got = 0;
        if (!wait_packet(s, packet, sizeof(packet), DHT_TIMEOUT_MS, &from, &got)) continue;

        size_t consumed = 0;
        BValue *root = bdecode(packet, (size_t)got, &consumed);
        if (!root || root->type != B_DICT || !same_txn(root, txn)) { bfree(root); continue; }

        BValue *type = bdict_get(root, "y");
        if (!type || type->type != B_BYTES || type->bytes.length != 1 || type->bytes.data[0] != 'r') { bfree(root); continue; }
        BValue *r = bdict_get(root, "r");
        if (!r || r->type != B_DICT) { bfree(root); continue; }

        BValue *rid = bdict_get(r, "id");
        if (rid && rid->type == B_BYTES && rid->bytes.length == 20) {
            memcpy(n->id, rid->bytes.data, 20);
            n->responded = 1;
        }

        size_t before = peer_count;
        parse_values(bdict_get(r, "values"), peers, &peer_count, max_peers);
        if (peer_count == before) parse_nodes(bdict_get(r, "nodes"), nodes, &node_count);
        bfree(root);
    }

    closesocket(s);
    printf("[DHT] Found %zu peers.\n", peer_count);
    return peer_count;
}
