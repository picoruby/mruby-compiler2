
#include <string.h>
#include "../include/mrc_irep.h"
#include "../include/mrc_ccontext.h"
#include "../include/mrc_parser_util.h"
#include "../include/mrc_throw.h"
#include "../include/mrc_opcode.h"
#include "../include/mrc_presym.h"
#include "../include/mrc_pool.h"
#include "../include/mrc_dump.h"
#include "../include/mrc_debug.h"

#if defined(PICORB_VM_MRUBY)
#include "../include/mrc_proc.h"
#endif

#ifdef MRBC_REQUIRE_32BIT_ALIGNMENT
#include <mrubyc.h>
#define printf(...) console_printf(__VA_ARGS__)
#else
#define printf(n) ((void)0)
#endif

#if defined(MRC_INT64)
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

/**
 * Function requires n arguments.
 *
 * @param n
 *      The number of required arguments.
 */
#define MRC_ARGS_REQ(n)     ((mrc_aspec)((n)&0x1f) << 18)

/**
 * Function takes n optional arguments
 *
 * @param n
 *      The number of optional arguments.
 */
#define MRC_ARGS_OPT(n)     ((mrc_aspec)((n)&0x1f) << 13)

/**
 * Function takes n1 mandatory arguments and n2 optional arguments
 *
 * @param n1
 *      The number of required arguments.
 * @param n2
 *      The number of optional arguments.
 */
#define MRC_ARGS_ARG(n1,n2)   (MRC_ARGS_REQ(n1)|MRC_ARGS_OPT(n2))

/** rest argument */
#define MRC_ARGS_REST()     ((mrc_aspec)(1 << 12))

/** required arguments after rest */
#define MRC_ARGS_POST(n)    ((mrc_aspec)((n)&0x1f) << 7)

/** keyword arguments (n of keys, kdict) */
#define MRC_ARGS_KEY(n1,n2) ((mrc_aspec)((((n1)&0x1f) << 2) | ((n2)?(1<<1):0)))

/**
 * Function takes a block argument
 */
#define MRC_ARGS_BLOCK()    ((mrc_aspec)1)

/**
 * Function accepts any number of arguments
 */
#define MRC_ARGS_ANY()      MRC_ARGS_REST()

/**
 * Function accepts no arguments
 */
#define MRC_ARGS_NONE()     ((mrc_aspec)0)


#define MRC_INT_OVERFLOW_MASK ((mrc_uint)1 << (MRC_INT_BIT - 1))

static inline mrc_bool
mrc_int_add_overflow(mrc_int a, mrc_int b, mrc_int *c)
{
  mrc_uint x = (mrc_uint)a;
  mrc_uint y = (mrc_uint)b;
  mrc_uint z = (mrc_uint)(x + y);
  *c = (mrc_int)z;
  return !!(((x ^ z) & (y ^ z)) & MRC_INT_OVERFLOW_MASK);
}

static inline mrc_bool
mrc_int_sub_overflow(mrc_int a, mrc_int b, mrc_int *c)
{
  mrc_uint x = (mrc_uint)a;
  mrc_uint y = (mrc_uint)b;
  mrc_uint z = (mrc_uint)(x - y);
  *c = (mrc_int)z;
  return !!(((x ^ z) & (~y ^ z)) & MRC_INT_OVERFLOW_MASK);
}

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
  mrc_pool *mpool; // -> *page

  struct scope *prev;

  mrc_constant_id_list *lv;

  uint16_t sp;
  uint32_t pc;
  uint32_t lastpc;
  uint32_t lastlabel;
  uint16_t ainfo:15;
  mrc_bool mscope:1;

  struct loopinfo *loop;
  //mrc_sym filename_sym;
  const char *filename;
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
  int ai;

  int debug_start_pos;
  uint16_t filename_index;
  mrc_ccontext* c;

  int rlev;                     /* recursion levels */
} mrc_codegen_scope;

static void codegen(mrc_codegen_scope *s, mrc_node *tree, int val);

