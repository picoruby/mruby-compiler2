
#include <string.h>
#include "../include/mrc_irep.h"
#include "../include/mrc_ccontext.h"
#include "../include/mrc_parser_util.h"
#include "../include/mrc_throw.h"
#include "../include/mrc_opcode.h"
#include "../include/mrc_presym.h"

#if defined(MRC_INT64)
  typedef int64_t mrc_int;
  typedef uint64_t mrc_uint;
# define MRC_INT_BIT 64
# define MRC_INT_MIN INT64_MIN
# define MRC_INT_MAX INT64_MAX
# define MRC_PRIo PRIo64
# define MRC_PRId PRId64
# define MRC_PRIx PRIx64
#else
  typedef int32_t mrc_int;
  typedef uint32_t mrc_uint;
# define MRC_INT_BIT 32
# define MRC_INT_MIN INT32_MIN
# define MRC_INT_MAX INT32_MAX
# define MRC_PRIo PRIo32
# define MRC_PRId PRId32
# define MRC_PRIx PRIx32
#endif

static inline mrc_bool
mrc_int_mul_overflow(mrc_int a, mrc_int b, mrc_int *c)
{
#ifdef MRC_INT32
  int64_t n = (int64_t)a * b;
  *c = (mrc_int)n;
  return n > MRC_INT_MAX || n < MRC_INT_MIN;
#else /* MRC_INT64 */
  if (a > 0 && b > 0 && a > MRC_INT_MAX / b) return TRUE;
  if (a < 0 && b > 0 && a < MRC_INT_MIN / b) return TRUE;
  if (a > 0 && b < 0 && b < MRC_INT_MIN / a) return TRUE;
  if (a < 0 && b < 0 && (a <= MRC_INT_MIN || b <= MRC_INT_MIN || -a > MRC_INT_MAX / -b))
    return TRUE;
  *c = a * b;
  return FALSE;
#endif
}

static mrc_int
mrc_div_int(mrc_int x, mrc_int y)
{
  mrc_int div = x / y;

  if ((x ^ y) < 0 && x != div * y) {
    div -= 1;
  }
  return div;
}

#define NUMERIC_SHIFT_WIDTH_MAX (MRC_INT_BIT-1)

static mrc_bool
mrc_num_shift(mrc_int val, mrc_int width, mrc_int *num)
{
  if (width < 0) {              /* rshift */
    if (width == MRC_INT_MIN || -width >= NUMERIC_SHIFT_WIDTH_MAX) {
      if (val < 0) {
        *num = -1;
      }
      else {
        *num = 0;
      }
    }
    else {
      *num = val >> -width;
    }
  }
  else if (val > 0) {
    if ((width > NUMERIC_SHIFT_WIDTH_MAX) ||
        (val   > (MRC_INT_MAX >> width))) {
      return FALSE;
    }
    *num = val << width;
  }
  else {
    if ((width > NUMERIC_SHIFT_WIDTH_MAX) ||
        (val   < (MRC_INT_MIN >> width))) {
      return FALSE;
    }
    if (width == NUMERIC_SHIFT_WIDTH_MAX)
      *num = MRC_INT_MIN;
    else
      *num = val * ((mrc_int)1 << width);
  }
  return TRUE;
}

#ifdef MRC_ENDIAN_BIG
# define MRC_ENDIAN_LOHI(a,b) a b
#else
# define MRC_ENDIAN_LOHI(a,b) b a
#endif
#ifndef MRC_CODEGEN_LEVEL_MAX
#define MRC_CODEGEN_LEVEL_MAX 256
#endif

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

static const mrc_code*
mrc_prev_pc(mrc_codegen_scope *s, const mrc_code *pc)
{
  const mrc_code *prev_pc = NULL;
  const mrc_code *i = s->iseq;

  while (i<pc) {
    uint8_t insn = i[0];
    prev_pc = i;
    switch (insn) {
    case OP_EXT1:
      i += mrc_insn_size1[i[1]] + 1;
      break;
    case OP_EXT2:
      i += mrc_insn_size2[i[1]] + 1;
      break;
    case OP_EXT3:
      i += mrc_insn_size3[i[1]] + 1;
      break;
    default:
      i += mrc_insn_size[insn];
      break;
    }
  }
  return prev_pc;
}

