#include <string.h>
#include "../include/mrc_common.h"
#include "../include/mrc_ccontext.h"

mrc_ccontext *
mrc_ccontext_new(mrb_state *mrb)
{
  mrc_ccontext *c = (mrc_ccontext*)xmalloc(sizeof(mrc_ccontext));
  c->mrb = mrb;
  return c;
}

const char *
mrc_ccontext_filename(mrc_ccontext *c, const char *s)
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

void mrc_ccontext_free(mrc_ccontext *c)
{
  xfree(c->filename);
  xfree(c->syms);
  xfree(c);
}
