#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lrama_helper.h"

#undef xmalloc
#undef xcalloc
#undef xrealloc
#undef xfree
// FIXME: to be configurable
#define xmalloc malloc
#define xcalloc calloc
#define xrealloc realloc
#define xfree free

#define Qfalse 0x00
#define Qnil   0x04
#define Qtrue  0x14
#define Qundef 0x24

#define rb_encoding void
#define OnigCodePoint unsigned int

ID next_id = 0;

#define POOL_CAPACITY 1024

pm_constant_pool_t pool;

static IO *
io_new(void)
{
    IO *io;

    io = (IO *)xmalloc(sizeof(IO));
    io->type = T_IO;

    return io;
}

static Encoding *
encoding_new(void)
{
    Encoding *encoding;

    encoding = (Encoding *)xmalloc(sizeof(Encoding));
    encoding->type = T_ENCODING;

    return encoding;
}

static VALUE
debug_output_stdout(void)
{
    return (VALUE)io_new();
}

static rb_encoding *
utf8_encoding(void)
{
    return (rb_encoding *)encoding_new();
}

static rb_encoding *
enc_get(VALUE obj)
{
    return utf8_encoding();
}

static int
enc_asciicompat(rb_encoding *enc)
{
    return 1;
}

static int
nil_p(VALUE obj)
{
    return obj == Qnil;
}

static char *
string_value_cstr(volatile VALUE *ptr)
{
    return ((String *)*ptr)->s;
}

static VALUE
compile_callback(VALUE (*func)(VALUE), VALUE arg)
{
    return func(arg);
}

static char *
rstring_ptr(VALUE str)
{
    // if(str<10000) return "dummy";
    return ((String *)str)->s;
}

static long
rstring_len(VALUE str)
{
    if(str<10000) return 0;
    return ((String *)str)->len;
}

static VALUE
verbose(void)
{
    return Qfalse;
}

static int
rtest(VALUE obj)
{
    return obj != Qfalse;
}

static void *
alloc_n(size_t nelems, size_t elemsiz)
{
    return xmalloc(nelems * elemsiz);
}

static void *
sized_realloc_n(void *ptr, size_t new_nelems, size_t elemsiz, size_t old_nelems)
{
    (void)old_nelems;
    return xrealloc(ptr, new_nelems * elemsiz);
}

static int
enc_isalnum(OnigCodePoint c, rb_encoding *enc)
{
    return isalnum(c);
}

static int
enc_precise_mbclen(const char *p, const char *e, rb_encoding *enc)
{
    return 1;
}

static int
mbclen_charfound_p(int len)
{
    return 1;
}

static int
is_local_id(ID id)
{
    return 0;
}

static void
sized_xfree(void *x, size_t size)
{
    xfree(x);
}

static VALUE
str_to_interned_str(VALUE str)
{
  return str;
}

static VALUE
id2sym(ID id)
{
    return (VALUE)id;
}

static double
parser_strtod(const char *nptr, char **endptr)
{
    return 0.0;
}

static VALUE
enc_str_new(const char *ptr, long len, rb_encoding *enc)
{
    return (VALUE)string_new_with_str_len(ptr, len);
}

static VALUE
id2str(ID id)
{
    pm_constant_t *constant = pm_constant_pool_id_to_constant(&pool, id);
    return (VALUE)enc_str_new((const char *)constant->start, constant->length, NULL) - POOL_ID_OFFSET;
}

static int
is_ascii_string(VALUE str)
{
    return 1;
}

static void *
my_zalloc(size_t size)
{
    return xcalloc(size, 1);
}

static VALUE
io_flush(VALUE io)
{
    return Qnil;
}

static VALUE
syntax_error_append(VALUE error_buffor, VALUE source_file_string, int line, int column, rb_encoding *enc, const char *fmt, va_list args)
{
    return error_buffor;
}

static VALUE
syntax_error_new(void)
{
    return (VALUE)string_new_with_str("SyntaxError");
}

static void
set_errinfo(VALUE error)
{
}

static VALUE
str_buf_cat(VALUE dst, const char *src, long srclen)
{
    String *string = (String *)dst;
    string->s = realloc(string->s, string->len + srclen);
    memcpy(string->s + string->len, src, srclen);
    string->len += srclen;
    return dst;
}

static VALUE
enc_associate(VALUE str, rb_encoding *enc)
{
    return str;
}

static int
id_type(ID id)
{
    return 0;
}

static ID
intern3(const char *name, long len, rb_encoding *enc)
{
    (void)enc;
    /*
     * FIXME: memory leak
     * 考察：たとえば、parse.yのtokenize_ident()のTOK_INTERN()がtok(p)
     * ではなくp->lex.ptokをつかえば、xmallocせずに済むかもしれない。
     */
    char *str = (char *)xmalloc(len + 1);
    memcpy(str, name, len);
    str[len] = '\0';
    return pm_constant_pool_insert_constant(&pool, (const uint8_t *)str, len) + POOL_ID_OFFSET;
}

static ID
intern2(const char *name, long len)
{
    return intern3(name, len, NULL);
}

static ID
intern(const char *name)
{
    return intern2(name, strlen(name));
}

static char *
enc_prev_char(const char *s, const char *p, const char *e, rb_encoding *enc)
{
    return (char *)p;
}

static int
stderr_tty_p(void)
{
    return 0;
}