#define pc_addr(s) &((s)->iseq[(s)->pc])
#define addr_pc(s, addr) (uint32_t)((addr) - s->iseq)
#define rewind_pc(s) s->pc = s->lastpc

static struct mrc_insn_data
mrc_last_insn(mrc_codegen_scope *s)
{
  if (s->pc == 0) {
    struct mrc_insn_data data = { OP_NOP, 0 };
    return data;
  }
  return mrc_decode_insn(&s->iseq[s->lastpc]);
}

static mrc_bool
get_int_operand(mrc_codegen_scope *s, struct mrc_insn_data *data, mrc_int *n)
{
  switch (data->insn) {
  case OP_LOADI__1:
    *n = -1;
    return TRUE;

  case OP_LOADINEG:
    *n = -data->b;
    return TRUE;

  case OP_LOADI_0: case OP_LOADI_1: case OP_LOADI_2: case OP_LOADI_3:
  case OP_LOADI_4: case OP_LOADI_5: case OP_LOADI_6: case OP_LOADI_7:
    *n = data->insn - OP_LOADI_0;
    return TRUE;

  case OP_LOADI:
  case OP_LOADI16:
    *n = (int16_t)data->b;
    return TRUE;

  case OP_LOADI32:
    *n = (int32_t)((uint32_t)data->b<<16)+data->cc;
    return TRUE;

  case OP_LOADL:
    {
      // TODO
      //mrb_pool_value *pv = &s->pool[data->b];

      //if (pv->tt == IREP_TT_INT32) {
      //  *n = (mrc_int)pv->u.i32;
      //}
#ifdef MRC_INT64
      //else if (pv->tt == IREP_TT_INT64) {
      //  *n = (mrc_int)pv->u.i64;
      //}
#endif
      //else {
        return FALSE;
      //}
    }
    return TRUE;

  default:
    return FALSE;
  }
}

static mrc_bool
no_peephole(mrc_codegen_scope *s)
{
  return no_optimize(s) || s->lastlabel == s->pc || s->pc == 0 || s->pc == s->lastpc;
}

#define JMPLINK_START UINT32_MAX

static void
gen_jmpdst(mrc_codegen_scope *s, uint32_t pc)
{

  if (pc == JMPLINK_START) {
    pc = 0;
  }
  uint32_t pos2 = s->pc+2;
  int32_t off = pc - pos2;

  if (off > INT16_MAX || INT16_MIN > off) {
    codegen_error(s, "too big jump offset");
  }
  gen_S(s, (uint16_t)off);
}

static uint32_t
genjmp(mrc_codegen_scope *s, mrc_code i, uint32_t pc)
{
  uint32_t pos;

  genop_0(s, i);
  pos = s->pc;
  gen_jmpdst(s, pc);
  return pos;
}

#define genjmp_0(s,i) genjmp(s,i,JMPLINK_START)

static uint32_t
genjmp2(mrc_codegen_scope *s, mrc_code i, uint16_t a, uint32_t pc, int val)
{
  uint32_t pos;

  if (!no_peephole(s) && !val) {
    struct mrc_insn_data data = mrc_last_insn(s);

    switch (data.insn) {
    case OP_MOVE:
      if (data.a == a && data.a > s->nlocals) {
        rewind_pc(s);
        a = data.b;
      }
      break;
    case OP_LOADNIL:
    case OP_LOADF:
      if (data.a == a || data.a > s->nlocals) {
        s->pc = addr_pc(s, data.addr);
        if (i == OP_JMPNOT || (i == OP_JMPNIL && data.insn == OP_LOADNIL)) {
          return genjmp(s, OP_JMP, pc);
        }
        else {                  /* OP_JMPIF */
          return JMPLINK_START;
        }
      }
      break;
    case OP_LOADT: case OP_LOADI: case OP_LOADINEG: case OP_LOADI__1:
    case OP_LOADI_0: case OP_LOADI_1: case OP_LOADI_2: case OP_LOADI_3:
    case OP_LOADI_4: case OP_LOADI_5: case OP_LOADI_6: case OP_LOADI_7:
      if (data.a == a || data.a > s->nlocals) {
        s->pc = addr_pc(s, data.addr);
        if (i == OP_JMPIF) {
          return genjmp(s, OP_JMP, pc);
        }
        else {                  /* OP_JMPNOT and OP_JMPNIL */
          return JMPLINK_START;
        }
      }
      break;
    }
  }

  if (a > 0xff) {
    check_no_ext_ops(s, a, 0);
    gen_B(s, OP_EXT1);
    genop_0(s, i);
    gen_S(s, a);
  }
  else {
    genop_0(s, i);
    gen_B(s, (uint8_t)a);
  }
  pos = s->pc;
  gen_jmpdst(s, pc);
  return pos;
}

