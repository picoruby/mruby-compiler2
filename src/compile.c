#include "prism.h" // in lib/prism/include
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
mrc_load_exec(mrc_ccontext *c, mrc_node *root)
{
  mrc_irep *irep;
  //if (parse error) {
  //  print error message
  //  return NULL;
  //}
  irep = mrc_generate_code(c, root);
  //if (codegen error) {
  //  print error message
  //  return NULL;
  //}
  if (c->dump_result) {
    {
      pm_buffer_t buffer = { 0 };
      pm_prettyprint(&buffer, c->p, root);
      printf("\n(ast buffer length: %ld)\n%s\n", buffer.length, buffer.value);
      pm_buffer_free(&buffer);
    }
    mrc_codedump_all(c, irep);
  }

  return irep;
}

#ifndef MRC_NO_STDIO
static mrc_node *
mrc_parse_file_cxt(mrc_ccontext *c, const char *filename)
{
  pm_string_t string;
  pm_string_mapped_init(&string, filename);
  pm_parser_init(c->p, string.source, string.length, NULL);
  return pm_parse(c->p);
}

mrc_irep *
mrc_load_file_cxt(mrc_ccontext *c, const char *filename)
{
  mrc_node *root = mrc_parse_file_cxt(c, filename);
  mrc_irep *irep = mrc_load_exec(c, root);
  pm_node_destroy(c->p, root);
  return irep;
}
#endif

static mrc_node *
mrc_parse_string_cxt(mrc_ccontext *c, const uint8_t *source, size_t length)
{
  pm_string_t string;
  pm_string_owned_init(&string, (uint8_t *)source, length);
  pm_parser_init(c->p, string.source, string.length, NULL);
  return pm_parse(c->p);
}

mrc_irep *
mrc_load_string_cxt(mrc_ccontext *c, const uint8_t *source, size_t length)
{
  mrc_node *root = mrc_parse_string_cxt(c, source, length);
  mrc_irep *irep = mrc_load_exec(c, root);
  pm_node_destroy(c->p, root);
  return irep;
}