static VALUE
str_catf(VALUE str, const char *fmt, ...)
{
    va_list args;
    char *buf;
    int len;

    va_start(args, fmt);
    len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    buf = xmalloc(len + 1);
    va_start(args, fmt);
    vsnprintf(buf, len + 1, fmt, args);
    va_end(args);

    str_buf_cat(str, buf, len);
    xfree(buf);

    return str;
}

static void
write_error_str(VALUE str)
{
}

static void *
xmalloc_mul_add(size_t x, size_t y, size_t z)
{
  //TODO: check overflow
    return xmalloc(x * y + z);
}

static void *
nonempty_memcpy(void *dst, const void *src, size_t n, size_t nn)
{
    (void)nn;
    if (n > 0) {
        return memcpy(dst, src, n);
    }
    return dst;
}

static VALUE
str_new_cstr(const char *ptr)
{
    return (VALUE)string_new_with_str(ptr);
}

static int
long2int(long l)
{
    return (int)l;
}

static String *
string_new(void)
{
    String *string;

    string = (String *)xmalloc(sizeof(String));
    string->type = T__STRING;

    return string;
}

String *
string_new_with_str_len(const char *ptr, long len)
{
    String *string = string_new();
    string->s = xmalloc(sizeof(char) * len);
    if (ptr)
      memcpy(string->s, ptr, len);
    string->len = len;

    return string;
}

String *
string_new_with_str(const char *ptr)
{
    return string_new_with_str_len(ptr, strlen(ptr));
}

void
parser_config_initialize(rb_parser_config_t *config)
{
    config->calloc = xcalloc;
    config->malloc = xmalloc;
    config->alloc = xmalloc;
    config->alloc_n = alloc_n;
    config->sized_xfree = sized_xfree;
    config->sized_realloc_n = sized_realloc_n;

    config->qnil = Qnil;
//    config->qtrue = Qtrue;
    config->qfalse = Qfalse;
//    config->qundef = Qundef;

    config->debug_output_stdout = debug_output_stdout;
    config->debug_output_stderr = debug_output_stdout;
    config->utf8_encoding = utf8_encoding;

    config->str_to_interned_str = str_to_interned_str;

    config->enc_get = enc_get;
    config->enc_asciicompat = enc_asciicompat;
    config->nil_p = nil_p;
    config->string_value_cstr = string_value_cstr;
    config->compile_callback = compile_callback;
    config->rstring_ptr = rstring_ptr;
    config->rstring_len = rstring_len;
    config->verbose = verbose;
    config->rtest = rtest;
    config->enc_isalnum = enc_isalnum;
    config->enc_precise_mbclen = enc_precise_mbclen;
    config->mbclen_charfound_p = mbclen_charfound_p;
    config->intern3 = intern3;
    config->is_local_id = is_local_id;

    // added by hasumi
    config->id2sym = id2sym;
    config->strtod = parser_strtod;
    config->id2str = id2str;
    config->enc_str_new = enc_str_new;
    config->is_ascii_string = is_ascii_string;
    config->zalloc = my_zalloc;
    config->free = xfree;
    config->io_flush = io_flush;
    config->syntax_error_append = syntax_error_append;
    config->syntax_error_new = syntax_error_new;
    config->set_errinfo = set_errinfo;
    config->enc_associate = enc_associate;
    config->id_type = id_type;
    config->intern2 = intern2;
    config->enc_prev_char = enc_prev_char;
    config->stderr_tty_p = stderr_tty_p;
    config->str_catf = str_catf;
    config->write_error_str = write_error_str;
    config->nonempty_memcpy = nonempty_memcpy;
    // needed by node.c
    config->xmalloc_mul_add = xmalloc_mul_add;
    // needed by node_dump.c
    config->str_new_cstr = str_new_cstr;
    config->long2int = long2int;

    if (!pm_constant_pool_init(&pool, POOL_CAPACITY)) {
      fprintf(stderr, "pm_constant_pool_init failed\n");
    }
}

/* missing functions */

VALUE
rb_str_new_parser_string(rb_parser_string_t *str)
{
  return (VALUE)str;
}

VALUE
rb_node_file_path_val(const NODE *node)
{
    return rb_str_new_parser_string(RNODE_FILE(node)->path);
}

VALUE
rb_node_str_string_val(const NODE *node)
{
    rb_parser_string_t *str = RNODE_STR(node)->string;
    return rb_str_new_parser_string(str);
}

VALUE
rb_node_line_lineno_val(const NODE *node)
{
    return Qnil; //INT2FIX(node->nd_loc.beg_pos.lineno);
}

VALUE
rb_str_new_mutable_parser_string(rb_parser_string_t *str)
{
  return (VALUE)str;
}

VALUE
rb_node_encoding_val(const NODE *node)
{
    return Qnil; // rb_enc_from_encoding(RNODE_ENCODING(node)->enc);
}

VALUE
rb_node_integer_literal_val(const NODE *n)
{
  return Qnil;
}

VALUE
rb_node_float_literal_val(const NODE *n)
{
  return Qnil;
}

VALUE
rb_node_rational_literal_val(const NODE *n)
{
  return Qnil;
}

VALUE
rb_node_imaginary_literal_val(const NODE *n)
{
  return Qnil;
}

VALUE
rb_node_regx_string_val(const NODE *node)
{
  return Qnil;
}

VALUE
rb_node_sym_string_val(const NODE *node)
{
  return Qnil;
}

VALUE
rb_sym2id(VALUE sym)
{
  return Qnil;
}