#define genjmp2_0(s,i,a,val) genjmp2(s,i,a,JMPLINK_START,val)

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
  if (val) push();
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
  if (no_peephole(s)) {
    genop_1(s, op, src);
  }
  else {
    struct mrc_insn_data data = mrc_last_insn(s);

    if (data.insn == OP_MOVE && src == data.a) {
      rewind_pc(s);
      genop_1(s, op, data.b);
    }
    else if (data.insn != OP_RETURN) {
      genop_1(s, op, src);
    }
  }
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
    //const char *filename = mrc_sym_name_len(fname, NULL);

    //mrc_debug_info_append_file(s->irep->debug_info,
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

static int
new_sym(mrc_codegen_scope *s, mrc_sym sym)
{
  int i, len;

  mrc_assert(s->irep);

  len = s->irep->slen;
  for (i=0; i<len; i++) {
    if (s->syms[i] == sym) return i;
  }
  if (s->irep->slen >= s->scapa) {
    s->scapa *= 2;
    if (s->scapa > 0xffff) {
      codegen_error(s, "too many symbols");
    }
    s->syms = (mrc_sym*)codegen_realloc(s, s->syms, sizeof(mrc_sym)*s->scapa);
  }
  s->syms[s->irep->slen] = sym;
  return s->irep->slen++;
}

static void
gen_addsub(mrc_codegen_scope *s, uint8_t op, uint16_t dst)
{
  if (no_peephole(s)) {
  normal:
    genop_1(s, op, dst);
    return;
  }
  else {
    struct mrc_insn_data data = mrc_last_insn(s);
    mrc_int n;

    if (!get_int_operand(s, &data, &n)) {
      /* not integer immediate */
      goto normal;
    }
    struct mrc_insn_data data0 = mrc_decode_insn(mrc_prev_pc(s, data.addr));
    mrc_int n0;
    if (addr_pc(s, data.addr) == s->lastlabel || !get_int_operand(s, &data0, &n0)) {
      /* OP_ADDI/OP_SUBI takes upto 8bits */
      if (n > INT8_MAX || n < INT8_MIN) goto normal;
      rewind_pc(s);
      if (n == 0) return;
      if (n > 0) {
        if (op == OP_ADD) genop_2(s, OP_ADDI, dst, (uint16_t)n);
        else genop_2(s, OP_SUBI, dst, (uint16_t)n);
      }
      else {                    /* n < 0 */
        n = -n;
        if (op == OP_ADD) genop_2(s, OP_SUBI, dst, (uint16_t)n);
        else genop_2(s, OP_ADDI, dst, (uint16_t)n);
      }
      return;
    }
    if (op == OP_ADD) {
//TODO
//      if (mrc_int_add_overflow(n0, n, &n)) goto normal;
    }
    else { /* OP_SUB */
//      if (mrc_int_sub_overflow(n0, n, &n)) goto normal;
    }
    s->pc = addr_pc(s, data0.addr);
    gen_int(s, dst, n);
  }
}

static void
gen_muldiv(mrc_codegen_scope *s, uint8_t op, uint16_t dst)
{
  if (no_peephole(s)) {
  normal:
    genop_1(s, op, dst);
    return;
  }
  else {
    struct mrc_insn_data data = mrc_last_insn(s);
    mrc_int n, n0;
    if (addr_pc(s, data.addr) == s->lastlabel || !get_int_operand(s, &data, &n)) {
      /* not integer immediate */
      goto normal;
    }
    struct mrc_insn_data data0 = mrc_decode_insn(mrc_prev_pc(s, data.addr));
    if (!get_int_operand(s, &data0, &n0)) {
      goto normal;
    }
    if (op == OP_MUL) {
      if (mrc_int_mul_overflow(n0, n, &n)) goto normal;
    }
    else { /* OP_DIV */
      if (n == 0) goto normal;
      if (n0 == MRC_INT_MIN && n == -1) goto normal;
      n = mrc_div_int(n0, n);
    }
    s->pc = addr_pc(s, data0.addr);
    gen_int(s, dst, n);
  }
}

