#include <string.h>
#include "../include/mrc_ccontext.h"
#include "../include/mrc_parser_util.h"

MRC_API mrc_ccontext *
mrc_ccontext_new(mrb_state *mrb)
{
  mrc_ccontext temp_c = {0};
  temp_c.mrb = mrb;
  mrc_ccontext *c = (mrc_ccontext *)mrc_calloc((&temp_c), 1, sizeof(mrc_ccontext));
  c->p = (mrc_parser_state *)mrc_malloc((&temp_c), sizeof(mrc_parser_state));
  c->options = NULL;
  c->mrb = temp_c.mrb;
  return c;
}


MRC_API void
mrc_ccontext_cleanup_local_variables(mrc_ccontext *c)
{
  if (c->syms) {
    mrc_free(c, c->syms);
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
    char *p = (char*)mrc_malloc(c, len + 1);

    if (p == NULL) return NULL;
    memcpy(p, s, len + 1);
    if (c->filename) {
      mrc_free(c, c->filename);
    }
    c->filename = p;
  }
  return c->filename;
}

MRC_API void mrc_ccontext_free(mrc_ccontext *c)
{
  if (c->filename_table) mrc_free(c, c->filename_table);
  if (c->filename) mrc_free(c, c->filename);
  if (c->syms) mrc_free(c, c->syms);
  pm_parser_free(c->p);
  mrc_diagnostic_list_free(c);
  if (c->p->lex_callback) {
    mrc_free(c, c->p->lex_callback);
  }
  mrc_free(c, c->p);
  mrc_free(c, c);
}
