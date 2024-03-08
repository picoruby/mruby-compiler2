#include "../include/mrc_ccontext.h"

mrc_ccontext *
mrc_ccontext_new(mrb_state *mrb)
{
  return NULL;
}

const char *
mrc_ccontext_filename(mrb_state *mrb, mrc_ccontext *c, const char *s)
{
  return NULL;
}

void
mrc_ccontext_partial_hook(mrb_state *mrb, mrc_ccontext *c, int (*func)(struct mrc_parser_state*), void *data)
{
}

void mrc_ccontext_free(mrb_state *mrb, mrc_ccontext *c)
{
}
