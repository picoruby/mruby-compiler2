#ifndef MRC_CONTEXT_H
#define MRC_CONTEXT_H

#include "mrc_common.h"
#include "mrc_parser.h"
#include <stddef.h>

MRC_BEGIN_DECL

typedef struct mrc_ccontext {
  mrb_state *mrb;
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

  size_t parser_nerr;
} mrc_ccontext;                 /* compiler context */

mrc_ccontext *mrc_ccontext_new(mrb_state *mrb);
const char *mrc_ccontext_filename(mrc_ccontext *c, const char *s);
void mrc_ccontext_free(mrc_ccontext *c);

MRC_END_DECL

#endif // MRC_CONTEXT_H