static mrc_bool
gen_uniop(mrc_codegen_scope *s, mrc_sym sym, uint16_t dst)
{
  if (no_peephole(s)) return FALSE;
//  struct mrc_insn_data data = mrc_last_insn(s);
//  mrc_int n;
//
//  if (!get_int_operand(s, &data, &n)) return FALSE;
//  if (sym == MRC_OPSYM_2(plus)) {
//    /* unary plus does nothing */
//  }
//  else if (sym == MRC_OPSYM_2(minus)) {
//    if (n == MRC_INT_MIN) return FALSE;
//    n = -n;
//  }
//  else if (sym == MRC_OPSYM_2(neg)) {
//    n = ~n;
//  }
//  else {
//    return FALSE;
//  }
//  s->pc = addr_pc(s, data.addr);
//  gen_int(s, dst, n);
  return TRUE;
}

static mrc_bool
gen_binop(mrc_codegen_scope *s, mrc_sym op, uint16_t dst)
{
  if (no_peephole(s)) return FALSE;
  else if (op == MRC_OPSYM_2(aref)) {
    genop_1(s, OP_GETIDX, dst);
    return TRUE;
  }
  else {
    struct mrc_insn_data data = mrc_last_insn(s);
    mrc_int n, n0;
    if (addr_pc(s, data.addr) == s->lastlabel || !get_int_operand(s, &data, &n)) {
      /* not integer immediate */
      return FALSE;
    }
    struct mrc_insn_data data0 = mrc_decode_insn(mrc_prev_pc(s, data.addr));
    if (!get_int_operand(s, &data0, &n0)) {
      return FALSE;
    }
    if (op == MRC_OPSYM_2(lshift)) {
      if (!mrc_num_shift(n0, n, &n)) return FALSE;
    }
    else if (op == MRC_OPSYM_2(rshift)) {
      if (n == MRC_INT_MIN) return FALSE;
      if (!mrc_num_shift(n0, -n, &n)) return FALSE;
    }
    else if (op == MRC_OPSYM_2(mod) && n != 0) {
      if (n0 == MRC_INT_MIN && n == -1) {
        n = 0;
      }
      else {
        mrc_int n1 = n0 % n;
        if ((n0 < 0) != (n < 0) && n1 != 0) {
          n1 += n;
        }
        n = n1;
      }
    }
    else if (op == MRC_OPSYM_2(and)) {
      n = n0 & n;
    }
    else if (op == MRC_OPSYM_2(or)) {
      n = n0 | n;
    }
    else if (op == MRC_OPSYM_2(xor)) {
      n = n0 ^ n;
    }
    else {
      return FALSE;
    }
    s->pc = addr_pc(s, data0.addr);
    gen_int(s, dst, n);
    return TRUE;
  }
}

#define JMPLINK_START UINT32_MAX

static uint32_t
dispatch(mrc_codegen_scope *s, uint32_t pos0)
{
  int32_t pos1;
  int32_t offset;
  int16_t newpos;

  if (pos0 == JMPLINK_START) return 0;

  pos1 = pos0 + 2;
  offset = s->pc - pos1;
  if (offset > INT16_MAX) {
    codegen_error(s, "too big jmp offset");
  }
  s->lastlabel = s->pc;
  newpos = (int16_t)PEEK_S(s->iseq+pos0);
  emit_S(s, pos0, (uint16_t)offset);
  if (newpos == 0) return 0;
  return pos1+newpos;
}

#define CALL_MAXARGS 15
#define GEN_LIT_ARY_MAX 64
#define GEN_VAL_STACK_MAX 99

