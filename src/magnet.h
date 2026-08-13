#ifndef BT_MAGNET_H
#define BT_MAGNET_H
#include "common.h"

typedef struct {
    unsigned char info_hash[20];
    int has_info_hash;
    char *display_name;
    char **trackers;
    size_t tracker_count;
} Magnet;

int magnet_parse(const char *uri, Magnet *m);
void magnet_free(Magnet *m);
#endif
