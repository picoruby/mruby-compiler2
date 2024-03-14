#include <string.h>
#include "mrc_common.h"

typedef struct {
  int index;
  const char *name;
} mrc_sym_entry;

static mrc_sym_entry symTable[] = {
#define MRC_OPSYM_2(name, lit, num) {num, #name},
#define MRC_SYM_2(name, lit, num)   {num, #name},
#include "mrc_presym.inc"
#undef MRC_OPSYM_2
#undef MRC_SYM_2
  {0, NULL} // sentinel
};

mrc_sym
mrc_find_presym(const uint8_t *name, size_t len)
{
  for (int i = 0; ; i++) {
    if (symTable[i].name == NULL) {
      return 0;
    }
    if (strlen(symTable[i].name) == len && memcmp(symTable[i].name, name, len) == 0) {
      return symTable[i].index;
    }
  }
  return 0; //should not happen. Todo: error handling
}

