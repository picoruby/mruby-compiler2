#include <string.h>
#include "mrc_common.h"
#include "mrc_ccontext.h"

typedef struct {
  int index;
  const char *lit;
} mrc_sym_entry;

static mrc_sym_entry symTable[] = {
#define MRC_OPSYM_2(name, lit, num) {num, #lit},
#define MRC_SYM_2(lit, num)         {num, #lit},
#include "mrc_presym.inc"
#undef MRC_OPSYM_2
#undef MRC_SYM_2
  {0, NULL} // sentinel
};

mrc_sym
mrc_find_presym(const uint8_t *lit, size_t len)
{
  for (int i = 0; ; i++) {
    if (symTable[i].lit == NULL) {
      return 0;
    }
    if (strlen(symTable[i].lit) == len && memcmp(symTable[i].lit, lit, len) == 0) {
      return symTable[i].index;
    }
  }
  return 0; //should not happen. Todo: error handling
}

void
mrc_init_presym(pm_constant_pool_t *pool)
{
  pm_constant_id_t id;
  for (int i = 0; ; i++) {
    if (symTable[i].lit == NULL) { break; }
    id = pm_constant_pool_insert_constant(pool, (const uint8_t *)symTable[i].lit, strlen(symTable[i].lit));
    mrc_assert(id == symTable[i].index);
  }
}
