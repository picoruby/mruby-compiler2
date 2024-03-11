#include "../include/mrc_dump.h"

int
mrc_dump_irep_cstruct(mrc_ccontext *c, const mrc_irep *irep, uint8_t flags, FILE *fp, const char *initname)
{
  return 0;
}

int
mrc_dump_irep_cfunc(mrc_ccontext *c, const mrc_irep *irep, uint8_t flags, FILE *fp, const char *initname)
{
  return 0;
}

int
mrc_dump_irep_binary(mrc_ccontext *c, const mrc_irep *irep, uint8_t flags, FILE* fp)
{
  return 0;
}

#ifndef MRC_NO_STDIO
void
mrc_codedump_all_file(mrc_ccontext *c, mrc_irep *irep, FILE *out)
{
}
#endif

void
mrc_codedump_all(mrc_ccontext *c, mrc_irep *irep)
{
#ifndef MRC_NO_STDIO
  mrc_codedump_all_file(c, irep, stdout);
#endif
}
