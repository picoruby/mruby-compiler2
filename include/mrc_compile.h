#ifndef MRC_COMPILE_H
#define MRC_COMPILE_H

#include "mrc_ccontext.h"
#include "mrc_irep.h"

MRC_BEGIN_DECL

void mrc_parser_set_filename(struct mrc_parser_state *p, const char *f);
mrc_irep *mrc_load_file_cxt(mrb_state *mrb, FILE *f, mrc_ccontext *c);

MRC_END_DECL

#endif
