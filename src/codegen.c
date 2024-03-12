
#include <string.h>
#include "mrc_irep.h"
#include "mrc_ccontext.h"
#include "mrc_parser.h"
#include "mrc_throw.h"
#include "mrc_opcode.h"

#ifndef MRC_CODEGEN_LEVEL_MAX
#define MRC_CODEGEN_LEVEL_MAX 256
#endif

#define MAXARG_S (1<<16)

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
//  mrb_pool *mpool; // -> *page

  struct scope *prev;

  mrc_constant_id_list *lv;

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
  mrc_ccontext* c;

  int rlev;                     /* recursion levels */
} mrc_codegen_scope;

static void codegen(mrc_codegen_scope *s, mrc_node *tree, int val);

static void
mrc_debug_info_alloc(mrc_irep *irep)
{
  // TODO
}

static void
codegen_error(mrc_codegen_scope *s, const char *message)
{
  // TODO
}

static void*
codegen_palloc(mrc_codegen_scope *s, size_t len)
{
  void *p = xmalloc(len);

  if (!p) codegen_error(s, "pool memory allocation");
  return p;
}

static void*
codegen_realloc(mrc_codegen_scope *s, void *p, size_t len)
{
  p = xrealloc(p, len);

  if (!p && len > 0) codegen_error(s, "mrc_realloc");
  return p;
}

static void
check_no_ext_ops(mrc_codegen_scope *s, uint16_t a, uint16_t b)
{
  if (s->c->no_ext_ops && (a | b) > 0xff) {
    codegen_error(s, "need OP_EXTs instruction (currently OP_EXTs are prohibited)");
  }
}

static int
new_label(mrc_codegen_scope *s)
{
  return s->lastlabel = s->pc;
}

static void
emit_B(mrc_codegen_scope *s, uint32_t pc, uint8_t i)
{
  if (pc >= s->icapa) {
    if (pc == UINT32_MAX) {
      codegen_error(s, "too big code block");
    }
    if (pc >= UINT32_MAX / 2) {
      pc = UINT32_MAX;
    }
    else {
      s->icapa *= 2;
    }
    s->iseq = (mrc_code*)codegen_realloc(s, s->iseq, sizeof(mrc_code)*s->icapa);
    if (s->lines) {
      s->lines = (uint16_t*)codegen_realloc(s, s->lines, sizeof(uint16_t)*s->icapa);
    }
  }
  if (s->lines) {
    if (s->lineno > 0 || pc == 0)
      s->lines[pc] = s->lineno;
    else
      s->lines[pc] = s->lines[pc-1];
  }
  s->iseq[pc] = i;
}

static void
emit_S(mrc_codegen_scope *s, int pc, uint16_t i)
{
  uint8_t hi = i>>8;
  uint8_t lo = i&0xff;

  emit_B(s, pc,   hi);
  emit_B(s, pc+1, lo);
}

static void
gen_B(mrc_codegen_scope *s, uint8_t i)
{
  emit_B(s, s->pc, i);
  s->pc++;
}

static void
gen_S(mrc_codegen_scope *s, uint16_t i)
{
  emit_S(s, s->pc, i);
  s->pc += 2;
}

static void
genop_0(mrc_codegen_scope *s, mrc_code i)
{
  s->lastpc = s->pc;
  gen_B(s, i);
}

static void
genop_1(mrc_codegen_scope *s, mrc_code i, uint16_t a)
{
  s->lastpc = s->pc;
  check_no_ext_ops(s, a, 0);
  if (a > 0xff) {
    gen_B(s, OP_EXT1);
    gen_B(s, i);
    gen_S(s, a);
  }
  else {
    gen_B(s, i);
    gen_B(s, (uint8_t)a);
  }
}

static void
genop_2(mrc_codegen_scope *s, mrc_code i, uint16_t a, uint16_t b)
{
  s->lastpc = s->pc;
  check_no_ext_ops(s, a, b);
  if (a > 0xff && b > 0xff) {
    gen_B(s, OP_EXT3);
    gen_B(s, i);
    gen_S(s, a);
    gen_S(s, b);
  }
  else if (b > 0xff) {
    gen_B(s, OP_EXT2);
    gen_B(s, i);
    gen_B(s, (uint8_t)a);
    gen_S(s, b);
  }
  else if (a > 0xff) {
    gen_B(s, OP_EXT1);
    gen_B(s, i);
    gen_S(s, a);
    gen_B(s, (uint8_t)b);
  }
  else {
    gen_B(s, i);
    gen_B(s, (uint8_t)a);
    gen_B(s, (uint8_t)b);
  }
}

static void
genop_3(mrc_codegen_scope *s, mrc_code i, uint16_t a, uint16_t b, uint16_t c)
{
  genop_2(s, i, a, b);
  gen_B(s, (uint8_t)c);
}

