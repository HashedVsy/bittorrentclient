#ifndef BT_BENCODE_H
#define BT_BENCODE_H
#include <stddef.h>
#include <stdint.h>
typedef enum { B_INT, B_BYTES, B_LIST, B_DICT } BType;
typedef struct BValue BValue;
struct BValue { BType type; union { int64_t integer; struct { unsigned char *data; size_t length; } bytes; struct { BValue **items; size_t count; } list; struct { char **keys; BValue **values; size_t count; } dict; }; };
BValue *bdecode(const unsigned char*,size_t,size_t*);
void bfree(BValue*);
BValue *bdict_get(BValue*,const char*);
#endif
