#include "../include/mrc_common.h"
#include "../include/mrc_ccontext.h"

mrc_ccontext *
mrc_ccontext_new(mrb_state *mrb)
{
  return (mrc_ccontext*)xcalloc(1, sizeof(mrc_ccontext));
}

const char *
mrc_ccontext_filename(mrb_state *mrb, mrc_ccontext *c, const char *s)
{
  if (s) {
    size_t len = strlen(s);
    char *p = (char*)xmalloc(len + 1);

    if (p == NULL) return NULL;
    memcpy(p, s, len + 1);
    if (c->filename) {
      xfree(c->filename);
    }
    c->filename = p;
  }
  return c->filename;
}

void
mrc_ccontext_partial_hook(mrb_state *mrb, mrc_ccontext *c, int (*func)(struct mrc_parser_state*), void *data)
{
  c->partial_hook = func;
  c->partial_data = data;
}

void mrc_ccontext_free(mrb_state *mrb, mrc_ccontext *cxt)
{
  xfree(cxt->filename);
  xfree(cxt->syms);
  xfree(cxt);
}