static void
codegen_error(mrc_codegen_scope *s, const char *message)
{
  if (!s) return;
  s->c->capture_errors = TRUE;

  mrc_diagnostic_list_append(s->c, 0, message, MRC_GENERATOR_ERROR);

#ifndef MRC_NO_STDIO
  if (s->filename && s->lineno) {
    const char *filename = (const char *)s->filename;
    fprintf(stderr, "%s:%d: %s\n", filename, s->lineno, message);
  }
  else {
    fprintf(stderr, "%s\n", message);
  }

#endif
  while (s->prev) {
    mrc_codegen_scope *tmp = s->prev;
    if (s->irep) {
      mrc_free(s->c, s->iseq);
      for (int i=0; i<s->irep->plen; i++) {
        mrc_pool_value *pv = &s->pool[i];
        if ((pv->tt & 0x3) == IREP_TT_STR || pv->tt == IREP_TT_BIGINT) {
          mrc_free(s->c, (void*)pv->u.str);
        }
      }
      mrc_free(s->c, s->pool);
      mrc_free(s->c, s->syms);
      mrc_free(s->c, s->catch_table);
      if (s->reps) {
        /* copied from mrc_irep_free() in state.c */
        //for (int i=0; i<s->irep->rlen; i++) {
        //  if (s->reps[i])
        //    mrc_irep_decref(s->mrb, (mrc_irep*)s->reps[i]);
        //}
        mrc_free(s->c, s->reps);
      }
      mrc_free(s->c, s->lines);
    }
    mrc_pool_close(s->mpool);
    s = tmp;
  }
  MRC_THROW(s->c->jmp);
}

static void*
codegen_palloc(mrc_codegen_scope *s, size_t len)
{
  void *p = mrc_pool_alloc(s->mpool, len);

  if (!p) codegen_error(s, "pool memory allocation");
  return p;
}

