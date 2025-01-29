#include <string.h>
#include "../include/mrc_common.h"
#include "../include/mrc_ccontext.h"
#include "../include/mrc_parser_util.h"

MRC_API mrc_ccontext *
mrc_ccontext_new(mrb_state *mrb)
{
  mrc_ccontext *c = (mrc_ccontext*)mrc_calloc(1, sizeof(mrc_ccontext));
  c->p = (mrc_parser_state *)mrc_malloc(sizeof(mrc_parser_state));
  c->options = NULL;
#if defined(MRC_TARGET_MRUBY)
  c->mrb = mrb;
#else
  (void)mrb;
#endif
  return c;
}


MRC_API void
mrc_ccontext_cleanup_local_variables(mrc_ccontext *c)
{
  if (c->syms) {
    mrc_free(c->syms);
    c->syms = NULL;
    c->slen = 0;
  }
  c->keep_lv = FALSE;
}

MRC_API const char *
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

MRC_API void mrc_ccontext_free(mrc_ccontext *c)
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
