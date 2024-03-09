#include "prism.h" // in lib/prism/include
#include "../include/mrc_common.h"
#include "../include/mrc_irep.h"
#include "../include/mrc_ccontext.h"
#include "../include/opcode.h"

#ifndef MRC_CODEGEN_LEVEL_MAX
#define MRC_CODEGEN_LEVEL_MAX 256
#endif

#define MAXARG_S (1<<16)

typedef struct {
    pm_parser_t parser;
    pm_options_t options;
    pm_string_t input;
    bool parsed;
} pm_parse_result_t;

enum looptype {
  LOOP_NORMAL,
  LOOP_BLOCK,
  LOOP_FOR,
  LOOP_BEGIN,
  LOOP_RESCUE,
};

struct loopinfo {
  enum looptype type;
  uint32_t pc0;                 /* `next` destination */
  uint32_t pc1;                 /* `redo` destination */
  uint32_t pc2;                 /* `break` destination */
  int reg;                      /* destination register */
  struct loopinfo *prev;
};

typedef struct scope {
  mrb_state *mrb;
//  mrb_pool *mpool; // -> *page

  struct scope *prev;

  pm_constant_id_list_t *lv;

  uint16_t sp;
  uint32_t pc;
  uint32_t lastpc;
  uint32_t lastlabel;
  uint16_t ainfo:15;
  mrc_bool mscope:1;

  struct loopinfo *loop;
  mrc_sym filename_sym;
  uint16_t lineno;

  mrc_code *iseq;
  uint16_t *lines;
  uint32_t icapa;

  mrc_irep *irep;
  mrc_pool_value *pool;
  mrc_sym *syms;
  mrc_irep **reps;
  struct mrc_irep_catch_handler *catch_table;
  uint32_t pcapa, scapa, rcapa;

  uint16_t nlocals;
  uint16_t nregs;
//  int ai;

  int debug_start_pos;
  uint16_t filename_index;
  parser_state* parser;

  int rlev;                     /* recursion levels */
} codegen_scope;

static void
mrc_assert(int cond)
{
  if (!cond) {
    abort();
  }
}

static void
mrc_debug_info_alloc(mrc_irep *irep)
{
  // TODO
}

static void
codegen_error(codegen_scope *s, const char *message)
{
  // TODO
}

static mrc_irep*
mrc_add_irep(void)
{
  static const mrc_irep mrc_irep_zero = { 0 };
  mrc_irep *irep = (mrc_irep *)xmalloc(sizeof(mrc_irep));
  *irep = mrc_irep_zero;
  irep->refcnt = 1;
  return irep;
}

static void
scope_add_irep(codegen_scope *s)
{
  mrc_irep *irep;
  codegen_scope *prev = s->prev;
  if (prev->irep == NULL) {
    irep = mrc_add_irep();
    prev->irep = s->irep = irep;
    return;
  }
  else {
    if (prev->irep->rlen == UINT16_MAX) {
      codegen_error(s, "too many nested blocks/methods");
    }
    s->irep = irep = mrc_add_irep();
    if (prev->irep->rlen == prev->rcapa) {
      prev->rcapa *= 2;
      prev->reps = (mrc_irep **)xrealloc(prev->reps, sizeof(mrc_irep *)*prev->rcapa);
    }
    prev->reps[prev->irep->rlen++] = irep;
  }
}

static codegen_scope *
scope_new(codegen_scope *prev, pm_constant_id_list_t *nlv)
{
  static const codegen_scope codegen_scope_zero = { 0 };
  codegen_scope *s = (codegen_scope *)xcalloc(1, sizeof(codegen_scope));
  if (!s) {
    if (prev)
      codegen_error(prev, "unexpected scope");
    return NULL;
  }
  *s = codegen_scope_zero;
  if (!prev) return s;
  s->prev = prev;
  s->ainfo = 0;
  s->mscope = 0;

  scope_add_irep(s);

  s->rcapa = 8;
  s->reps = (mrc_irep **)xmalloc(sizeof(mrc_irep *)*s->rcapa);
  s->icapa = 1024;
  s->iseq = (mrc_code *)xmalloc(sizeof(mrc_code)*s->icapa);
  s->pcapa = 32;
  s->pool = (mrc_pool_value *)xmalloc(sizeof(mrc_pool_value)*s->pcapa);
  s->scapa = 256;
  s->syms = (mrc_sym *)xmalloc(sizeof(mrc_sym)*s->scapa);
  s->lv = nlv;
  s->sp += nlv->size + 1; // add self
  s->nlocals = s->sp;
  if (nlv) {
    mrc_sym *lv;
    s->irep->lv = lv = (mrc_sym *)xmalloc(sizeof(mrc_sym)*(s->nlocals-1));
    memcpy(lv, nlv->ids, sizeof(mrc_sym)*nlv->size);
    mrc_assert(nlv->size < UINT16_MAX);
  }
  // s->ai = gc_areba_save(mrc);
  s->filename_sym = prev->filename_sym;
  if (s->filename_sym) {
    s->lines = (uint16_t *)xmalloc(sizeof(uint16_t)*s->icapa);
  }
  s->lineno = prev->lineno;

  /* degug info */
  s->debug_start_pos = 0;
  if (s->filename_sym) {
    mrc_debug_info_alloc(s->irep);
  }
  else {
    s->irep->debug_info = NULL;
  }
  s->parser = prev->parser;
  s->filename_index = prev->filename_index;
  s->rlev = prev->rlev + 1;
  return s;
}

