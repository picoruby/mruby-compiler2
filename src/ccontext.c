#include <string.h>
#include "../include/mrc_common.h"
#include "../include/mrc_ccontext.h"
#include "../include/mrc_parser_util.h"

mrc_ccontext *
mrc_ccontext_new(mrb_state *mrb)
{
  mrc_ccontext *c = (mrc_ccontext*)mrc_calloc(1, sizeof(mrc_ccontext));
  c->p = (mrc_parser_state *)mrc_malloc(sizeof(mrc_parser_state));
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

void mrc_ccontext_free(mrc_ccontext *c)
{
  mrc_free(c->filename_table);
  mrc_free(c->filename);
  mrc_free(c->syms);
  pm_parser_free(c->p);
  mrc_diagnostic_list_free(c);
  if (c->p->lex_callback) {
    mrc_free(c->p->lex_callback);
  }
  mrc_free(c->p);
  mrc_free(c);
}
