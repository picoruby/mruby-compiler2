#include "../include/mrc_ccontext.h"
#include "../include/mrc_parser_util.h"

const char*
mrc_sym_name_len(mrc_ccontext *c, mrc_sym sym, mrc_int *lenp)
{
//#ifdef MRC_USE_ALL_SYMBOLS
//  return sym2name_len(c, sym, NULL, lenp);
//#else
//  return sym2name_len(c, sym, mrb->symbuf, lenp);
//#endif
#if defined(MRC_PARSER_PRISM)
  pm_constant_t *constant = pm_constant_pool_id_to_constant(&c->p->constant_pool, sym);
  if (constant) {
    *lenp = constant->length;
    return (const char*)constant->start;
  }
#elif defined(MRC_PARSER_LRAMA)
  // TODO
  *lenp = 4;
  return "puts";
#endif
  return NULL;
}