static void
genop_2S(mrc_codegen_scope *s, mrc_code i, uint16_t a, uint16_t b)
{
  genop_1(s, i, a);
  gen_S(s, b);
}

static void
genop_2SS(mrc_codegen_scope *s, mrc_code i, uint16_t a, uint32_t b)
{
  genop_1(s, i, a);
  gen_S(s, b>>16);
  gen_S(s, b&0xffff);
}

static void
genop_W(mrc_codegen_scope *s, mrc_code i, uint32_t a)
{
  uint8_t a1 = (a>>16) & 0xff;
  uint8_t a2 = (a>>8) & 0xff;
  uint8_t a3 = a & 0xff;

  s->lastpc = s->pc;
  gen_B(s, i);
  gen_B(s, a1);
  gen_B(s, a2);
  gen_B(s, a3);
}

#define NOVAL  0
#define VAL    1

#define nregs_update do {if (s->sp > s->nregs) s->nregs = s->sp;} while (0)
static void
push_n_(mrc_codegen_scope *s, int n)
{
  if (s->sp+n >= 0xffff) {
    codegen_error(s, "too complex expression");
  }
  s->sp+=n;
  nregs_update;
}

static void
pop_n_(mrc_codegen_scope *s, int n)
{
  if ((int)s->sp-n < 0) {
    codegen_error(s, "stack pointer underflow");
  }
  s->sp-=n;
}

