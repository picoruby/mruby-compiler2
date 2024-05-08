#include <string.h>
#include "../include/mrc_common.h"
#include "../include/mrc_ccontext.h"
#include "../include/mrc_parser_util.h"

mrc_ccontext *
mrc_ccontext_new(mrb_state *mrb)
{
  mrc_ccontext *c = (mrc_ccontext*)mrc_calloc(1, sizeof(mrc_ccontext));
#if defined(MRC_PARSER_PRISM)
  c->p = (mrc_parser_state *)mrc_malloc(sizeof(mrc_parser_state));
#elif defined(MRC_PARSER_LRAMA)
  rb_parser_config_t *config = (rb_parser_config_t *)malloc(sizeof(rb_parser_config_t));
  parser_config_initialize(config);
  c->p = (mrc_parser_state *)rb_ruby_parser_new(config);
#endif
  c->mrb = mrb;
  return c;
}

const char *
mrc_ccontext_filename(mrc_ccontext *c, const char *s)
{
  if (s) {
    size_t len = strlen(s);
    char *p = (char*)mrc_malloc(len + 1);

    if (p == NULL) return NULL;
    memcpy(p, s, len + 1);
    if (c->filename) {
      mrc_free(c->filename);
    }
    c->filename = p;
  }
  return c->filename;
}

static void
kn_parser_free(mrc_parser_state *p)
{
  // TODO
}

void mrc_ccontext_free(mrc_ccontext *c)
{
  mrc_free(c->filename);
  mrc_free(c->syms);
#if defined(MRC_PARSER_PRISM)
  pm_parser_free(c->p);
#elif defined(MRC_PARSER_LRAMA)
  kn_parser_free(c->p);
#endif
  mrc_free(c->p);
  mrc_free(c);
}
