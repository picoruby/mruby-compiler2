#include "../include/mrc_common.h"
#include "../include/mrc_parser_util.h"
#include "../include/mrc_irep.h"
#include "../include/mrc_ccontext.h"
#include "../include/mrc_codegen.h"
#include "../include/mrc_dump.h"
#include "../include/mrc_codedump.h"
#include "../include/mrc_opcode.h"
#include "../include/mrc_presym.h"
#include "../include/mrc_diagnostic.h"

static mrc_irep *
mrc_load_exec(mrc_ccontext *c, mrc_node *ast)
{
  mrc_irep *irep;
  /* parse error */
  if (0 < c->p->error_list.size) {
    pm_diagnostic_t *e = (pm_diagnostic_t *)c->p->error_list.head;
    while (e) {
      mrc_diagnostic_list_append(c, e->location.start, e->message, MRC_PARSER_ERROR);
      e = (pm_diagnostic_t *)e->node.next;
    }
    return NULL;
  }
  /* parse warning */
  if (0 < c->p->warning_list.size) {
    pm_diagnostic_t *w = (pm_diagnostic_t *)c->p->warning_list.head;
    while (w) {
      mrc_diagnostic_list_append(c, w->location.start, w->message, MRC_PARSER_WARNING);
      w = (pm_diagnostic_t *)w->node.next;
    }
  }
  irep = mrc_generate_code(c, ast);
  if (c->capture_errors) {
    return NULL;
  }
  if (c->dump_result) {
#if defined(MRC_PARSER_PRISM)
    {
      pm_buffer_t buffer = { 0 };
#if defined(MRC_DUMP_PRETTY)
      pm_prettyprint(&buffer, c->p, ast);
#endif
      printf("\n(ast buffer length: %ld)\n%s\n", buffer.length, buffer.value);
      pm_buffer_free(&buffer);
    }
    mrc_codedump_all(c, irep);
#elif defined(MRC_PARSER_LRAMA)
    parser_dump(ast->body.root, 0);
#endif
  }

  return irep;
}


#if defined(MRC_PARSER_PRISM)
static void
mrc_pm_parser_init(mrc_parser_state *p, const uint8_t *source, size_t size)
{
  pm_parser_init(p, source, size, NULL);
  mrc_init_presym(&p->constant_pool);
}
#elif defined(MRC_PARSER_LRAMA)

#include "internal/parse.h"

//rb_parser_string_t *rb_parser_string_new(rb_parser_t *p, const char *ptr, long len);

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
  mrc_pm_parser_init(c->p, string.source, string.length);
  return pm_parse(c->p);
#elif defined(MRC_PARSER_LRAMA)
  FILE *f = fopen(filename, "r");
  if (!f) {
    fprintf(stderr, "cannot open file: %s\n", filename);
    return NULL;
  } else {
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
  mrc_pm_parser_init(c->p, string.source, string.length);
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
