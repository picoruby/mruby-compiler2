#ifndef MRC_DUMP_H
#define MRC_DUMP_H

#include <stdio.h>
#include "mrc_common.h"
#include "mrc_irep.h"
#include "mrc_ccontext.h"

MRC_BEGIN_DECL

#define MRC_DUMP_DEBUG_INFO 1
#define MRC_DUMP_STATIC 2

#define MRC_DUMP_OK                     0
#define MRC_DUMP_GENERAL_FAILURE      (-1)
#define MRC_DUMP_WRITE_FAULT          (-2)
#define MRC_DUMP_READ_FAULT           (-3)
#define MRC_DUMP_INVALID_FILE_HEADER  (-4)
#define MRC_DUMP_INVALID_IREP         (-5)
#define MRC_DUMP_INVALID_ARGUMENT     (-6)

int mrc_dump_irep_cstruct(mrc_ccontext *c, const mrc_irep *irep, uint8_t flags, FILE *fp, const char *initname);
int mrc_dump_irep_cfunc(mrc_ccontext *c, const mrc_irep *irep, uint8_t flags, FILE *fp, const char *initname);
int mrc_dump_irep_binary(mrc_ccontext *c, const mrc_irep *irep, uint8_t flags, FILE* fp);

#ifndef MRC_NO_STDIO
void mrc_codedump_all_file(mrc_ccontext *c, mrc_irep *irep, FILE *out);
#endif
void mrc_codedump_all(mrc_ccontext *c, mrc_irep *irep);

MRC_END_DECL

#endif // MRC_DUMP_H

