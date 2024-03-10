#include <string.h>
#include "../include/mrc_common.h"
#include "../include/mrc_ccontext.h"
#include "../include/mrc_parser.h"

mrc_ccontext *
mrc_ccontext_new(mrb_state *mrb)
{
  mrc_ccontext *c = (mrc_ccontext*)calloc(1, sizeof(mrc_ccontext));
  c->p = (mrc_parser_state *)xmalloc(sizeof(mrc_parser_state));
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
#if defined(MRC_PARSER_PRISM)
  pm_parser_free(c->p);
#elif defined(MRC_PARSER_KANEKO)
  kn_parser_free(c->p);
#endif
  xfree(c->p);
  xfree(c);
}