#define push() push_n_(s,1)
#define push_n(n) push_n_(s,n)
#define pop() pop_n_(s,1)
#define pop_n(n) pop_n_(s,n)
#define cursp() (s->sp)

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
scope_add_irep(mrc_codegen_scope *s)
{
  mrc_irep *irep;
  mrc_codegen_scope *prev = s->prev;
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

static mrc_codegen_scope *
scope_new(mrc_codegen_scope *prev, mrc_constant_id_list *nlv)
{
  static const mrc_codegen_scope codegen_scope_zero = { 0 };
  mrc_codegen_scope *s = (mrc_codegen_scope *)xmalloc(sizeof(mrc_codegen_scope));
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
  s->c = prev->c;
  s->filename_index = prev->filename_index;
  s->rlev = prev->rlev + 1;
  return s;
}

static mrc_bool
no_optimize(mrc_codegen_scope *s)
{
  if (s && s->c && s->c->no_optimize)
    return TRUE;
  return FALSE;
}

struct mrc_insn_data
mrc_decode_insn(const mrc_code *pc)
{
  struct mrc_insn_data data = { 0 };
  if (pc == 0) return data;
  data.addr = pc;
  mrc_code insn = READ_B();
  uint16_t a = 0;
  uint16_t b = 0;
  uint16_t cc = 0;

  switch (insn) {
#define FETCH_Z() /* empty */
#define OPCODE(i,x) case OP_ ## i: FETCH_ ## x (); break;
#include "mrc_ops.h"
#undef OPCODE
  }
  switch (insn) {
  case OP_EXT1:
    insn = READ_B();
    switch (insn) {
#define OPCODE(i,x) case OP_ ## i: FETCH_ ## x ## _1 (); break;
#include "mrc_ops.h"
#undef OPCODE
    }
    break;
  case OP_EXT2:
    insn = READ_B();
    switch (insn) {
#define OPCODE(i,x) case OP_ ## i: FETCH_ ## x ## _2 (); break;
#include "mrc_ops.h"
#undef OPCODE
    }
    break;
  case OP_EXT3:
    insn = READ_B();
    switch (insn) {
#define OPCODE(i,x) case OP_ ## i: FETCH_ ## x ## _3 (); break;
#include "mrc_ops.h"
#undef OPCODE
    }
    break;
  default:
    break;
  }
  data.insn = insn;
  data.a = a;
  data.b = b;
  data.cc = cc;
  return data;
}

#undef OPCODE
#define Z 1
#define S 3
#define W 4
#define OPCODE(_,x) x,
/* instruction sizes */
static uint8_t mrc_insn_size[] = {
#define B 2
#define BB 3
#define BBB 4
#define BS 4
#define BSS 6
#include "mrc_ops.h"
#undef B
#undef BB
#undef BBB
#undef BS
#undef BSS
};
/* EXT1 instruction sizes */
static uint8_t mrc_insn_size1[] = {
#define B 3
#define BB 4
#define BBB 5
#define BS 5
#define BSS 7
#include "mrc_ops.h"
#undef B
#undef BS
#undef BSS
};
/* EXT2 instruction sizes */
static uint8_t mrc_insn_size2[] = {
#define B 2
#define BS 4
#define BSS 6
#include "mrc_ops.h"
#undef B
#undef BB
#undef BBB
#undef BS
#undef BSS
};
/* EXT3 instruction sizes */
#define B 3
#define BB 5
#define BBB 6
#define BS 5
#define BSS 7
static uint8_t mrc_insn_size3[] = {
#include "mrc_ops.h"
};
#undef B
#undef BB
#undef BBB
#undef BS
#undef BSS
#undef OPCODE
static mrc_bool
no_peephole(mrc_codegen_scope *s)
{
  return no_optimize(s) || s->lastlabel == s->pc || s->pc == 0 || s->pc == s->lastpc;
}

static void
gen_move(mrc_codegen_scope *s, uint16_t dst, uint16_t src, int nopeep)
{
  if (nopeep || no_peephole(s)) goto normal;
  else if (dst == src) return;
  else {
    // todo: peephole
  }
 normal:
  genop_2(s, OP_MOVE, dst, src);
}

static void
gen_assignment(mrc_codegen_scope *s, mrc_node *tree, mrc_node *rhs, int sp, int val)
{
  int idx;

  switch (PM_NODE_TYPE(tree)) {
    case PM_LOCAL_VARIABLE_WRITE_NODE:
    {
      if (rhs) {
        codegen(s, rhs, VAL);
        pop();
        sp = cursp();
      }
      break;
    }
    default:
    {
      printf("Not implemented %s\n", pm_node_type_to_str(PM_NODE_TYPE(tree)));
      break;
    }
  }

  switch (PM_NODE_TYPE(tree)) {
    case PM_LOCAL_VARIABLE_WRITE_NODE:
    {
      pm_local_variable_write_node_t *cast = (pm_local_variable_write_node_t *)tree;
      idx = cast->name;
      if (cast->depth == 0) {
        if (idx != sp) {
          gen_move(s, idx, sp, val);
        }
        break;
      }
      else {
        // TODO
        //gen_setupvar(s, sp, idx, cast->depth, val);
      }
      break;
    }
    default:
    {
      printf("Not implemented %s\n", pm_node_type_to_str(PM_NODE_TYPE(tree)));
      break;
    }
  }
}

static void
gen_int(mrc_codegen_scope *s, uint16_t dst, mrc_int i)
{
  if (i < 0) {
    if (i == -1) genop_1(s, OP_LOADI__1, dst);
    else if (i >= -0xff) genop_2(s, OP_LOADINEG, dst, (uint16_t)-i);
    else if (i >= INT16_MIN) genop_2S(s, OP_LOADI16, dst, (uint16_t)i);
    else if (i >= INT32_MIN) genop_2SS(s, OP_LOADI32, dst, (uint32_t)i);
    else goto int_lit;
  }
  else if (i < 8) genop_1(s, OP_LOADI_0 + (uint8_t)i, dst);
  else if (i <= 0xff) genop_2(s, OP_LOADI, dst, (uint16_t)i);
  else if (i <= INT16_MAX) genop_2S(s, OP_LOADI16, dst, (uint16_t)i);
  else if (i <= INT32_MAX) genop_2SS(s, OP_LOADI32, dst, (uint32_t)i);
  else {
  int_lit:
    // todo
    //genop_2(s, OP_LOADL, dst, new_lit_int(s, i));
  }
}

static void
gen_return(mrc_codegen_scope *s, uint8_t op, uint16_t src)
{
  // todo: peephole
//  if (no_peephole(s)) {
    genop_1(s, op, src);
//  }
//  else {
//    struct mrc_insn_data data = mrc_last_insn(s);
//
//    if (data.insn == OP_MOVE && src == data.a) {
//      rewind_pc(s);
//      genop_1(s, op, data.b);
//    }
//    else if (data.insn != OP_RETURN) {
//      genop_1(s, op, src);
//    }
//  }
}

static void
scope_finish(mrc_codegen_scope *s)
{
  mrc_irep *irep = s->irep;

  if (0xff < s->nlocals) {
    codegen_error(s, "too many local variables");
  }
  irep->flags = 0;
  if (s->iseq) {
    size_t catchsize = sizeof(struct mrc_irep_catch_handler) * irep->rlen;
    irep->iseq = (const mrc_code *)codegen_realloc(s, s->iseq, sizeof(mrc_code)*s->pc + catchsize);
    irep->ilen = s->pc;
    if (0 < irep->clen) {
      memcpy((void *)(irep->iseq + irep->ilen), s->catch_table, catchsize);
    }
  }
  else {
    irep->clen = 0;
  }
  xfree(s->catch_table);
  s->catch_table = NULL;
  irep->pool = (const mrc_pool_value *)codegen_realloc(s, s->pool, sizeof(mrc_pool_value)*irep->plen);
  irep->syms = (const mrc_sym *)codegen_realloc(s, s->syms, sizeof(mrc_sym)*irep->slen);
  irep->reps = (const mrc_irep **)codegen_realloc(s, s->reps, sizeof(mrc_irep *)*irep->rlen);
  if (s->filename_sym) {
    // todo
    //mrc_sym fname = mrc_parser_get_filename(s->parser, s->filename_index);
    //const char *filename = mrc_sym_name_len(s->mrb, fname, NULL);

    //mrc_debug_info_append_file(s->mrb, s->irep->debug_info,
    //                           filename, s->lines, s->debug_start_pos, s->pc);
  }
  xfree(s->lines);
  irep->nlocals = s->nlocals;
  irep->nregs = s->nregs;

  //mrb_gc_arena_restore(mrb, s->ai);
  //mrb_pool_close(s->mpool);
}

static int
scope_body(mrc_codegen_scope *s, mrc_node *tree, int val)
{
  mrc_constant_id_list *nlv;
  mrc_node *node;
  switch (PM_NODE_TYPE(tree)) {
    case PM_PROGRAM_NODE:
    {
      pm_program_node_t *program = (pm_program_node_t *)tree;
      nlv = &program->locals;
      node = (mrc_node *)program->statements;
      break;
    }
    default:
    {
      printf("Not implemented %s\n", pm_node_type_to_str(PM_NODE_TYPE(tree)));
      break;
    }
  }
  mrc_codegen_scope *scope = scope_new(s, nlv);

  codegen(scope, node, VAL);
  gen_return(scope, OP_RETURN, scope->sp-1);
  if (!s->iseq) {
    genop_0(scope, OP_STOP);
  }
  scope_finish(scope);
  if (!s->irep) {
    /* should not happen */
    return 0;
  }
  return s->irep->rlen - 1;
}

static void
codegen(mrc_codegen_scope *s, mrc_node *tree, int val)
{
//  int nt;
  int rlev = s->rlev;

  if (!tree) {
    if (val) {
      genop_1(s, OP_LOADNIL, cursp());
      push();
    }
    return;
  }

  s->rlev++;
  if (s->rlev > MRC_CODEGEN_LEVEL_MAX) {
    codegen_error(s, "too complex expression");
  }
  // FIXME
  //if (s->irep && s->filename_index != tree->filename_index) {
  //  const char *filename = s->c->filename;

  //  mrc_debug_info_append_file(c, s->irep->debug_info,
  //                             filename, s->lines, s->debug_start_pos, s->pc);
  //  s->debug_start_pos = s->pc;
  //  s->filename_index = tree->filename_index;
  //  s->filename_sym = mrc_parser_get_filename(s->c->p, tree->filename_index);
  //}

//  nt = nint(tree->car);
//  s->lineno = tree->lineno;
//  tree = tree->cdr;
  switch (PM_NODE_TYPE(tree)) {
    case PM_PROGRAM_NODE: {
      // todo: lvar
      scope_body(s, tree, val);
      break;
    }
    case PM_STATEMENTS_NODE:
    {
      pm_statements_node_t *cast = (pm_statements_node_t *)tree;
      size_t last_index = cast->body.size;
      for (uint32_t i = 0; i < last_index; i++) {
        codegen(s, (mrc_node *)cast->body.nodes[i], val);
      }
      break;
    }
    case PM_LOCAL_VARIABLE_WRITE_NODE:
    {
      pm_local_variable_write_node_t *cast = (pm_local_variable_write_node_t *)tree;
      gen_assignment(s, tree, (mrc_node *)cast->value, 0, val);
    }
    case PM_INTEGER_NODE:
    {
      pm_integer_node_t *cast = (pm_integer_node_t *)tree;
      // todo: check overflow
      gen_int(s, cursp(), cast->value.head.value); // todo: negative or big integer
      push();
      break;
    }
    default:
    {
      printf("Not implemented %s\n", pm_node_type_to_str(PM_NODE_TYPE(tree)));
      break;
    }
  }
}

static mrc_irep *
generate_code(mrc_ccontext *c, mrc_node *node, int val)
{
  mrc_codegen_scope *scope = scope_new(NULL, NULL);
  struct mrc_jmpbuf *prev_jmp = c->jmp; // FIXME: c->jmp is not initialized
  struct mrc_jmpbuf jmpbuf;

  c->jmp = &jmpbuf;

  scope->c = c;
  //scope->filename_sym = c->filename_sym;
  //scppe->filename_index = c->filename_index;

  MRC_TRY(c->jmp) {
    codegen(scope, node, val);
    //proc->c = NULL;
    //if (mrb->c->cibase && mrb->c->cibase->proc == proc->upper) {
    //  proc->upper = NULL;
    //}
    c->jmp = prev_jmp;
    return scope->irep;
  }
  MRC_CATCH(c->jmp) {
    c->jmp = prev_jmp;
    return NULL;
  }
  MRC_END_EXC(c->jmp);
}

mrc_irep *
mrc_generate_code(mrc_ccontext *c, mrc_node *node)
{
  return generate_code(c, node, VAL);
}
