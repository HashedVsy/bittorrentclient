#include "torrent.h"
#include <openssl/sha.h>

static unsigned char *load_file(const char *path, size_t *n) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (_fseeki64(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long long z = _ftelli64(f);
    if (z < 0 || _fseeki64(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }
    unsigned char *d = malloc((size_t)z);
    if (!d) { fclose(f); return NULL; }
    if (fread(d, 1, (size_t)z, f) != (size_t)z) { free(d); fclose(f); return NULL; }
    fclose(f); *n = (size_t)z; return d;
}

static char *bytes_string(BValue *v) {
    if (!v || v->type != B_BYTES) return NULL;
    char *s = malloc(v->bytes.length + 1);
    if (!s) return NULL;
    memcpy(s, v->bytes.data, v->bytes.length);
    s[v->bytes.length] = 0;
    return s;
}

static int put(unsigned char **buf, size_t *len, size_t *cap, const void *p, size_t n) {
    if (*len + n > *cap) {
        size_t c = *cap ? *cap : 1024;
        while (c < *len + n) {
            if (c > SIZE_MAX / 2) return 0;
            c *= 2;
        }
        unsigned char *q = realloc(*buf, c);
        if (!q) return 0;
        *buf = q; *cap = c;
    }
    memcpy(*buf + *len, p, n); *len += n; return 1;
}

static int enc(BValue *v, unsigned char **buf, size_t *len, size_t *cap) {
    char tmp[64];
    if (!v) return 0;
    switch (v->type) {
    case B_INT:
        snprintf(tmp, sizeof(tmp), "i%lld", (long long)v->integer);
        return put(buf, len, cap, tmp, strlen(tmp)) && put(buf, len, cap, "e", 1);
    case B_BYTES:
        snprintf(tmp, sizeof(tmp), "%zu:", v->bytes.length);
        return put(buf, len, cap, tmp, strlen(tmp)) && put(buf, len, cap, v->bytes.data, v->bytes.length);
    case B_LIST:
        if (!put(buf, len, cap, "l", 1)) return 0;
        for (size_t i = 0; i < v->list.count; ++i) if (!enc(v->list.items[i], buf, len, cap)) return 0;
        return put(buf, len, cap, "e", 1);
    case B_DICT:
        if (!put(buf, len, cap, "d", 1)) return 0;
        for (size_t i = 0; i < v->dict.count; ++i) {
            BValue key = {0}; key.type = B_BYTES;
            key.bytes.data = (unsigned char *)v->dict.keys[i];
            key.bytes.length = strlen(v->dict.keys[i]);
            if (!enc(&key, buf, len, cap) || !enc(v->dict.values[i], buf, len, cap)) return 0;
        }
        return put(buf, len, cap, "e", 1);
    }
    return 0;
}

static void add_string(char ***arr, size_t *count, const char *s) {
    char *x = _strdup(s);
    if (!x) return;
    char **p = realloc(*arr, (*count + 1) * sizeof(**arr));
    if (!p) { free(x); return; }
    *arr = p; (*arr)[(*count)++] = x;
}

int torrent_load(const char *path, Torrent *t) {
    memset(t, 0, sizeof(*t));
    size_t n = 0; unsigned char *d = load_file(path, &n);
    if (!d) return 0;
    size_t consumed = 0;
    t->root = bdecode(d, n, &consumed);
    free(d);
    if (!t->root || t->root->type != B_DICT) { torrent_free(t); return 0; }
    t->info = bdict_get(t->root, "info");
    if (!t->info || t->info->type != B_DICT) { torrent_free(t); return 0; }

    BValue *name = bdict_get(t->info, "name");
    BValue *pl = bdict_get(t->info, "piece length");
    BValue *ps = bdict_get(t->info, "pieces");
    t->name = bytes_string(name);
    if (!pl || pl->type != B_INT || !ps || ps->type != B_BYTES || ps->bytes.length % 20) { torrent_free(t); return 0; }
    t->piece_length = (uint32_t)pl->integer;
    t->pieces_length = ps->bytes.length;
    t->pieces = malloc(ps->bytes.length);
    if (!t->pieces) { torrent_free(t); return 0; }
    memcpy(t->pieces, ps->bytes.data, ps->bytes.length);

    BValue *length = bdict_get(t->info, "length");
    if (length && length->type == B_INT) {
        t->total_length = (uint64_t)length->integer;
    } else {
        BValue *files = bdict_get(t->info, "files");
        if (!files || files->type != B_LIST) { torrent_free(t); return 0; }
        for (size_t i = 0; i < files->list.count; ++i) {
            BValue *fl = bdict_get(files->list.items[i], "length");
            if (fl && fl->type == B_INT) t->total_length += (uint64_t)fl->integer;
        }
    }

    /* BEP 3: SHA-1 of the exact bencoded info dictionary. */
    unsigned char *encoded = NULL; size_t encoded_len = 0, encoded_cap = 0;
    if (!enc(t->info, &encoded, &encoded_len, &encoded_cap)) { free(encoded); torrent_free(t); return 0; }
    SHA1(encoded, encoded_len, t->info_hash);
    free(encoded);

    BValue *announce = bdict_get(t->root, "announce");
    if (announce) { char *s = bytes_string(announce); if (s) { add_string(&t->trackers, &t->tracker_count, s); free(s); } }
    BValue *al = bdict_get(t->root, "announce-list");
    if (al && al->type == B_LIST) {
        for (size_t i = 0; i < al->list.count; ++i) {
            BValue *tier = al->list.items[i];
            if (!tier || tier->type != B_LIST) continue;
            for (size_t j = 0; j < tier->list.count; ++j) {
                char *s = bytes_string(tier->list.items[j]);
                if (s) { add_string(&t->trackers, &t->tracker_count, s); free(s); }
            }
        }
    }
    BValue *url = bdict_get(t->root, "url-list");
    if (url && url->type == B_BYTES) { char *s = bytes_string(url); if (s) { add_string(&t->webseeds, &t->webseed_count, s); free(s); } }
    if (url && url->type == B_LIST) for (size_t i = 0; i < url->list.count; ++i) { char *s = bytes_string(url->list.items[i]); if (s) { add_string(&t->webseeds, &t->webseed_count, s); free(s); } }
    BValue *hs = bdict_get(t->root, "httpseeds");
    if (hs && hs->type == B_LIST) for (size_t i = 0; i < hs->list.count; ++i) { char *s = bytes_string(hs->list.items[i]); if (s) { add_string(&t->webseeds, &t->webseed_count, s); free(s); } }
    return 1;
}

void torrent_free(Torrent *t) {
    if (!t) return;
    free(t->name); free(t->pieces);
    for (size_t i = 0; i < t->tracker_count; ++i) free(t->trackers[i]);
    for (size_t i = 0; i < t->webseed_count; ++i) free(t->webseeds[i]);
    free(t->trackers); free(t->webseeds); bfree(t->root); memset(t, 0, sizeof(*t));
}

size_t torrent_piece_count(const Torrent *t) { return t->pieces_length / 20; }
uint64_t torrent_piece_length(const Torrent *t, size_t i) {
    size_t n = torrent_piece_count(t);
    if (!n || i >= n) return 0;
    return i + 1 == n ? t->total_length - (uint64_t)i * t->piece_length : t->piece_length;
}
int torrent_verify_piece(const Torrent *t, size_t i, const unsigned char *d, size_t n) {
    if (i >= torrent_piece_count(t)) return 0;
    unsigned char h[20]; SHA1(d, n, h); return memcmp(h, t->pieces + i * 20, 20) == 0;
}
