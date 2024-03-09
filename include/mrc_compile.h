#ifndef MRC_COMPILE_H
#define MRC_COMPILE_H

#include "mrc_ccontext.h"
#include "mrc_irep.h"

MRC_BEGIN_DECL

mrc_irep *mrc_load_file_cxt(const char *filename, mrc_ccontext *c);
mrc_irep *mrc_load_string_cxt(const uint8_t *source, size_t length, mrc_ccontext *c);

MRC_END_DECL

#endif /* MRC_COMPILE_H */
