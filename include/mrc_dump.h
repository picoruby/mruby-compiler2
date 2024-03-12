#ifndef MRC_DUMP_H
#define MRC_DUMP_H

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

static inline size_t
uint8_to_bin(uint8_t s, uint8_t *bin)
{
  *bin = s;
  return sizeof(uint8_t);
}

static inline size_t
uint16_to_bin(uint16_t s, uint8_t *bin)
{
  *bin++ = (s >> 8) & 0xff;
  *bin   = s & 0xff;
  return sizeof(uint16_t);
}

static inline size_t
uint32_to_bin(uint32_t l, uint8_t *bin)
{
  *bin++ = (l >> 24) & 0xff;
  *bin++ = (l >> 16) & 0xff;
  *bin++ = (l >> 8) & 0xff;
  *bin   = l & 0xff;
  return sizeof(uint32_t);
}

static inline uint32_t
bin_to_uint32(const uint8_t *bin)
{
  return (uint32_t)bin[0] << 24 |
         (uint32_t)bin[1] << 16 |
         (uint32_t)bin[2] << 8  |
         (uint32_t)bin[3];
}

static inline uint16_t
bin_to_uint16(const uint8_t *bin)
{
  return (uint16_t)bin[0] << 8 |
         (uint16_t)bin[1];
}

static inline uint8_t
bin_to_uint8(const uint8_t *bin)
{
  return (uint8_t)bin[0];
}
MRC_END_DECL

#endif // MRC_DUMP_H

