#include "common.h"
#include "engine.h"
#ifdef _WIN32
#include "gui.h"
#endif
#include <stdio.h>
#include <string.h>

static int run_cli(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "BitTorrent Client %s\nUsage: bittorrentclient <torrent-or-magnet> <save-path>\n", BT_VERSION);
        return 1;
    }
    return bt_engine_run(argv[1], argv[2]);
}

int main(int argc, char **argv) {
#ifdef _WIN32
    if (argc == 1 || (argc > 1 && strcmp(argv[1], "--gui") == 0))
        return gui_run(argc, argv);
#endif
    return run_cli(argc, argv);
}