static int
gen_values(mrc_codegen_scope *s, mrc_node *tree, int val, int limit)
{
  pm_arguments_node_t *cast = (pm_arguments_node_t *)tree;
  mrc_node *t = (mrc_node *)cast->arguments.nodes[0];

  int n = 0;
  int first = 1;
  int slimit = GEN_VAL_STACK_MAX;

  if (limit == 0) limit = GEN_LIT_ARY_MAX;
  if (cursp() >= slimit) slimit = INT16_MAX;

  if (!val) {
    for (int i = 0; i < cast->arguments.size; i++) {
      t = (mrc_node *)cast->arguments.nodes[i];
      codegen(s, t, NOVAL);
      n++;
    }
    return n;
  }

  for (int i = 0; i < cast->arguments.size; i++) {
    t = (mrc_node *)cast->arguments.nodes[i];
    if (PM_NODE_TYPE(t) == PM_KEYWORD_HASH_NODE) break;
    int is_splat = PM_NODE_TYPE(t) == PM_SPLAT_NODE;

    if (is_splat || cursp() >= slimit) { /* flush stack */
      pop_n(n);
      if (first) {
        if (n == 0) {
          genop_1(s, OP_LOADNIL, cursp());
        }
        else {
          genop_2(s, OP_ARRAY, cursp(), n);
        }
        push();
        first = 0;
        limit = GEN_LIT_ARY_MAX;
      }
      else if (n > 0) {
        pop();
        genop_2(s, OP_ARYPUSH, cursp(), n);
        push();
      }
      n = 0;
    }
    if (is_splat) {
      pm_array_node_t *a = (pm_array_node_t *)((pm_splat_node_t *)t)->expression;
      codegen(s, (mrc_node *)a, VAL);
      pop(); pop();
      genop_1(s, OP_ARYCAT, cursp());
      push();
    }
    else {
      codegen(s, t, val);
      n++;
    }
  }
  if (!first) {
    pop();
    if (n > 0) {
      pop_n(n);
      genop_2(s, OP_ARYPUSH, cursp(), n);
    }
    return -1;                  /* variable length */
  }
  else if (n > limit) {
    pop_n(n);
    genop_2(s, OP_ARRAY, cursp(), n);
    return -1;
  }
  return n;
}


static int
gen_hash(mrc_codegen_scope *s, mrc_node *tree, int val, int limit)
{
  struct pm_node_list elements;
  if (PM_NODE_TYPE(tree) == PM_HASH_NODE) {
    pm_hash_node_t *cast = (pm_hash_node_t *)tree;
    elements = cast->elements;
  }
  else {
    pm_keyword_hash_node_t *cast = (pm_keyword_hash_node_t *)tree;
    elements = cast->elements;
  }

  int slimit = GEN_VAL_STACK_MAX;
  if (cursp() >= GEN_LIT_ARY_MAX) slimit = INT16_MAX;
  int len = 0;
  mrc_bool update = FALSE;
  mrc_bool first = TRUE;

  //while (tree) {
  for (int i = 0; i < elements.size; i++) {
    if (PM_NODE_TYPE(elements.nodes[i]) == PM_ASSOC_SPLAT_NODE) {
      pm_assoc_splat_node_t *assocsplat = (pm_assoc_splat_node_t *)elements.nodes[i];
      if (val && first) {
        genop_2(s, OP_HASH, cursp(), 0);
        push();
        update = TRUE;
      }
      else if (val && len > 0) {
        pop_n(len*2);
        if (!update) {
          genop_2(s, OP_HASH, cursp(), len);
        }
        else {
          pop();
          genop_2(s, OP_HASHADD, cursp(), len);
        }
        push();
      }
      codegen(s, assocsplat->value, val);
      if (val && (len > 0 || update)) {
        pop(); pop();
        genop_1(s, OP_HASHCAT, cursp());
        push();
      }
      update = TRUE;
      len = 0;
    }
    else {
      pm_assoc_node_t *assoc = (pm_assoc_node_t *)elements.nodes[i];
      codegen(s, assoc->key, val);
      codegen(s, assoc->value, val);
      len++;
    }
    if (val && cursp() >= slimit) {
      pop_n(len*2);
      if (!update) {
        genop_2(s, OP_HASH, cursp(), len);
      }
      else {
        pop();
        genop_2(s, OP_HASHADD, cursp(), len);
      }
      push();
      update = TRUE;
      len = 0;
    }
    first = FALSE;
  }
  if (val && len > limit) {
    pop_n(len*2);
    genop_2(s, OP_HASH, cursp(), len);
    push();
    return -1;
  }
  if (update) {
    if (val && len > 0) {
      pop_n(len*2+1);
      genop_2(s, OP_HASHADD, cursp(), len);
      push();
    }
    return -1;                  /* variable length */
  }
  return len;
}

