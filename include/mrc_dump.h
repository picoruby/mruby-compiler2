#ifndef MRC_DUMP_H
#define MRC_DUMP_H

#include <stdio.h>
#include "mrc_common.h"
#include "mrc_irep.h"

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

int mrc_dump_irep_cstruct(mrb_state *mrb, const mrc_irep *irep, uint8_t flags, FILE *fp, const char *initname);
int mrc_dump_irep_cfunc(mrb_state *mrb, const mrc_irep *irep, uint8_t flags, FILE *fp, const char *initname);
int mrc_dump_irep_binary(mrb_state *mrb, const mrc_irep *irep, uint8_t flags, FILE* fp);

MRC_END_DECL

#endif // MRC_DUMP_H