static void*
codegen_realloc(mrc_codegen_scope *s, void *p, size_t oldlen, size_t newlen)
{
  p = mrc_pool_realloc(s->mpool, p, oldlen, newlen);

  if (!p && 0 < newlen) codegen_error(s, "pool memory reallocation");
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
    s->iseq = (mrc_code*)mrc_realloc(s->c, s->iseq, sizeof(mrc_code)*s->icapa);
    if (s->lines) {
      s->lines = (uint16_t*)mrc_realloc(s->c, s->lines, sizeof(uint16_t)*s->icapa);
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
mrc_add_irep(mrc_ccontext *c)
{
  static const mrc_irep mrc_irep_zero = { 0 };
  mrc_irep *irep = (mrc_irep *)mrc_malloc(c, sizeof(mrc_irep));
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
    irep = mrc_add_irep(s->c);
    prev->irep = s->irep = irep;
    return;
  }
  else {
    if (prev->irep->rlen == UINT16_MAX) {
      codegen_error(s, "too many nested blocks/methods");
    }
    s->irep = irep = mrc_add_irep(s->c);
    if (prev->irep->rlen == prev->rcapa) {
      prev->rcapa *= 2;
      prev->reps = (mrc_irep **)mrc_realloc(s->c, prev->reps, sizeof(mrc_irep *)*prev->rcapa);
    }
    prev->reps[prev->irep->rlen++] = irep;
  }
}

static mrc_codegen_scope *
scope_new(mrc_ccontext *c, mrc_codegen_scope *prev, mrc_constant_id_list *nlv)
{
  static const mrc_codegen_scope codegen_scope_zero = { 0 };
  mrc_pool *pool = mrc_pool_open(c);
  mrc_codegen_scope *s = (mrc_codegen_scope *)mrc_pool_alloc(pool, sizeof(mrc_codegen_scope));
  if (!s) {
    if (prev)
      codegen_error(prev, "unexpected scope");
    return NULL;
  }
  *s = codegen_scope_zero;
  if (prev) {
    s->c = prev->c;
  } else {
    s->c = c;
  }
  s->mpool = pool;
  if (!prev) return s;
  s->prev = prev;
  s->ainfo = 0;
  s->mscope = 0;

  scope_add_irep(s);

  s->rcapa = 8;
  s->reps = (mrc_irep **)mrc_malloc(c, sizeof(mrc_irep *)*s->rcapa);
  s->icapa = 1024;
  s->iseq = (mrc_code *)mrc_malloc(c, sizeof(mrc_code)*s->icapa);
  s->pcapa = 32;
  s->pool = (mrc_pool_value *)mrc_malloc(c, sizeof(mrc_pool_value)*s->pcapa);
  s->scapa = 256;
  s->syms = (mrc_sym *)mrc_malloc(c, sizeof(mrc_sym)*s->scapa);
  assert(nlv != NULL); // `if (!prev) return s;` prevents this from being NULL
  s->lv = nlv;

  s->sp += nlv->size + 1; // add self
  s->nlocals = s->nregs = s->sp;
  if (nlv) {
    mrc_sym *lv;
    size_t size = sizeof(mrc_sym) * nlv->size;
    if (0 < size) {
      s->irep->lv = lv = (mrc_sym *)mrc_malloc(c, sizeof(mrc_sym) * (s->nlocals - 1));
      memcpy(lv, nlv->ids, size);
    }
    else {
      s->irep->lv = lv = NULL;
    }
    mrc_assert(nlv->size < UINT16_MAX);
  }

  int ai = mrc_gc_arena_save(c);
  s->ai = ai;
  s->filename = prev->filename;
  if (s->filename) {
    s->lines = (uint16_t *)mrc_malloc(c, sizeof(uint16_t)*s->icapa);
  }
  s->lineno = prev->lineno;

  /* degug info */
  s->debug_start_pos = 0;
  if (s->filename) {
    mrc_debug_info_alloc(c, s->irep);
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
    struct mrc_insn_data data = { OP_NOP, 0, 0, 0, NULL };
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
      mrc_pool_value *pv = &s->pool[data->b];

      if (pv->tt == IREP_TT_INT32) {
        *n = (mrc_int)pv->u.i32;
      }
#ifdef MRC_INT64
      else if (pv->tt == IREP_TT_INT64) {
        *n = (mrc_int)pv->u.i64;
      }
#endif
      else {
        return FALSE;
      }
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

static int new_lit_int(mrc_codegen_scope *s, mrc_int num);

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
    genop_2(s, OP_LOADL, dst, new_lit_int(s, i));
  }
}

static void
gen_move(mrc_codegen_scope *s, uint16_t dst, uint16_t src, int nopeep)
{
  if (nopeep || no_peephole(s)) goto normal;
  else if (dst == src) return;
  else {
    struct mrc_insn_data data = mrc_last_insn(s);

    switch (data.insn) {
    case OP_MOVE:
      if (dst == src) return;   /* remove useless MOVE */
      if (data.a == src) {
        if (data.b == dst)      /* skip swapping MOVE */
          return;
        if (data.a < s->nlocals) goto normal;
        rewind_pc(s);
        s->lastpc = addr_pc(s, mrc_prev_pc(s, data.addr));
        gen_move(s, dst, data.b, FALSE);
        return;
      }
      if (dst == data.a) {      /* skip overwritten move */
        rewind_pc(s);
        s->lastpc = addr_pc(s, mrc_prev_pc(s, data.addr));
        gen_move(s, dst, src, FALSE);
        return;
      }
      goto normal;
    case OP_LOADNIL: case OP_LOADSELF: case OP_LOADT: case OP_LOADF:
    case OP_LOADI__1:
    case OP_LOADI_0: case OP_LOADI_1: case OP_LOADI_2: case OP_LOADI_3:
    case OP_LOADI_4: case OP_LOADI_5: case OP_LOADI_6: case OP_LOADI_7:
      if (data.a != src || data.a < s->nlocals) goto normal;
      rewind_pc(s);
      genop_1(s, data.insn, dst);
      return;
    case OP_HASH:
      if (data.b != 0) goto normal;
      /* fall through */
    case OP_LOADI: case OP_LOADINEG:
    case OP_LOADL: case OP_LOADSYM:
    case OP_GETGV: case OP_GETSV: case OP_GETIV: case OP_GETCV:
    case OP_GETCONST: case OP_STRING:
    case OP_LAMBDA: case OP_BLOCK: case OP_METHOD: case OP_BLKPUSH:
      if (data.a != src || data.a < s->nlocals) goto normal;
      rewind_pc(s);
      genop_2(s, data.insn, dst, data.b);
      return;
    case OP_LOADI16:
      if (data.a != src || data.a < s->nlocals) goto normal;
      rewind_pc(s);
      genop_2S(s, data.insn, dst, data.b);
      return;
    case OP_LOADI32:
      if (data.a != src || data.a < s->nlocals) goto normal;
      else {
        uint32_t i = (uint32_t)data.b<<16|data.cc;
        rewind_pc(s);
        genop_2SS(s, data.insn, dst, i);
      }
      return;
    case OP_ARRAY:
      if (data.a != src || data.a < s->nlocals || data.a < dst) goto normal;
      rewind_pc(s);
      if (data.b == 0 || dst == data.a)
        genop_2(s, OP_ARRAY, dst, 0);
      else
        genop_3(s, OP_ARRAY2, dst, data.a, data.b);
      return;
    case OP_ARRAY2:
      if (data.a != src || data.a < s->nlocals || data.a < dst) goto normal;
      rewind_pc(s);
      genop_3(s, OP_ARRAY2, dst, data.b, data.cc);
      return;
    case OP_AREF:
    case OP_GETUPVAR:
      if (data.a != src || data.a < s->nlocals) goto normal;
      rewind_pc(s);
      genop_3(s, data.insn, dst, data.b, data.cc);
      return;
    case OP_ADDI: case OP_SUBI:
      if (addr_pc(s, data.addr) == s->lastlabel || data.a != src || data.a < s->nlocals) goto normal;
      else {
        struct mrc_insn_data data0 = mrc_decode_insn(mrc_prev_pc(s, data.addr));
        if (data0.insn != OP_MOVE || data0.a != data.a || data0.b != dst) goto normal;
        s->pc = addr_pc(s, data0.addr);
        if (addr_pc(s, data0.addr) != s->lastlabel) {
          /* constant folding */
          data0 = mrc_decode_insn(mrc_prev_pc(s, data0.addr));
          mrc_int n;
          if (data0.a == dst && get_int_operand(s, &data0, &n)) {
            if ((data.insn == OP_ADDI && !mrc_int_add_overflow(n, data.b, &n)) ||
                (data.insn == OP_SUBI && !mrc_int_sub_overflow(n, data.b, &n))) {
              s->pc = addr_pc(s, data0.addr);
              gen_int(s, dst, n);
              return;
            }
          }
        }
      }
      genop_2(s, data.insn, dst, data.b);
      return;
    default:
      break;
    }
  }
 normal:
  genop_2(s, OP_MOVE, dst, src);
  return;
}

static int
lv_idx(mrc_codegen_scope *s, mrc_sym id)
{
  if (!s->lv) return 0;
  for (size_t n = 0; n < s->lv->size; n++) {
    if (s->lv->ids[n] == id) return n + 1;
  }
  return 0;
}


#define MRC_PROC_CFUNC_FL 128
#define MRC_PROC_CFUNC_P(p) (((p)->flags & MRC_PROC_CFUNC_FL) != 0)
#define MRC_PROC_SCOPE 2048
#define MRC_PROC_SCOPE_P(p) (((p)->flags & MRC_PROC_SCOPE) != 0)

static int
search_upvar(mrc_codegen_scope *s, mrc_sym id, int *idx)
{
  int lv = 0;
  mrc_codegen_scope *up = s->prev;

  while (up) {
    *idx = lv_idx(up, id);
    if (*idx > 0) {
      return lv;
    }
    lv++;
    up = up->prev;
  }

#if defined(PICORB_VM_MRUBY)
  const struct RProc *u;

  if (lv < 1) lv = 1;
  u = s->c->upper;
  pm_constant_t *constant = pm_constant_pool_id_to_constant(&s->c->p->constant_pool, id);
  mrc_sym intern = mrb_intern(s->c->mrb, (const char *)constant->start, constant->length);
  if (0 < intern) {
    while (u && !MRC_PROC_CFUNC_P(u)) {
      const struct mrc_irep *ir = u->body.irep;
      uint_fast16_t n = ir->nlocals;
      int i;
      const mrc_sym *v = ir->lv;
      if (v) {
        for (i=1; n > 1; n--, v++, i++) {
          if (*v == intern) {
            *idx = i;
            return lv - 1;
          }
        }
      }
      if (MRC_PROC_SCOPE_P(u)) break;
      u = u->upper;
      lv++;
    }
  }
#endif

  if (id == MRC_OPSYM_2(and)) {
    codegen_error(s, "No anonymous block parameter");
  }
  else if (id == MRC_OPSYM_2(mul)) {
    codegen_error(s, "No anonymous rest parameter");
  }
  else if (id == MRC_OPSYM_2(pow)) {
    codegen_error(s, "No anonymous keyword rest parameter");
  }
  else {
    codegen_error(s, "Can't find local variables");
  }
  return -1; /* not reached */
}

static void
gen_getupvar(mrc_codegen_scope *s, uint16_t dst, mrc_sym id, int depth)
{
  int idx;
  int lv = search_upvar(s, id, &idx);

  mrc_assert(lv == depth-1);

  if (!no_peephole(s)) {
    struct mrc_insn_data data = mrc_last_insn(s);
    if (data.insn == OP_SETUPVAR && data.a == dst && data.b == idx && data.cc == lv) {
      /* skip GETUPVAR right after SETUPVAR */
      return;
    }
  }
  genop_3(s, OP_GETUPVAR, dst, idx, lv);
}

static void
gen_setupvar(mrc_codegen_scope *s, uint16_t dst, mrc_sym id, int depth)
{
  int idx;
  int lv = search_upvar(s, id, &idx);

  mrc_assert(lv == depth-1);

  if (!no_peephole(s)) {
    struct mrc_insn_data data = mrc_last_insn(s);
    if (data.insn == OP_MOVE && data.a == dst) {
      dst = data.b;
      rewind_pc(s);
    }
  }
  genop_3(s, OP_SETUPVAR, dst, idx, lv);
}

static void
gen_return(mrc_codegen_scope *s, uint8_t op, uint16_t src)
{
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

static void*
simple_realloc(mrc_ccontext *c, void *p, size_t len)
{
  if (len == 0) return p;
  return mrc_realloc(c, p, len);
}

static const char*
mrc_parser_get_filename(mrc_ccontext *c, uint16_t idx) {
  if (idx >= c->filename_table_length) return 0;
  else {
    return c->filename_table[idx].filename;
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
    size_t catchsize = sizeof(struct mrc_irep_catch_handler) * irep->clen;
    irep->iseq = (const mrc_code *)mrc_realloc(s->c, s->iseq, sizeof(mrc_code)*s->pc + catchsize);
    irep->ilen = s->pc;
    if (0 < irep->clen) {
      memcpy((void *)(irep->iseq + irep->ilen), s->catch_table, catchsize);
    }
  }
  else {
    irep->clen = 0;
  }
  mrc_free(s->c, s->catch_table);
  s->catch_table = NULL;
  irep->pool = (const mrc_pool_value *)simple_realloc(s->c, s->pool, sizeof(mrc_pool_value)*irep->plen);
  irep->syms = (const mrc_sym *)simple_realloc(s->c, s->syms, sizeof(mrc_sym)*irep->slen);
  irep->reps = (const mrc_irep **)simple_realloc(s->c, s->reps, sizeof(mrc_irep *)*irep->rlen);
  if (s->filename) {
    const char *filename = mrc_parser_get_filename(s->c, s->filename_index);
    mrc_debug_info_append_file(s->c, s->irep->debug_info,
                               filename, s->lines, s->debug_start_pos, s->pc);
  }
  mrc_free(s->c, s->lines);
  irep->nlocals = s->nlocals;
  irep->nregs = s->nregs;

  mrc_gc_arena_restore(s->c, s->ai);
  mrc_pool_close(s->mpool);
}

static mrc_pool_value*
lit_pool_extend(mrc_codegen_scope *s)
{
  if (s->irep->plen == s->pcapa) {
    s->pcapa *= 2;
    s->pool = (mrc_pool_value*)mrc_realloc(s->c, s->pool, sizeof(mrc_pool_value)*s->pcapa);
  }

  return &s->pool[s->irep->plen++];
}

#ifndef MRC_NO_FLOAT
static int
new_lit_float(mrc_codegen_scope *s, mrc_float num)
{
  int i;
  mrc_pool_value *pv;

  for (i=0; i<s->irep->plen; i++) {
    mrc_float f;
    pv = &s->pool[i];
    if (pv->tt != IREP_TT_FLOAT) continue;
    f = pv->u.f;
    if (f == num && !signbit(f) == !signbit(num)) return i;
  }

  pv = lit_pool_extend(s);

  pv->tt = IREP_TT_FLOAT;
  pv->u.f = num;

  return i;
}
#endif

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
    s->syms = (mrc_sym*)mrc_realloc(s->c, s->syms, sizeof(mrc_sym)*s->scapa);
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
      if (mrc_int_add_overflow(n0, n, &n)) goto normal;
    }
    else { /* OP_SUB */
      if (mrc_int_sub_overflow(n0, n, &n)) goto normal;
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
  struct mrc_insn_data data = mrc_last_insn(s);
  mrc_int n;

  if (!get_int_operand(s, &data, &n)) return FALSE;
  if (sym == MRC_OPSYM_2(add)) {
    /* unary plus does nothing */
  }
  else if (sym == MRC_OPSYM_2(sub)) {
    if (n == MRC_INT_MIN) return FALSE;
    n = -n;
  }
  else if (sym == MRC_OPSYM_2(neg)) {
    n = ~n;
  }
  else {
    return FALSE;
  }
  s->pc = addr_pc(s, data.addr);
  gen_int(s, dst, n);
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

static void
dispatch_linked(mrc_codegen_scope *s, uint32_t pos)
{
  if (pos==JMPLINK_START) return;
  for (;;) {
    pos = dispatch(s, pos);
    if (pos==0) break;
  }
}

static int
new_litbint(mrc_codegen_scope *s, const char *p, int base, mrc_bool neg)
{
  int i;
  size_t plen;
  mrc_pool_value *pv;

  plen = strlen(p);
  if (plen > 255) {
    codegen_error(s, "integer too big");
  }
  for (i=0; i<s->irep->plen; i++) {
    size_t len;
    pv = &s->pool[i];
    if (pv->tt != IREP_TT_BIGINT) continue;
    len = pv->u.str[0];
    if (len == plen && pv->u.str[1] == base && memcmp(pv->u.str+2, p, len) == 0)
      return i;
  }

  pv = lit_pool_extend(s);

  char *buf;
  pv->tt = IREP_TT_BIGINT;
  buf = (char*)mrc_realloc(s->c, NULL, plen+3);
  buf[0] = (char)plen;
  if (neg) buf[1] = -base;
  else buf[1] = base;
  memcpy(buf+2, p, plen);
  buf[plen+2] = '\0';
  pv->u.str = buf;

  return i;
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
    p = (char*)mrc_realloc(s->c, NULL, len+1);
    memcpy(p, str, len);
    p[len] = '\0';
    pv->u.str = p;
  //}

  return i;
}

static int
new_lit_cstr(mrc_codegen_scope *s, const char *str)
{
  return new_lit_str(s, str, (mrc_int)strlen(str));
}

static int
new_lit_int(mrc_codegen_scope *s, mrc_int num)
{
  int i;
  mrc_pool_value *pv;

  for (i=0; i<s->irep->plen; i++) {
    pv = &s->pool[i];
    if (pv->tt == IREP_TT_INT32) {
      if (num == pv->u.i32) return i;
    }
#ifdef MRC_64BIT
    else if (pv->tt == IREP_TT_INT64) {
      if (num == pv->u.i64) return i;
    }
    continue;
#endif
  }

  pv = lit_pool_extend(s);

#ifdef MRC_INT64
  pv->tt = IREP_TT_INT64;
  pv->u.i64 = num;
#else
  pv->tt = IREP_TT_INT32;
  pv->u.i32 = num;
#endif

  return i;
}

static int
catch_handler_new(mrc_codegen_scope *s)
{
  size_t newsize = sizeof(struct mrc_irep_catch_handler) * (s->irep->clen + 1);
  s->catch_table = (struct mrc_irep_catch_handler*)mrc_realloc(s->c, (void*)s->catch_table, newsize);
  return s->irep->clen++;
}

static void
catch_handler_set(mrc_codegen_scope *s, int ent, enum mrc_catch_type type, uint32_t begin, uint32_t end, uint32_t target)
{
  struct mrc_irep_catch_handler *e;
  mrc_assert(ent >= 0 && ent < s->irep->clen);
  e = &s->catch_table[ent];
  mrc_uint8_to_bin(type, &e->type);
  mrc_irep_catch_handler_pack(begin, e->begin);
  mrc_irep_catch_handler_pack(end, e->end);
  mrc_irep_catch_handler_pack(target, e->target);
}

static void
raise_error(mrc_codegen_scope *s, const char *msg)
{
  int idx = new_lit_cstr(s, msg);

  genop_1(s, OP_ERR, idx);
}

static struct loopinfo*
loop_push(mrc_codegen_scope *s, enum looptype t)
{
  struct loopinfo *p = (struct loopinfo*)codegen_palloc(s, sizeof(struct loopinfo));

  p->type = t;
  p->pc0 = p->pc1 = p->pc2 = JMPLINK_START;
  p->prev = s->loop;
  p->reg = cursp();
  s->loop = p;

  return p;
}

// Implementation in codegen_prism.inc
static void gen_retval(mrc_codegen_scope *s, mrc_node *tree);

static void
loop_break(mrc_codegen_scope *s, mrc_node *tree)
{
  if (!s->loop) {
    codegen(s, tree, NOVAL);
    raise_error(s, "unexpected break");
  }
  else {
    struct loopinfo *loop;

    loop = s->loop;
    if (tree) {
      if (loop->reg < 0) {
        codegen(s, tree, NOVAL);
      }
      else {
        gen_retval(s, tree);
      }
    }
    while (loop) {
      if (loop->type == LOOP_BEGIN) {
        loop = loop->prev;
      }
      else if (loop->type == LOOP_RESCUE) {
        loop = loop->prev;
      }
      else{
        break;
      }
    }
    if (!loop) {
      raise_error(s, "unexpected break");
      return;
    }

    if (loop->type == LOOP_NORMAL) {
      int tmp;

      if (loop->reg >= 0) {
        if (tree) {
          gen_move(s, loop->reg, cursp(), 0);
        }
        else {
          genop_1(s, OP_LOADNIL, loop->reg);
        }
      }
      tmp = genjmp(s, OP_JMPUW, loop->pc2);
      loop->pc2 = tmp;
    }
    else {
      if (!tree) {
        genop_1(s, OP_LOADNIL, cursp());
      }
      gen_return(s, OP_BREAK, cursp());
    }
  }
}

static void
loop_pop(mrc_codegen_scope *s, int val)
{
  if (val) {
    genop_1(s, OP_LOADNIL, cursp());
  }
  dispatch_linked(s, s->loop->pc2);
  s->loop = s->loop->prev;
  if (val) push();
}

static void
gen_blkmove(mrc_codegen_scope *s, uint16_t ainfo, int lv)
{
  int m1 = (ainfo>>7)&0x3f;
  int r  = (ainfo>>6)&0x1;
  int m2 = (ainfo>>1)&0x1f;
  int kd = (ainfo)&0x1;
  int off = m1+r+m2+kd+1;
  if (lv == 0) {
    gen_move(s, cursp(), off, 0);
  }
  else {
    genop_3(s, OP_GETUPVAR, cursp(), off, lv);
  }
  push();
}

static void
gen_setxv(mrc_codegen_scope *s, uint8_t op, uint16_t dst, mrc_sym sym, int val)
{
  int idx = new_sym(s, sym);
  if (!val && !no_peephole(s)) {
    struct mrc_insn_data data = mrc_last_insn(s);
    if (data.insn == OP_MOVE && data.a == dst) {
      dst = data.b;
      rewind_pc(s);
    }
  }
  genop_2(s, op, dst, idx);
}

static mrc_irep *
generate_code(mrc_ccontext *c, mrc_node *node, int val)
{
  mrc_codegen_scope *scope = scope_new(c, NULL, NULL);
  struct mrc_jmpbuf *prev_jmp = c->jmp;
  struct mrc_jmpbuf jmpbuf;

  c->jmp = &jmpbuf;

  scope->c = c;
  scope->filename_index = 0;
  scope->filename = (const char *)c->filename_table[0].filename;

  MRC_TRY(c->jmp) {
    codegen(scope, node, val);
    // TODO: mrc_ccontext has an upper Proc if MRC_TARGET_MRUBY
    //proc->c = NULL;
    //if (mrb->c->cibase && mrb->c->cibase->proc == proc->upper) {
    //  proc->upper = NULL;
    //}
    //mrc_irep_free(c, scope->irep);
    mrc_irep *irep = scope->irep;
    mrc_pool_close(scope->mpool);
    c->jmp = prev_jmp;
    return irep;
  }
  MRC_CATCH(c->jmp) {
    // TODO?
    //mrc_irep_free(c, scope->irep);
    mrc_pool_close(scope->mpool);
    c->jmp = prev_jmp;
    return NULL;
  }
  MRC_END_EXC(c->jmp);
}

MRC_API mrc_irep *
mrc_generate_code(mrc_ccontext *c, mrc_node *node)
{
  return generate_code(c, node, VAL);
}

#define CALL_MAXARGS 15
#define GEN_LIT_ARY_MAX 64
#define GEN_VAL_STACK_MAX 99

/*--------------------------------------------------------------------------
 * Parser dependent code
 *------------------------------------------------------------------------*/

#include "codegen_prism.inc"
