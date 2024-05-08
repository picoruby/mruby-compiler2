#include "../include/mrc_common.h"
#include "../include/mrc_parser_util.h"
#include "../include/mrc_irep.h"
#include "../include/mrc_ccontext.h"
#include "../include/mrc_codegen.h"
#include "../include/mrc_dump.h"
#include "../include/mrc_codedump.h"
#include "../include/mrc_opcode.h"

//static void
//mrc_assert(int cond)
//{
//  if (!cond) {
//    abort();
//  }
//}

static mrc_irep *
mrc_load_exec(mrc_ccontext *c, mrc_node *ast)
{
  mrc_irep *irep;
  //if (parse error) {
  //  print error message
  //  return NULL;
  //}
  irep = mrc_generate_code(c, ast);
  //if (codegen error) {
  //  print error message
  //  return NULL;
  //}
  if (c->dump_result) {
#if defined(MRB_PARSER_PRISM)
    {
      pm_buffer_t buffer = { 0 };
      pm_prettyprint(&buffer, c->p, ast);
      printf("\n(ast buffer length: %ld)\n%s\n", buffer.length, buffer.value);
      pm_buffer_free(&buffer);
    }
    mrc_codedump_all(c, irep);
#elif defined(MRB_PARSER_LRAMA)
    parser_dump(ast->body.root, 0);
#endif
  }

  return irep;
}

#if defined(MRC_PARSER_LRAMA)

#include "internal/parse.h"

rb_parser_string_t *rb_parser_string_new(rb_parser_t *p, const char *ptr, long len);

static rb_parser_string_t *
rb_parser_gets(struct parser_params *p, rb_parser_input_data input, int line)
{
  FILE *fp = (FILE *)input;
  rb_parser_string_t *str;
  char buf[1024];
  if (!fgets(buf, sizeof(buf), fp)) {
    return NULL;
  }
  str = rb_parser_string_new(p, buf, strlen(buf));
  return str;
}
#endif

#ifndef MRC_NO_STDIO
static mrc_node *
mrc_parse_file_cxt(mrc_ccontext *c, const char *filename)
{
#if defined(MRC_PARSER_PRISM)
  pm_string_t string;
  pm_string_mapped_init(&string, filename);
  pm_parser_init(c->p, string.source, string.length, NULL);
  return pm_parse(c->p);
#elif defined(MRC_PARSER_LRAMA)
  FILE *f = fopen(filename, "r");
  if (!f) {
    fprintf(stderr, "cannot open file: %s\n", filename);
    return NULL;
  } else {
//    size_t len;
//    fseek(f, 0, SEEK_END);
//    len = ftell(f);
//    fseek(f, 0, SEEK_SET);
//    char *str = (char *)mrc_malloc(len); // FIXME: memory leak
//    fread(str, len, 1, f);
//    fclose(f);
//    VALUE string = (VALUE)string_new_with_str_len(str, len);
//    rb_ast_t *ast = rb_ruby_parser_compile_string(c->p, filename, string, 0);
    rb_ast_t *ast = rb_parser_compile(c->p, &rb_parser_gets, NULL, 0, NULL, (rb_parser_input_data)f, 1);
    return (mrc_node *)ast->body.root;
  }
#endif
}

mrc_irep *
mrc_load_file_cxt(mrc_ccontext *c, const char *filename)
{
  mrc_node *root = mrc_parse_file_cxt(c, filename);
  mrc_irep *irep = mrc_load_exec(c, root);
#if defined(MRC_PARSER_PRISM)
  pm_node_destroy(c->p, root);
#endif
  return irep;
}
#endif

static mrc_node *
mrc_parse_string_cxt(mrc_ccontext *c, const uint8_t *source, size_t length)
{
#if defined(MRC_PARSER_PRISM)
  pm_string_t string;
  pm_string_owned_init(&string, (uint8_t *)source, length);
  pm_parser_init(c->p, string.source, string.length, NULL);
  return pm_parse(c->p);
#elif defined(MRC_PARSER_LRAMA)
  //VALUE str = (VALUE)string_new_with_str_len((const char *)source, length);
  //rb_ast_t *ast = rb_ruby_parser_compile_string(c->p, "name", str, 0);
  //return (mrc_node *)ast->body.root;
  return NULL; // TODO
#endif
}

mrc_irep *
mrc_load_string_cxt(mrc_ccontext *c, const uint8_t *source, size_t length)
{
  mrc_node *root = mrc_parse_string_cxt(c, source, length);
  mrc_irep *irep = mrc_load_exec(c, root);
#if defined(MRC_PARSER_PRISM)
  pm_node_destroy(c->p, root);
#elif defined(MRC_PARSER_LRAMA)
  // TODO
#endif
  return irep;
}
