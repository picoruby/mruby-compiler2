#ifndef MRC_CONTEXT_H
#define MRC_CONTEXT_H

#include "mrc_common.h"

MRC_BEGIN_DECL

struct mrc_parser_state;

typedef struct mrc_ccontext {
  mrc_sym *syms;
  int slen;
  char *filename;
  uint16_t lineno;
  int (*partial_hook)(struct mrc_parser_state*);
  void *partial_data;
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

// TODO: move somewhere else
struct mrc_parser_state {
  mrc_ccontext *cxt;
  FILE *f;
};

// TODO: FIX
typedef void mrb_state;

mrc_ccontext *mrc_ccontext_new(mrb_state *mrb);
const char *mrc_ccontext_filename(mrb_state *mrb, mrc_ccontext *c, const char *s);
void mrc_ccontext_partial_hook(mrb_state *mrb, mrc_ccontext *c, int (*func)(struct mrc_parser_state*), void *data);
void mrc_ccontext_free(mrb_state *mrb, mrc_ccontext *c);

MRC_END_DECL

#endif // MRC_CONTEXT_H