static mrc_sym
nsym(mrc_parser_state *p, const uint8_t *start, size_t length)
{
  mrc_sym pm_sym = pm_constant_pool_find(&p->constant_pool, start, length);
  if (pm_sym == 0) {
    pm_sym = pm_constant_pool_insert_shared(&p->constant_pool, start, length);
  }
  return pm_sym;
}

static void
gen_call(mrc_codegen_scope *s, mrc_node *tree, int val, int safe)
{
  pm_call_node_t *cast = (pm_call_node_t *)tree;
  pm_constant_t *constant = pm_constant_pool_id_to_constant(&s->c->p->constant_pool, (const pm_constant_id_t)cast->name);
  mrc_sym sym = mrc_find_presym(constant->start, constant->length);
  mrc_sym pm_sym = 0;
  if (sym == 0) pm_sym = nsym(s->c->p, constant->start, constant->length);
  int skip = 0, n = 0, nk = 0, noop = no_optimize(s), noself = 0, blk = 0, sp_save = cursp();

  if (cast->receiver == NULL) {
    noself = noop = 1;
    push();
  }
  else {
    codegen(s, cast->receiver, VAL); /* receiver */
  }
  if (safe) {
    int recv = cursp()-1;
    gen_move(s, cursp(), recv, 1);
    skip = genjmp2_0(s, OP_JMPNIL, cursp(), val);
  }
  pm_arguments_node_t *arguments = (pm_arguments_node_t *)cast->arguments;
  if (arguments) {
    if (0 < arguments->arguments.size) {            /* positional arguments */
      n = gen_values(s, (mrc_node *)arguments, VAL, 14);
      if (n < 0) {              /* variable length */
        noop = 1;               /* not operator */
        n = 15;
        push();
      }
    }
    for (int i = 0; i < arguments->arguments.size; i++) {
      mrc_node *t = (mrc_node *)arguments->arguments.nodes[i];
      if (PM_NODE_TYPE(t) == PM_KEYWORD_HASH_NODE) {       /* keyword arguments */
        noop = 1;
        nk = gen_hash(s, t, VAL, 14);
        if (nk < 0) nk = 15;
      }
    }
  }
  if (cast->block) {
    codegen(s, cast->block, VAL);
    pop();
    noop = 1;
    blk = 1;
  }
  push();pop();
  s->sp = sp_save;
  if (!noop && sym == MRC_OPSYM_2(add) && n == 1)  {
    gen_addsub(s, OP_ADD, cursp());
  }
  else if (!noop && sym == MRC_OPSYM_2(sub) && n == 1)  {
    gen_addsub(s, OP_SUB, cursp());
  }
  else if (!noop && sym == MRC_OPSYM_2(mul) && n == 1)  {
    gen_muldiv(s, OP_MUL, cursp());
  }
  else if (!noop && sym == MRC_OPSYM_2(div) && n == 1)  {
    gen_muldiv(s, OP_DIV, cursp());
  }
  else if (!noop && sym == MRC_OPSYM_2(lt) && n == 1)  {
    genop_1(s, OP_LT, cursp());
  }
  else if (!noop && sym == MRC_OPSYM_2(le) && n == 1)  {
    genop_1(s, OP_LE, cursp());
  }
  else if (!noop && sym == MRC_OPSYM_2(gt) && n == 1)  {
    genop_1(s, OP_GT, cursp());
  }
  else if (!noop && sym == MRC_OPSYM_2(ge) && n == 1)  {
    genop_1(s, OP_GE, cursp());
  }
  else if (!noop && sym == MRC_OPSYM_2(eq) && n == 1)  {
    genop_1(s, OP_EQ, cursp());
  }
  else if (!noop && sym == MRC_OPSYM_2(aset) && n == 2)  {
    genop_1(s, OP_SETIDX, cursp());
  }
  else if (!noop && n == 0 && gen_uniop(s, sym, cursp())) {
    /* constant folding succeeded */
  }
  else if (!noop && n == 1 && gen_binop(s, sym, cursp())) {
    /* constant folding succeeded */
  }
  else if (noself){
    genop_3(s, blk ? OP_SSENDB : OP_SSEND, cursp(), new_sym(s, pm_sym), n|(nk<<4));
  }
  else {
    genop_3(s, blk ? OP_SENDB : OP_SEND, cursp(), new_sym(s, pm_sym), n|(nk<<4));
  }
  if (safe) {
    dispatch(s, skip);
  }
  if (val) {
    push();
  }
}

