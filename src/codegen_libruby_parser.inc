
int indent = 0;

extern pm_constant_pool_t pool;

static void
print_literal(FILE *fp, VALUE lit)
{
    object_type_t type = ((Object *)lit)->type;

    switch (type) {
      case T_NUMBER:
        fprintf(fp, "(number) %ld", ((Number *)lit)->l);
        return;
      case T__STRING:
        fprintf(fp, "(string)");
        return;
      case T__HASH:
        fprintf(fp, "(hash)");
        return;
      case T_IO:
        fprintf(fp, "(io)");
        return;
      case T_ENCODING:
        fprintf(fp, "(encoding)");
        return;
      default:
        fprintf(fp, "(Unknown) %i", type);
        return;
    }
}

static void
print_indent(FILE *fp, int indent)
{
    for (int i = 0; i < indent; i++) {
        fprintf(fp, "    ");
    }
}

#define PRINT(io, str) \
    do { \
        print_indent(io, indent); \
        fprintf(io, str); \
    } while (0)

#define FPRINTF(io, fmt, ...) \
    do { \
        print_indent(io, indent); \
        fprintf(io, fmt, __VA_ARGS__); \
    } while (0)

#define PRINT_ID(io, id) \
    do { \
        if (id < POOL_ID_OFFSET) { \
            FPRINTF(io, "id: %c\n", (char)id); \
        } \
        else { \
            pm_constant_t *constant = pm_constant_pool_id_to_constant(&pool, id - POOL_ID_OFFSET); \
            FPRINTF(io, "id: %.*s\n", (int)constant->length, constant->start); \
        } \
    } while (0)

#define PRINT_STR(io, str) \
    do { \
        print_indent(io, indent); \
        fprintf(io, "str: %s\n", str->ptr); \
    } while (0)

#define PRINT_LIT(io, lit) \
    do { \
        print_indent(io, indent); \
        fprintf(io, "lit: "); \
        print_literal(io, lit); \
        fprintf(io, "\n"); \
    } while (0)

static void
print_array(FILE *fp, mrc_codegen_scope *s, int indent, const NODE *node)
{
    codegen(s, RNODE_LIST(node)->nd_head, indent+1);
    while (RNODE_LIST(node)->nd_next && nd_type(RNODE_LIST(node)->nd_next) == NODE_LIST) {
        node = RNODE_LIST(node)->nd_next;
        codegen(s, RNODE_LIST(node)->nd_head, indent+1);
    }
}

static int
scope_body(mrc_codegen_scope *s, mrc_node *tree, int val)
{
  mrc_constant_id_list *nd_tbl;
  mrc_node *node;
  switch (nd_type(tree)) {
    case NODE_SCOPE:
    {
      rb_node_scope_t *program = RNODE_SCOPE(tree);
      nd_tbl = program->nd_tbl;
      node = (mrc_node *)program->nd_body;
      break;
    }
    default:
    {
      printf("Not implemented %s\n", "pm_node_type_to_str(PM_NODE_TYPE(tree))");
      break;
    }
  }
  mrc_codegen_scope *scope = scope_new(s->c, s, nd_tbl);

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
gen_call(mrc_codegen_scope *s, mrc_node *tree, int val, int safe)
{
  rb_node_list_t *cast = RNODE_LIST(tree);
//  pm_constant_t *constant = pm_constant_pool_id_to_constant(&s->c->p->constant_pool, (const pm_constant_id_t)cast->nd_mid);
  mrc_sym sym = 0;//mrc_find_presym(constant->start, constant->length);
  mrc_sym pm_sym = nsym(s, "puts", 4);
//  if (sym == 0) pm_sym = nsym(s->c->p, constant->start, constant->length);
  int skip = 0, n = 0, nk = 0, noop = no_optimize(s), noself = 0, blk = 0, sp_save = cursp();

 // if (cast->receiver == NULL) {
  if (1) {
    noself = noop = 1;
    push();
  }
//  else {
//    codegen(s, cast->receiver, VAL); /* receiver */
//  }
  if (safe) {
    int recv = cursp()-1;
    gen_move(s, cursp(), recv, 1);
    skip = genjmp2_0(s, OP_JMPNIL, cursp(), val);
  }

  codegen(s, cast->nd_head, val);

//  if (arguments) {
//    if (0 < arguments->arguments.size) {            /* positional arguments */
//      n = gen_values(s, (mrc_node *)arguments, VAL, 14);
//      if (n < 0) {              /* variable length */
//        noop = 1;               /* not operator */
//        n = 15;
//        push();
//      }
//    }
//    for (int i = 0; i < arguments->arguments.size; i++) {
//      mrc_node *t = (mrc_node *)arguments->arguments.nodes[i];
//      if (PM_NODE_TYPE(t) == PM_KEYWORD_HASH_NODE) {       /* keyword arguments */
//        noop = 1;
//        nk = gen_hash(s, t, VAL, 14);
//        if (nk < 0) nk = 15;
//      }
//    }
//  }
//  if (cast->block) {
//    codegen(s, cast->block, VAL);
//    pop();
//    noop = 1;
//    blk = 1;
//  }
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

static void
codegen(mrc_codegen_scope *s, mrc_node *node, int val)
{
  int rlev = s->rlev;

  if (!node) {
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
//
  enum node_type type = nd_type(node);
  switch (type) {
    case NODE_BEGIN:
      codegen(s, RNODE_BEGIN(node)->nd_body, val); indent+=1;
      return;
    case NODE_SCOPE:
      scope_body(s, node, val);
      return;
    case NODE_OPCALL:
      PRINT(stderr, "OPCALL:\n");
      PRINT_ID(stderr, RNODE_OPCALL(node)->nd_mid);
      codegen(s, RNODE_OPCALL(node)->nd_recv, val); indent+=1;
      codegen(s, RNODE_OPCALL(node)->nd_args, val); indent+=1;
      return;
    case NODE_LIST:
      print_array(stderr, s, indent, node);
      return;
    case NODE_FCALL:
      gen_call(s, RNODE_FCALL(node)->nd_args, val, 0); indent+=1;
      return;
    case NODE_SYM:
      PRINT(stderr, "SYM:\n");
      PRINT_STR(stderr, RNODE_SYM(node)->string);
      return;
    case NODE_STR:
      rb_node_str_t *str = RNODE_STR(node);
      char *p = str->string->ptr;
      mrc_int len = str->string->len;
      int off = new_lit_str(s, p, len);

      genop_2(s, OP_STRING, cursp(), off);
      push();
      return;
    case NODE_INTEGER:
      PRINT(stderr, "INTEGER:\n");
      FPRINTF(stderr, "lit: %s\n", RNODE_INTEGER(node)->val);
      return;
    default:
      FPRINTF(stderr, "Unknown type: %i\n", type);
      return;
  }
}
