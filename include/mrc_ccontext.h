#ifndef MRC_CCONTEXT_H
#define MRC_CCONTEXT_H

#include "mrc_common.h"
#include "mrc_diagnostic.h"
#include "mrc_throw.h"
#include <stddef.h>

MRC_BEGIN_DECL

#if defined(MRC_PARSER_PRISM)
  #include "prism.h" // in lib/prism/include
  typedef pm_node_t mrc_node;
  typedef pm_parser_t mrc_parser_state;
  typedef pm_constant_id_list_t mrc_constant_id_list;
  typedef struct {
    pm_parser_t parser;
    pm_options_t options;
    pm_string_t input;
    bool parsed;
  } pm_parse_result_t;
#elif defined(MRC_PARSER_LRAMA)
  #include "rubyparser.h"
  #include "lrama_helper.h"
  typedef NODE mrc_node;
  typedef struct parser_params mrc_parser_state;
  typedef rb_ast_id_table_t mrc_constant_id_list;
#else
  #error "No parser defined. Please define MRC_PARSER_PRISM or MRC_PARSER_LRAMA."
#endif

struct mrc_diagnostic_list;
typedef struct mrc_diagnostic_list mrc_diagnostic_list;

typedef struct mrc_ccontext {
  mrb_state *mrb;
  struct mrc_jmpbuf *jmp;
  mrc_parser_state *p;
  mrc_sym *syms;
  int slen;
  char *filename;
  uint16_t lineno;
  struct RClass *target_class;
  mrc_bool capture_errors:1;
  mrc_bool dump_result:1;
  mrc_bool no_exec:1;
  mrc_bool keep_lv:1;
  mrc_bool no_optimize:1;
  mrc_bool no_ext_ops:1;
  const struct RProc *upper;

  //size_t parser_nerr;
  mrc_diagnostic_list *diagnostic_list;
} mrc_ccontext;                 /* compiler context */

mrc_ccontext *mrc_ccontext_new(mrb_state *mrb);
const char *mrc_ccontext_filename(mrc_ccontext *c, const char *s);
void mrc_ccontext_free(mrc_ccontext *c);

MRC_END_DECL

#endif // MRC_CCONTEXT_H
