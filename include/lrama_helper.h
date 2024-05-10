#ifndef LRAMA_HELPER_H
#define LRAMA_HELPER_H 1

#include "prism/util/pm_constant_pool.h"
#undef xmalloc
#undef xcalloc
#undef xrealloc
#undef xfree

#include "rubyparser.h"

#define POOL_ID_OFFSET 128

typedef enum object_type {
    T_NUMBER,
    T__STRING,
    T__HASH,
    T_IO,
    T_ENCODING,
} object_type_t;

typedef struct Object {
    object_type_t type;
} Object;

typedef struct Number {
    object_type_t type;
    long l;
} Number;

typedef struct String {
    object_type_t type;
    char *s;
    size_t len;
} String;

typedef struct Hash {
    object_type_t type;
} Hash;

typedef struct IO {
    object_type_t type;
} IO;

typedef struct Encoding {
    object_type_t type;
} Encoding;

String *string_new_with_str_len(const char *ptr, long length);
String *string_new_with_str(const char *ptr);
void parser_config_initialize(rb_parser_config_t *config);
void dump_ast(rb_ast_t *ast);

#endif /* LRAMA_HELPER_H */
