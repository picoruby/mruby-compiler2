#include <string.h>
#include <stdint.h>
#include "../include/mrc_debug.h"

const char *
mrc_debug_get_filename(mrc_ccontext *c, const mrc_irep *irep, uint32_t pc)
{
//  if (irep && pc < irep->ilen) {
//    if (!irep->debug_info) return NULL;
//    return debug_get_filename(mrb, get_file(irep->debug_info, pc));
//  }
  return NULL;
}

mrc_irep_debug_info*
mrc_debug_info_alloc(mrc_irep *irep)
{
  static const mrc_irep_debug_info initial = { 0, 0, NULL };

  mrc_assert(!irep->debug_info);
  mrc_irep_debug_info *ret = (mrc_irep_debug_info*)mrc_malloc(sizeof(*ret));
  *ret = initial;
  irep->debug_info = ret;
  return ret;
}

//MRC_API mrc_irep_debug_info_file*
//mrc_debug_info_append_file(mrc_state *mrb, mrc_irep_debug_info *d,
//                           const char *filename, uint16_t *lines,
//                           uint32_t start_pos, uint32_t end_pos)
//{
//  if (!d) return NULL;
//  if (start_pos == end_pos) return NULL;
//
//  mrc_assert(filename);
//  mrc_assert(lines);
//
//  if (d->flen > 0) {
//    const char *fn = mrc_sym_name_len(mrb, d->files[d->flen - 1]->filename_sym, NULL);
//    if (strcmp(filename, fn) == 0)
//      return NULL;
//  }
//
//  mrc_irep_debug_info_file *f = (mrc_irep_debug_info_file*)mrc_malloc(mrb, sizeof(*f));
//  d->files = (mrc_irep_debug_info_file**)mrc_realloc(mrb, d->files, sizeof(mrc_irep_debug_info_file*) * (d->flen + 1));
//  d->files[d->flen++] = f;
//
//  uint32_t file_pc_count = end_pos - start_pos;
//  f->start_pos = start_pos;
//  d->pc_count = end_pos;
//
//  size_t fn_len = strlen(filename);
//  f->filename_sym = mrc_intern(mrb, filename, fn_len);
//  f->line_type = mrc_debug_line_packed_map;
//  f->lines.ptr = NULL;
//
//  uint16_t prev_line = 0;
//  uint32_t prev_pc = 0;
//  size_t packed_size = 0;
//  uint8_t *p;
//
//  for (uint32_t i = 0; i < file_pc_count; i++) {
//    if (lines[start_pos + i] == prev_line) continue;
//    packed_size += mrc_packed_int_len(start_pos+i-prev_pc);
//    prev_pc = start_pos+i;
//    packed_size += mrc_packed_int_len(lines[start_pos+i]-prev_line);
//    prev_line = lines[start_pos + i];
//  }
//  f->lines.packed_map = p = (uint8_t*)mrc_malloc(mrb, packed_size);
//  prev_line = 0; prev_pc = 0;
//  for (uint32_t i = 0; i < file_pc_count; i++) {
//    if (lines[start_pos + i] == prev_line) continue;
//    p += mrc_packed_int_encode(start_pos+i-prev_pc, p);
//    prev_pc = start_pos + i;
//    p += mrc_packed_int_encode(lines[start_pos + i]-prev_line, p);
//    prev_line = lines[start_pos + i];
//  }
//  f->line_entry_count = (uint32_t)packed_size;
//
//  return f;
//}

//MRC_API void
//mrc_debug_info_free(mrc_state *mrb, mrc_irep_debug_info *d)
//{
//  if (!d) { return; }
//
//  if (d->files) {
//    for (uint32_t i = 0; i < d->flen; i++) {
//      if (d->files[i]) {
//        mrc_free(mrb, d->files[i]->lines.ptr);
//        mrc_free(mrb, d->files[i]);
//      }
//    }
//    mrc_free(mrb, d->files);
//  }
//  mrc_free(mrb, d);
//}