static mrc_pool_value*
lit_pool_extend(mrc_codegen_scope *s)
{
  if (s->irep->plen == s->pcapa) {
    s->pcapa *= 2;
    s->pool = (mrc_pool_value*)codegen_realloc(s, s->pool, sizeof(mrc_pool_value)*s->pcapa);
  }

  return &s->pool[s->irep->plen++];
}

static int
new_lit_str(mrc_codegen_scope *s, const char *str, mrc_int len)
{
  int i;
  mrc_pool_value *pv;

  for (i=0; i<s->irep->plen; i++) {
    pv = &s->pool[i];
    if (pv->tt & IREP_TT_NFLAG) continue;
    mrc_int plen = pv->tt>>2;
    if (len != plen) continue;
    if (memcmp(pv->u.str, str, plen) == 0)
      return i;
  }

  pv = lit_pool_extend(s);

  //if (mrb_ro_data_p(str)) {
  //  pv->tt = (uint32_t)(len<<2) | IREP_TT_SSTR;
  //  pv->u.str = str;
  //}
  //else {
    char *p;
    pv->tt = (uint32_t)(len<<2) | IREP_TT_STR;
    p = (char*)codegen_realloc(s, NULL, len+1);
    memcpy(p, str, len);
    p[len] = '\0';
    pv->u.str = p;
  //}

  return i;
}
static void
codegen(mrc_codegen_scope *s, mrc_node *tree, int val)
{
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

//  s->lineno = tree->lineno;
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
      break;
    }
    case PM_INTEGER_NODE:
    {
      pm_integer_node_t *cast = (pm_integer_node_t *)tree;
      // todo: check overflow
      gen_int(s, cursp(), cast->value.head.value); // todo: negative or big integer
      push();
      break;
    }
    case PM_CALL_NODE:
    {
      pm_call_node_t *cast = (pm_call_node_t *)tree;
      gen_call(s, tree, val, (cast->base.flags & PM_CALL_NODE_FLAGS_SAFE_NAVIGATION) ? 1 : 0);
      break;
    }
    case PM_ARRAY_NODE:
    {
      int n;
      n = gen_values(s, tree, val, 0);
      if (val) {
        if (n >= 0) {
          pop_n(n);
          genop_2(s, OP_ARRAY, cursp(), n);
        }
        push();
      }
      break;
    }
    case PM_SYMBOL_NODE:
    {
      if (val) {
        pm_symbol_node_t *cast = (pm_symbol_node_t *)tree;
        int sym = new_sym(s, nsym(s->c->p, cast->unescaped.source, cast->unescaped.length));

        genop_2(s, OP_LOADSYM, cursp(), sym);
        push();
      }
      break;
    }
    case PM_KEYWORD_HASH_NODE:
    case PM_HASH_NODE:
    {
      int nk = gen_hash(s, tree, val, GEN_LIT_ARY_MAX);
      if (val && nk >= 0) {
        pop_n(nk*2);
        genop_2(s, OP_HASH, cursp(), nk);
        push();
      }
      break;
    }
    case PM_STRING_NODE:
    {
      if (val) {
        pm_string_node_t *cast = (pm_string_node_t *)tree;
        char *p = (char*)cast->unescaped.source;
        mrc_int len = cast->unescaped.length;
        int off = new_lit_str(s, p, len);

        genop_2(s, OP_STRING, cursp(), off);
        push();
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

static mrc_irep *
generate_code(mrc_ccontext *c, mrc_node *node, int val)
{
  // FIXME: memory leak of scope
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
