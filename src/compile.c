#include "prism.h" // in lib/prism/include
#include "../include/mrc_common.h"
#include "../include/mrc_parser.h"
#include "../include/mrc_irep.h"
#include "../include/mrc_ccontext.h"
#include "../include/mrc_codegen.h"
#include "../include/mrc_dump.h"
#include "../include/opcode.h"

//static void
//mrc_assert(int cond)
//{
//  if (!cond) {
//    abort();
//  }
//}

mrc_irep *
mrc_load_exec(mrc_node *root, mrc_ccontext *c)
{
  { // Debug print for development
    pm_buffer_t buffer = { 0 };
    pm_prettyprint(&buffer, c->p, root);
    printf("buffer length: %ld\n%s\n", buffer.length, buffer.value);
    pm_buffer_free(&buffer);
  }

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
  mrc_codedump_all(c, irep);

  return NULL; // ?????
}

#ifndef MRC_NO_STDIO
static mrc_node *
mrc_parse_file_cxt(const char *filename, mrc_ccontext *c)
{
  pm_string_t string;
  pm_string_mapped_init(&string, filename);
  pm_parser_init(c->p, string.source, string.length, NULL);
  return pm_parse(c->p);
}

mrc_irep *
mrc_load_file_cxt(const char *filename, mrc_ccontext *c)
{
  mrc_node *root = mrc_parse_file_cxt(filename, c);
  mrc_irep *irep = mrc_load_exec(root, c);
  pm_node_destroy(c->p, root);
  return irep;
}
#endif

static mrc_node *
mrc_parse_string_cxt(const uint8_t *source, size_t length, mrc_ccontext *c)
{
  pm_string_t string;
  pm_string_owned_init(&string, (uint8_t *)source, length);
  pm_parser_init(c->p, string.source, string.length, NULL);
  return pm_parse(c->p);
}

mrc_irep *
mrc_load_string_cxt(const uint8_t *source, size_t length, mrc_ccontext *c)
{
  mrc_node *root = mrc_parse_string_cxt(source, length, c);
  mrc_irep *irep = mrc_load_exec(root, c);
  pm_node_destroy(c->p, root);
  return irep;
}