mrc_code *
mrc_compile(const char *src)
{
  pm_parse_result_t result = { 0 };
  pm_parser_t *parser = &result.parser;
  pm_parser_init(parser, pm_string_source(&result.input), pm_string_length(&result.input), &result.options);
  const pm_node_t *node = pm_parse(parser);
  // todo: check error
  // todo: check warnings
  pm_program_node_t *cast = (pm_program_node_t *)node;
  codegen_scope *scope = scope_new(NULL, &cast->locals);
  return scope->iseq;
}

/*------------------------------------*/

mrc_irep *
mrc_load_exec(mrb_state *mrb, struct mrc_parser_state *p, mrc_ccontext *c)
{
  return NULL;
}

struct mrc_parser_state*
mrc_parser_new(mrb_state *mrb)
{
  struct mrc_parser_state *p;
  static const struct mrc_parser_state parser_state_zero = { 0 };
  p = (struct mrc_parser_state *)xmalloc(sizeof(struct mrc_parser_state));
  *p = parser_state_zero;
  p->mrb = mrb;
  return p;
}

#ifndef MRC_NO_STDIO
static struct mrc_parser_state *
mrc_parse_file_continue(mrb_state *mrb, const char *filename, const void *prebuf, size_t prebufsize, mrc_ccontext *c)
{
  struct mrc_parser_state *p;

  p = mrc_parser_new(mrb);
  if (!p) return NULL;
  if (prebuf) {
    p->s = (const char*)prebuf;
    p->send = (const char*)prebuf + prebufsize;
  }
  else {
    p->s = p->send = NULL;
  }
  p->filename = filename;

  pm_parser_t parser;
  pm_string_t string;

  pm_string_mapped_init(&string, filename);
  pm_parser_init(&parser, string.source, string.length, NULL);
  pm_node_t *root = pm_parse(&parser);
  { // Debug print
    pm_buffer_t buffer = { 0 };
    pm_prettyprint(&buffer, &parser, root);
    printf("length: %ld\n%s\n", buffer.length, buffer.value);
    pm_buffer_free(&buffer);
  }
  pm_string_free(&string);
  pm_node_destroy(&parser, root);
  pm_parser_free(&parser);

  return p;
}

struct mrc_parser_state*
mrc_parse_file(mrb_state *mrb, const char *filename, mrc_ccontext *c)
{
  return mrc_parse_file_continue(mrb, filename, NULL, 0, c);
}

mrc_irep *
mrc_load_file_cxt(mrb_state *mrb, const char *filename, mrc_ccontext *c)
{
  return mrc_load_exec(mrb, mrc_parse_file(mrb, filename, c), c);
}
#endif

struct mrc_parser_state*
mrc_parse_string(mrb_state *mrb, const uint8_t *source, size_t length, mrc_ccontext *c)
{
  struct mrc_parser_state *p;
  p = mrc_parser_new(mrb);
  p->filename = c->filename;

  pm_parser_t parser;
  pm_string_t string;

  pm_string_owned_init(&string, (uint8_t *)source, length);
  pm_parser_init(&parser, string.source, string.length, NULL);
  pm_node_t *root = pm_parse(&parser);
  { // Debug print
    pm_buffer_t buffer = { 0 };
    pm_prettyprint(&buffer, &parser, root);
    printf("length: %ld\n%s\n", buffer.length, buffer.value);
    pm_buffer_free(&buffer);
  }
  pm_string_free(&string);
  pm_node_destroy(&parser, root);
  pm_parser_free(&parser);

  return p;
}

mrc_irep *
mrc_load_string_cxt(mrb_state *mrb, const uint8_t *source, size_t length, mrc_ccontext *c)
{
  return mrc_load_exec(mrb, mrc_parse_string(mrb, source, length, c), c);
}

