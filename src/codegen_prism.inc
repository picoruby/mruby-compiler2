#define nint(node) PM_NODE_TYPE(node)

#define CAST3(name, from, to) \
  pm_##name##_node_t *to = (pm_##name##_node_t *)from
#define CAST(name) CAST3(name,tree,cast)

static void gen_massignment(mrc_codegen_scope *s, mrc_node *tree, int rhs, int val);

static mrc_sym
nsym(mrc_parser_state *p, const uint8_t *start, size_t length)
{
  mrc_sym sym = pm_constant_pool_insert_constant(&p->constant_pool, start, length);
  return sym;
}

static mrc_bool
true_always(mrc_node *tree)
{
  switch (nint(tree)) {
  case PM_TRUE_NODE:
  case PM_INTEGER_NODE:
  case PM_STRING_NODE:
  case PM_SYMBOL_NODE:
    return TRUE;
  default:
    return FALSE;
  }
}

static mrc_bool
false_always(mrc_node *tree)
{
  switch (nint(tree)) {
  case PM_FALSE_NODE:
  case PM_NIL_NODE:
    return TRUE;
  default:
    return FALSE;
  }
}

static void
gen_retval(mrc_codegen_scope *s, mrc_node *tree)
{
  CAST(arguments);
  if (cast->arguments.size == 1 ) {
    if (nint(cast->arguments.nodes[0]) == PM_SPLAT_NODE) {
      codegen(s, ((pm_splat_node_t *)cast->arguments.nodes[0])->expression, VAL);
      pop();
      genop_1(s, OP_ARYSPLAT, cursp());
    }
    else {
      codegen(s, cast->arguments.nodes[0], VAL);
      pop();
    }
  }
  else {
    codegen(s, tree, VAL);
    pop();
  }
}

static void
gen_assignment_lvar(mrc_codegen_scope *s, int sp, mrc_sym name, int depth, int val)
{
  if (depth == 0) {
    int idx = lv_idx(s, name);
    if (idx != sp) {
      gen_move(s, idx, sp, val);
    }
  }
  else {
    gen_setupvar(s, sp, name, depth);
  }
}

static int
gen_values(mrc_codegen_scope *s, mrc_node *tree, int val, int limit)
{
  CAST(arguments);
  mrc_node *t;

  int n = 0;
  int first = 1;
  int slimit = GEN_VAL_STACK_MAX;

  if (limit == 0) limit = GEN_LIT_ARY_MAX;
  if (cursp() >= slimit) slimit = INT16_MAX;

  if (!val) {
    for (size_t i = 0; i < cast->arguments.size; i++) {
      t = (mrc_node *)cast->arguments.nodes[i];
      codegen(s, t, NOVAL);
      n++;
    }
    return n;
  }

  for (size_t i = 0; i < cast->arguments.size; i++) {
    t = (mrc_node *)cast->arguments.nodes[i];
    if (nint(t) == PM_KEYWORD_HASH_NODE) break;
    int is_splat = nint(t) == PM_SPLAT_NODE;

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
      CAST3(array, ((pm_splat_node_t *)t)->expression, a);
      codegen(s, (mrc_node *)a, val);
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

static void
gen_assignment(mrc_codegen_scope *s, mrc_node *tree, mrc_node *rhs, int sp, int val)
{
  int idx;

  switch (nint(tree)) {
    case PM_LOCAL_VARIABLE_WRITE_NODE:
    case PM_LOCAL_VARIABLE_TARGET_NODE:
    case PM_INSTANCE_VARIABLE_WRITE_NODE:
    case PM_INSTANCE_VARIABLE_TARGET_NODE:
    case PM_CONSTANT_WRITE_NODE:
    case PM_CONSTANT_TARGET_NODE:
    case PM_GLOBAL_VARIABLE_WRITE_NODE:
    case PM_GLOBAL_VARIABLE_TARGET_NODE:
    case PM_CLASS_VARIABLE_WRITE_NODE:
    case PM_CLASS_VARIABLE_TARGET_NODE:
    case PM_MULTI_TARGET_NODE:
    case PM_REQUIRED_PARAMETER_NODE:
    case PM_INDEX_TARGET_NODE:
    case PM_CALL_TARGET_NODE:
    {
      if (rhs) {
        codegen(s, rhs, VAL);
        pop();
        sp = cursp();
      }
      break;
    }
    case PM_CONSTANT_PATH_WRITE_NODE:
      break;
    default:
    {
      codegen_error(s, "Not implemented (#1)");
      break;
    }
  }

  switch (nint(tree)) {
    case PM_LOCAL_VARIABLE_WRITE_NODE:
    case PM_LOCAL_VARIABLE_TARGET_NODE:
    case PM_REQUIRED_PARAMETER_NODE:
    {
      CAST(local_variable_write);
      gen_assignment_lvar(s, sp, cast->name, cast->depth, val);
      break;
    }
    case PM_INSTANCE_VARIABLE_WRITE_NODE:
    case PM_INSTANCE_VARIABLE_TARGET_NODE:
    {
      CAST(instance_variable_write);
      gen_setxv(s, OP_SETIV, sp, cast->name, val);
      break;
    }
    case PM_CONSTANT_WRITE_NODE:
    case PM_CONSTANT_TARGET_NODE:
    {
      CAST(constant_write);
      gen_setxv(s, OP_SETCONST, sp, cast->name, val);
      break;
    }
    case PM_CONSTANT_PATH_WRITE_NODE:
    case PM_CONSTANT_PATH_TARGET_NODE:
    {
      CAST(constant_path_write);
      if (sp) {
        gen_move(s, cursp(), sp, 0);
      }
      sp = cursp();
      push();
      if (cast->target->parent) {
        codegen(s, cast->target->parent, VAL);
        idx = new_sym(s, cast->target->name);
      }
      else {   /* NODE_COLON3 */
        genop_1(s, OP_OCLASS, cursp());
        push();
        idx = new_sym(s, cast->target->name);
      }
      if (rhs) {
        codegen(s, rhs, VAL); pop();
        gen_move(s, sp, cursp(), 0);
      }
      pop_n(2);
      genop_2(s, OP_SETMCNST, sp, idx);
      break;
    }
    case PM_GLOBAL_VARIABLE_WRITE_NODE:
    case PM_GLOBAL_VARIABLE_TARGET_NODE:
    {
      CAST(global_variable_read);
      gen_setxv(s, OP_SETGV, sp, cast->name, val);
      break;
    }
    case PM_CLASS_VARIABLE_WRITE_NODE:
    case PM_CLASS_VARIABLE_TARGET_NODE:
    {
      CAST(class_variable_read);
      gen_setxv(s, OP_SETCV, sp, cast->name, val);
      break;
    }
    case PM_MULTI_TARGET_NODE:
    {
      gen_massignment(s, tree, sp, val);
      break;
    }
    case PM_INDEX_TARGET_NODE:
    {
      CAST(index_target);
      codegen(s, cast->receiver, VAL);
      int n = gen_values(s, (mrc_node *)cast->arguments, VAL, 14);
      genop_2(s, OP_MOVE, cursp(), cursp() - n * 2 + 1);
      if (n == 1) {
        pop_n(2);
        genop_1(s, OP_SETIDX, cursp());
      }
      else {
        pop_n(n+1);
        genop_3(s, OP_SEND, cursp(), new_sym(s, MRC_OPSYM_2(aset)), n+1);
      }
      break;
    }
    case PM_CALL_TARGET_NODE:
    {
      CAST(call_target);
      codegen(s, cast->receiver, VAL);
      genop_2(s, OP_MOVE, cursp(), sp);
      pop();
      genop_3(s, OP_SEND, cursp(), new_sym(s, cast->name), 1);
      break;
    }
    default:
    {
      codegen_error(s, "Not implemented (#2)");
      break;
    }
  }
  if (val) push();
}

static int
scope_body(mrc_codegen_scope *s, mrc_node *tree, int val)
{
  mrc_constant_id_list *nlv;
  mrc_node *statements;
  switch (nint(tree)) {
    case PM_PROGRAM_NODE:
    {
      CAST3(program, tree, program);
      nlv = &program->locals;
      statements = (mrc_node *)program->statements;
      break;
    }
    case PM_CLASS_NODE:
    {
      CAST3(class, tree, class);
      nlv = &class->locals;
      statements = class->body;
      break;
    }
    case PM_SINGLETON_CLASS_NODE:
    {
      CAST3(singleton_class, tree, sclass);
      nlv = &sclass->locals;
      statements = sclass->body;
      break;
    }
    case PM_MODULE_NODE:
    {
      CAST3(module, tree, module);
      nlv = &module->locals;
      statements = module->body;
      break;
    }
    default:
    {
      codegen_error(s, "Not implemented (#3)");
      statements = NULL;
      nlv = NULL;
      break;
    }
  }
  mrc_codegen_scope *scope = scope_new(s->c, s, nlv);

  codegen(scope, statements, VAL);

  // For PICOIRB
  s->c->scope_sp = scope->sp - 1;

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
gen_hash(mrc_codegen_scope *s, mrc_node *tree, int val, int limit)
{
  struct pm_node_list elements;
  if (nint(tree) == PM_HASH_NODE) {
    CAST(hash);
    elements = cast->elements;
  }
  else {
    CAST(keyword_hash);
    elements = cast->elements;
  }

  int slimit = GEN_VAL_STACK_MAX;
  if (cursp() >= GEN_LIT_ARY_MAX) slimit = INT16_MAX;
  int len = 0;
  mrc_bool update = FALSE;
  mrc_bool first = TRUE;

  //while (tree) {
  for (size_t i = 0; i < elements.size; i++) {
    if (nint(elements.nodes[i]) == PM_ASSOC_SPLAT_NODE) {
      CAST3(assoc_splat, elements.nodes[i], assocsplat);
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
      CAST3(assoc, elements.nodes[i], assoc);
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

static void
gen_call(mrc_codegen_scope *s, mrc_node *tree, int val, int safe)
{
  CAST(call);
  const mrc_sym sym = cast->name;
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
  CAST3(arguments, cast->arguments, arguments);
  if (arguments) {
    if (0 < arguments->arguments.size) {            /* positional arguments */
      n = gen_values(s, (mrc_node *)arguments, VAL, 14);
      if (n < 0) {              /* variable length */
        noop = 1;               /* not operator */
        n = 15;
        push();
      }
    }
    for (size_t i = 0; i < arguments->arguments.size; i++) {
      mrc_node *t = (mrc_node *)arguments->arguments.nodes[i];
      if (nint(t) == PM_KEYWORD_HASH_NODE) {       /* keyword arguments */
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
  if (cast->arguments && cast->arguments->base.flags &PM_ARGUMENTS_NODE_FLAGS_CONTAINS_FORWARDING) {
    blk = 1;
    n = 0xFF;
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
  else if (noself) {
    genop_3(s, blk ? OP_SSENDB : OP_SSEND, cursp(), new_sym(s, sym), n|(nk<<4));
    }
  else {
    genop_3(s, blk ? OP_SENDB : OP_SEND, cursp(), new_sym(s, sym), n|(nk<<4));
  }
  if (safe) {
    dispatch(s, skip);
  }
  if (val) {
    push();
  }
}

static void
gen_massignment(mrc_codegen_scope *s, mrc_node *tree, int rhs, int val)
{
  CAST(multi_write);
  int n = cast->lefts.size, post = cast->rights.size;

  if (0 < n) { /* pre */
    for (int i = 0; i < n; i++) {
      int sp = cursp();
      genop_3(s, OP_AREF, sp, rhs, i);
      push();
      gen_assignment(s, cast->lefts.nodes[i], NULL, sp, NOVAL);
      pop();
    }
  }
  if (cast->rest || 0 < post) {
    gen_move(s, cursp(), rhs, val);
    push_n(post+1);
    pop_n(post+1);
    genop_3(s, OP_APOST, cursp(), n, post);
    if (cast->rest) { /* rest */
      gen_assignment(s, ((pm_splat_node_t *)cast->rest)->expression, NULL, cursp(), NOVAL);
    }
    for (int i = 0; i < post; i++) {
      gen_assignment(s, cast->rights.nodes[i], NULL, cursp()+i+1, NOVAL);
    }
    if (val) {
      gen_move(s, cursp(), rhs, 0);
    }
  }
}

static void
for_body(mrc_codegen_scope *s, mrc_node *tree)
{
  mrc_codegen_scope *prev = s;
  int idx;
  struct loopinfo *lp;
  mrc_node *n2;

  CAST(for);

  /* generate receiver */
  codegen(s, (mrc_node *)cast->collection, VAL);
  /* generate loop-block */
  s = scope_new(s->c, s, s->lv);
  //                     ^^^^^ Different from original mruby
  //                           drawing the upper scope's lvars

  push();                       /* push for a block parameter */

  /* generate loop variable */
  n2 = cast->index;
  genop_W(s, OP_ENTER, 0x40000);
  if (nint(n2) == PM_MULTI_TARGET_NODE) {
    gen_massignment(s, n2, 1, VAL);
  }
  else {
    gen_assignment(s, n2, NULL, 1, NOVAL);
  }
  /* construct loop */
  lp = loop_push(s, LOOP_FOR);
  lp->pc1 = new_label(s);

  /* loop body */
  codegen(s, (mrc_node *)cast->statements, VAL);
  pop();
  gen_return(s, OP_RETURN, cursp());
  loop_pop(s, NOVAL);
  scope_finish(s);
  s = prev;
  genop_2(s, OP_BLOCK, cursp(), s->irep->rlen-1);
  push();pop(); /* space for a block */
  pop();
  idx = new_sym(s, MRC_SYM_1(each));
  genop_3(s, OP_SENDB, cursp(), idx, 0);
}

static void
mrc_constant_id_list_init_capacity(mrc_codegen_scope *s, pm_constant_id_list_t *list, size_t capacity)
{
  list->ids = (pm_constant_id_t *)codegen_palloc(s, capacity * sizeof(pm_constant_id_t));
  if (list->ids == NULL) codegen_error(s, "memory allocation error");
  list->size = 0;
  list->capacity = capacity;
}

static mrc_bool
mrc_constant_id_list_append(mrc_codegen_scope *s, pm_constant_id_list_t *list, pm_constant_id_t id)
{
  if (list->size >= list->capacity) {
    size_t oldlen = sizeof(pm_constant_id_t) * list->capacity;
    list->capacity = list->capacity == 0 ? 8 : list->capacity * 2;
    size_t newlen = sizeof(pm_constant_id_t) * list->capacity;
    list->ids = (pm_constant_id_t *)codegen_realloc(s, list->ids, oldlen, newlen);
    if (list->ids == NULL) return FALSE;
  }
  list->ids[list->size++] = id;
  return TRUE;
}

static int
lambda_body(mrc_codegen_scope *s, mrc_node *tree, mrc_node *body, pm_constant_id_list_t *locals, int blk)
{
  mrc_codegen_scope *parent = s;
  pm_parameters_node_t *parameters = NULL;
  int na = 0;
  if (tree) {
    switch nint(tree) {
      case PM_DEF_NODE:
        parameters = ((pm_def_node_t *)tree)->parameters;
        break;
      case PM_BLOCK_PARAMETERS_NODE:
        parameters = ((pm_block_parameters_node_t *)tree)->parameters;
        break;
      case PM_NUMBERED_PARAMETERS_NODE:
        parameters = NULL;
        na = ((pm_numbered_parameters_node_t *)tree)->maximum;
        break;
      default:
        codegen_error(s, "should not happen");
    }
  }
  else {
    parameters = NULL;
  }


  size_t i, ma, mma, oa, ra, pa, ppa, ka, kd, ba, forwarding;;
  forwarding = 0;
  int block_reg = 0;
  pm_constant_id_list_t *lv = (pm_constant_id_list_t *)codegen_palloc(s, sizeof(pm_constant_id_list_t));
  pm_constant_id_t null_mark = pm_constant_pool_insert_constant(&s->c->p->constant_pool, NULL, 0);

  // Create lv regs from Prism's locals
  if (parameters == NULL) {
    // TODO: FIXME this doesn't work with numbered parameters
    // TODO: `it`
    mrc_constant_id_list_init_capacity(s, lv, 2);
    mrc_constant_id_list_append(s, lv, null_mark);
    for (i = 0; i < locals->size; i++) {
      /* numbered parameters */
      mrc_constant_id_list_append(s, lv, locals->ids[i]);
    }
    ma = mma = oa = ra = pa = ppa = ka = kd = ba = 0;
  }
  else {
    int nregs;
    /* mandatory arguments */
    ma = parameters->requireds.size;
    mma = 0;
    for (i = 0; i < ma; i++) {
      if (nint(parameters->requireds.nodes[i]) == PM_MULTI_TARGET_NODE) {
        CAST3(multi_target, parameters->requireds.nodes[i], m);
        mma += m->lefts.size;
      }
    }
    oa = parameters->optionals.size;
    ra = parameters->rest ? 1 : 0;
    pa = parameters->posts.size;
    ppa = 0;
    for (i = 0; i < pa; i++) {
      if (nint(parameters->posts.nodes[i]) == PM_MULTI_TARGET_NODE) {
        CAST3(multi_target, parameters->posts.nodes[i], m);
        ppa += m->lefts.size;
      }
    }
    ka = parameters->keywords.size;
    kd = parameters->keyword_rest ? 1 : 0;
    ba = parameters->block ? 1 : 0;
    nregs = ma + mma + oa + ra + pa + ppa + ka + kd + ba;
    mrc_constant_id_list_init_capacity(s, lv, nregs);
    // mandatory
    for (i = 0; i < ma; i++) {
      if (nint(parameters->requireds.nodes[i]) == PM_MULTI_TARGET_NODE) {
        mrc_constant_id_list_append(s, lv, null_mark);
      } else {
        mrc_constant_id_list_append(s, lv, ((pm_required_parameter_node_t *)parameters->requireds.nodes[i])->name);
      }
    }
    // optional
    for (i = 0; i < oa; i++) {
      mrc_constant_id_list_append(s, lv, ((pm_optional_parameter_node_t *)parameters->optionals.nodes[i])->name);
    }
    // rest
    if (ra) {
      if (((pm_rest_parameter_node_t *)parameters->rest)->name) {
        mrc_constant_id_list_append(s, lv, ((pm_rest_parameter_node_t *)parameters->rest)->name);
      } else {
        pm_constant_id_t astr = MRC_OPSYM_2(mul);
        mrc_constant_id_list_append(s, lv, astr);
      }
    }
    // post
    for (i = 0; i < pa; i++) {
      if (nint(parameters->posts.nodes[i]) == PM_MULTI_TARGET_NODE) {
        mrc_constant_id_list_append(s, lv, null_mark);
      } else {
        mrc_constant_id_list_append(s, lv, ((pm_required_parameter_node_t *)parameters->posts.nodes[i])->name);
      }
    }
    // keywords and block
    if (ka || kd || ba) {
      // keyword rest
      mrc_bool write_dastr = false;
      if (ka || kd) {
        write_dastr = true;
      }
      if (kd) {
        switch (nint(parameters->keyword_rest)) {
          case PM_KEYWORD_REST_PARAMETER_NODE: {
            if (((pm_keyword_rest_parameter_node_t *)parameters->keyword_rest)->name) {
              mrc_constant_id_list_append(s, lv, ((pm_keyword_rest_parameter_node_t *)parameters->keyword_rest)->name);
              write_dastr = false;
            }
            break;
          }
          case PM_FORWARDING_PARAMETER_NODE: {
            forwarding = 1;
            write_dastr = false;
            pm_constant_id_t astr = MRC_OPSYM_2(mul);
            mrc_constant_id_list_append(s, lv, astr);
            pm_constant_id_t dastr = MRC_OPSYM_2(pow);
            mrc_constant_id_list_append(s, lv, dastr);
            mrc_constant_id_list_append(s, lv, null_mark);
            pm_constant_id_t and = MRC_OPSYM_2(and);
            mrc_constant_id_list_append(s, lv, and);
            block_reg = lv->size;
            break;
          }
          default:
            codegen_error(s, "Unknown node");
        }
      }
      if (write_dastr) {
        pm_constant_id_t dastr = MRC_OPSYM_2(pow);
        mrc_constant_id_list_append(s, lv, dastr);
      }
    }
    if (forwarding == 0) {
      mrc_constant_id_list_append(s, lv, null_mark);
    }
    // block
    if (ba) {
      if (((pm_block_parameter_node_t *)parameters->block)->name) {
        mrc_constant_id_list_append(s, lv, ((pm_block_parameter_node_t *)parameters->block)->name);
      }
      else {
        pm_constant_id_t and = MRC_OPSYM_2(and);
        mrc_constant_id_list_append(s, lv, and);
      }
      block_reg = lv->size;
    }
    // keywords
    for (i = 0; i < ka; i++) {
      if (nint(parameters->keywords.nodes[i]) == PM_REQUIRED_KEYWORD_PARAMETER_NODE) {
        mrc_constant_id_list_append(s, lv, ((pm_required_keyword_parameter_node_t *)parameters->keywords.nodes[i])->name);
      }
      else {
        mrc_constant_id_list_append(s, lv, ((pm_optional_keyword_parameter_node_t *)parameters->keywords.nodes[i])->name);
      }
    }
    for (i = 0; i < ma; i++) {
      if (nint(parameters->requireds.nodes[i]) == PM_MULTI_TARGET_NODE) {
        CAST3(multi_target, parameters->requireds.nodes[i], m);
        for (size_t j = 0; j < m->lefts.size; j++) {
          mrc_constant_id_list_append(s, lv, ((pm_required_parameter_node_t *)m->lefts.nodes[j])->name);
        }
      }
    }
    for (i = 0; i < pa; i++) {
      if (nint(parameters->posts.nodes[i]) == PM_MULTI_TARGET_NODE) {
        CAST3(multi_target, parameters->posts.nodes[i], m);
        for (size_t j = 0; j < m->lefts.size; j++) {
          mrc_constant_id_list_append(s, lv, ((pm_required_parameter_node_t *)m->lefts.nodes[j])->name);
        }
      }
    }
  }
  if (locals) {
    for (i = 0; i < locals->size; i++) {
      if (!pm_constant_id_list_includes(lv, locals->ids[i])) {
        mrc_constant_id_list_append(s, lv, locals->ids[i]);
      }
    }
    // free Prism's locals
    pm_constant_id_list_free(locals);
    locals->ids = NULL;
    locals->size = locals->capacity = 0;
  }

  s = scope_new(s->c, s, lv);

  s->mscope = !blk;

  if (blk) {
    struct loopinfo *lp = loop_push(s, LOOP_BLOCK);
    lp->pc0 = new_label(s);
  }
  if (parameters == NULL) { /* empty parameter OR numbered parameters */
    genop_W(s, OP_ENTER, MRC_ARGS_REQ(na));
    s->ainfo = (na & 0x3f) << 7;
  }
  else {
    mrc_aspec a;
    uint32_t pos;
    mrc_node **margs, **pargs;

    /* mandatory arguments */
    margs = parameters->requireds.nodes;
    /* mandatory arguments after rest argument */
    pargs = parameters->posts.nodes;

    if (ma > 0x1f || oa > 0x1f || pa > 0x1f || ka > 0x1f) {
      codegen_error(s, "too many formal arguments");
    }
    /* (23bits = 5:5:1:5:5:1:1) */
    ra = ra|forwarding;
    ba = ba|forwarding;
    a = MRC_ARGS_REQ(ma)
      | MRC_ARGS_OPT(oa)
      | (ra? MRC_ARGS_REST() : 0)
      | MRC_ARGS_POST(pa)
      | MRC_ARGS_KEY(ka, kd)
      | (ba? MRC_ARGS_BLOCK() : 0);
    genop_W(s, OP_ENTER, a);
    /* (12bits = 5:1:5:1) */
    s->ainfo = (((ma+oa) & 0x3f) << 7)
      | ((ra & 0x1) << 6)
      | ((pa & 0x1f) << 1)
      | ((ka | kd) ? 1 : 0);
    /* generate jump table for optional arguments initializer */
    pos = new_label(s);
    for (i=0; i<oa; i++) {
      new_label(s);
      genjmp_0(s, OP_JMP);
    }
    if (oa > 0) {
      genjmp_0(s, OP_JMP);
    }
    for (i = 0; i < oa; i++) {
      CAST3(optional_parameter, parameters->optionals.nodes[i], opt);
      int idx;
      mrc_sym id = opt->name;

      dispatch(s, pos+i*3+1);
      codegen(s, opt->value, VAL);
      pop();
      idx = lv_idx(s, id);
      if (idx > 0) {
        gen_move(s, idx, cursp(), 0);
      }
      else {
        // TODO
        //gen_getupvar(s, cursp(), id, );
        codegen_error(s, "getupvar happens in optional argument");
      }
    }
    if (oa > 0) {
      dispatch(s, pos+i*3+1);
    }

    /* keyword arguments */
    if (ka) {
      for (i = 0; i < ka; i++) {
        int jmpif_key_p, jmp_def_set = -1;
        CAST3(required_keyword_parameter, parameters->keywords.nodes[i], kwd);
        mrc_sym kwd_sym = kwd->name;

        if (nint((mrc_node *)kwd) == PM_OPTIONAL_KEYWORD_PARAMETER_NODE) {
          int idx;
          genop_2(s, OP_KEY_P, lv_idx(s, kwd_sym), new_sym(s, kwd_sym));
          jmpif_key_p = genjmp2_0(s, OP_JMPIF, lv_idx(s, kwd_sym), NOVAL);
          codegen(s, ((pm_optional_keyword_parameter_node_t *)kwd)->value, VAL);
          pop();
          idx = lv_idx(s, kwd_sym);
          if (idx > 0) {
            gen_move(s, idx, cursp(), 0);
          }
          else {
            // TODO
            // gen_getupvar(s, cursp(), kwd_sym);
            codegen_error(s, "getupvar happens in keyword argument");
          }
          jmp_def_set = genjmp_0(s, OP_JMP);
          dispatch(s, jmpif_key_p);
        }
        genop_2(s, OP_KARG, lv_idx(s, kwd_sym), new_sym(s, kwd_sym));
        if (jmp_def_set != -1) {
          dispatch(s, jmp_def_set);
        }
      }
      if (!kd) {
        genop_0(s, OP_KEYEND);
      }
    }

    /* block argument */
    if (block_reg) {
      gen_move(s, block_reg, block_reg-1, 0);
    }

    /* argument destructuring */
    if (margs) {
      pos = 1;
      for (i = 0; i < ma; i++) {
        if (nint(margs[i]) == PM_MULTI_TARGET_NODE) {
          gen_massignment(s, margs[i], pos, NOVAL);
          // Enabling the following three lines would generate VM code equivalent to mruby-compiler,
          // but it would not work in mruby/c.
          // It appears to work correctly even when commented out, so it is left commented out
          uint16_t n = ((pm_multi_target_node_t *)margs[i])->lefts.size;
          gen_move(s, cursp(), pos, 0);
          genop_3(s, OP_APOST, cursp(), n, 0);
        }
        pos++;
      }
    }
    if (pargs) {
      pos = ma+oa+ra+1;
      for (i = 0; i < pa; i++) {
        if (nint(pargs[i]) == PM_MULTI_TARGET_NODE) {
          gen_massignment(s, pargs[i], pos, NOVAL);
          // Enabling the following three lines would generate VM code equivalent to mruby-compiler,
          // but it would not work in mruby/c.
          // It appears to work correctly even when commented out, so it is left commented out
          uint16_t n = ((pm_multi_target_node_t *)pargs[i])->lefts.size;
          gen_move(s, cursp(), pos, 0);
          genop_3(s, OP_APOST, cursp(), n, 0);
        }
        pos++;
      }
    }
  }

  codegen(s, body, VAL);
  pop();
  if (s->pc > 0) {
    gen_return(s, OP_RETURN, cursp());
  }
  if (blk) {
    loop_pop(s, NOVAL);
  }
  scope_finish(s);
  return parent->irep->rlen - 1;
}

static void
gen_lvar(mrc_codegen_scope *s, mrc_sym name, int depth)
{
  if (depth == 0) {
    gen_move(s, cursp(), lv_idx(s, name), 1);
  }
  else {
    gen_getupvar(s, cursp(), name, depth);
  }
  push();
}

static void
gen_binary_operator(mrc_codegen_scope *s, mrc_sym binary_operator)
{
  if (binary_operator == MRC_OPSYM_2(add)) {
    gen_addsub(s, OP_ADD, cursp());
  }
  else if (binary_operator == MRC_OPSYM_2(sub)) {
    gen_addsub(s, OP_SUB, cursp());
  }
  else if (binary_operator == MRC_OPSYM_2(mul)) {
    genop_1(s, OP_MUL, cursp());
  }
  else if (binary_operator == MRC_OPSYM_2(div)) {
    genop_1(s, OP_DIV, cursp());
  }
  else {
    int idx = new_sym(s, binary_operator);
    genop_3(s, OP_SEND, cursp(), idx, 1);
  }
}

static void
regex_set_flags(pm_node_flags_t flags, char *p2, char *p3)
{
  int p2_len = 0;
  if (flags&PM_REGULAR_EXPRESSION_FLAGS_IGNORE_CASE) p2[p2_len++] = 'i';
  if (flags&PM_REGULAR_EXPRESSION_FLAGS_EXTENDED)    p2[p2_len++] = 'x';
  if (flags&PM_REGULAR_EXPRESSION_FLAGS_MULTI_LINE)  p2[p2_len++] = 'm';
  // mruby does not support once-only subexpression
  // if (flags|PM_REGULAR_EXPRESSION_FLAGS_ONCE)     p2[p2_len++] = 'o';
  if (flags&PM_REGULAR_EXPRESSION_FLAGS_EUC_JP)           p3[0] = 'e';
  else if (flags&PM_REGULAR_EXPRESSION_FLAGS_ASCII_8BIT)  p3[0] = 'n';
  else if (flags&PM_REGULAR_EXPRESSION_FLAGS_WINDOWS_31J) p3[0] = 's';
  else if (flags&PM_REGULAR_EXPRESSION_FLAGS_UTF_8)       p3[0] = 'u';
  // TODO???
  // /** internal bytes forced the encoding to UTF-8 */
  // PM_REGULAR_EXPRESSION_FLAGS_FORCED_UTF8_ENCODING
  // /** internal bytes forced the encoding to binary */
  // PM_REGULAR_EXPRESSION_FLAGS_FORCED_BINARY_ENCODING
  // /** internal bytes forced the encoding to US-ASCII */
  // PM_REGULAR_EXPRESSION_FLAGS_FORCED_US_ASCII_ENCODING
}

static void
gen_begin(mrc_codegen_scope *s, mrc_node *tree, int val)
{
  CAST(begin);
  if (val && !cast->statements) {
    genop_1(s, OP_LOADNIL, cursp());
    push();
  }
  if (cast->statements) {
    size_t last_index = cast->statements->body.size;
    for (uint32_t i = 0; i < last_index; i++) {
      codegen(s, (mrc_node *)cast->statements->body.nodes[i], (i+1 < last_index) ? NOVAL : val);
    }
  }
}

static void
gen_rescue(mrc_codegen_scope *s, mrc_node *tree, uint32_t *pos1, int *exc, uint32_t *extend, int val)
{
  CAST3(rescue, tree, rescue);
  if (nint((mrc_node *)rescue) != PM_RESCUE_NODE) {
    codegen_error(s, "should not happen");
  }
  size_t i;
  uint32_t pos2, tmp;

  dispatch(s, *pos1);
  pos2 = JMPLINK_START;

  /* handle classes */
  if (rescue->exceptions.size == 0) {
    genop_2(s, OP_GETCONST, cursp(), new_sym(s, MRC_SYM_1(StandardError)));
    push();
    pop();
    genop_2(s, OP_RESCUE, *exc, cursp());
    tmp = genjmp2(s, OP_JMPIF, cursp(), pos2, val);
    pos2 = tmp;
  }
  else {
    for (i = 0; i < rescue->exceptions.size; i++) {
      if (nint((mrc_node *)rescue->exceptions.nodes[i]) == PM_SPLAT_NODE) {
        codegen(s, (mrc_node *)rescue->exceptions.nodes[i], VAL);
        gen_move(s, cursp(), *exc, 0);
        push_n(2); pop_n(2); /* space for one arg and a block */
        pop();
        genop_3(s, OP_SEND, cursp(), new_sym(s, MRC_SYM_1(__case_eqq)), 1);
      }
      else {
        codegen(s, (mrc_node *)rescue->exceptions.nodes[i], VAL);
        pop();
        genop_2(s, OP_RESCUE, *exc, cursp());
      }
      tmp = genjmp2(s, OP_JMPIF, cursp(), pos2, val);
      pos2 = tmp;
    }
  }
  *pos1 = genjmp_0(s, OP_JMP);
  dispatch_linked(s, pos2);

  pop();
  /* exc_var: `=> e` */
  if (rescue->reference) {
    gen_assignment(s, rescue->reference, NULL, *exc, NOVAL);
  }
  /* handle body */
  codegen(s, (mrc_node *)rescue->statements, val);
  if (val) pop();
  tmp = genjmp(s, OP_JMP, *extend);
  *extend = tmp;
  push();
  /* rest of rescue(s) */
  if (rescue->subsequent) {
    gen_rescue(s, (mrc_node *)rescue->subsequent, pos1, exc, extend, val);
  }
}

static void
gen_ensure(mrc_codegen_scope *s, mrc_node *tree, uint32_t catch_entry, uint32_t begin)
{
  CAST3(ensure, tree, ensure);
  int ensure_end, ensure_target;
  int idx;
  push();
  ensure_end = ensure_target = s->pc;
  push();
  idx = cursp();
  genop_1(s, OP_EXCEPT, idx);
  push();
  codegen(s, (mrc_node *)ensure->statements, NOVAL);
  pop();
  genop_1(s, OP_RAISEIF, idx);
  pop();
  catch_handler_set(s, catch_entry, MRC_CATCH_ENSURE, begin, ensure_end, ensure_target);
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
  uint32_t token_pos = (uint32_t)(tree->location.start - s->c->p->start);

  if (s->filename_index+1 < s->c->filename_table_length) {
    if (s->c->filename_table[s->filename_index+1].start <= token_pos) {
      s->filename = (const char *)s->c->filename_table[s->filename_index++].filename;

      mrc_debug_info_append_file(s->c, s->irep->debug_info,
                                s->filename, s->lines, s->debug_start_pos, s->pc);
      s->debug_start_pos = s->pc;
    }
  }

  int nt = nint(tree);

  //TODO lineno
  // s->lineno = tree->lineno;

  switch (nt) {
    case PM_PROGRAM_NODE: {
      scope_body(s, tree, val);
      break;
    }
    case PM_STATEMENTS_NODE:
    {
      CAST(statements);
      size_t last_index = cast->body.size;
      if (last_index == 0) {
        genop_1(s, OP_LOADNIL, cursp());
        push();
        break;
      }
      for (uint32_t i = 0; i < last_index; i++) {
        codegen(s, (mrc_node *)cast->body.nodes[i], (i+1 < last_index) ? NOVAL : val);
      }
      break;
    }
    case PM_INSTANCE_VARIABLE_READ_NODE:
    {
      CAST(instance_variable_read);
      int sym = new_sym(s, cast->name);

      genop_2(s, OP_GETIV, cursp(), sym);
      if (val) push();
      break;
    }
    case PM_LOCAL_VARIABLE_READ_NODE:
    {
      if (val) {
        CAST(local_variable_read);
        gen_lvar(s, cast->name, cast->depth);
      }
      break;
    }
    case PM_GLOBAL_VARIABLE_READ_NODE:
    {
      CAST(global_variable_read);
      int sym = new_sym(s, cast->name);
      genop_2(s, OP_GETGV, cursp(), sym);
      if (val) push();
      break;
    }
    case PM_CLASS_VARIABLE_READ_NODE:
    {
      CAST(class_variable_read);
      int sym = new_sym(s, cast->name);
      genop_2(s, OP_GETCV, cursp(), sym);
      if (val) push();
      break;
    }
    case PM_CONSTANT_READ_NODE:
    {
      CAST(constant_read);
      int sym = new_sym(s, cast->name);
      genop_2(s, OP_GETCONST, cursp(), sym);
      if (val) push();
      break;
    }
#define case_WRITE_NODE(NODE_TYPE, CAST_TYPE) \
  case NODE_TYPE##_WRITE_NODE: \
    { \
      CAST_TYPE##_write_node_t *cast = (CAST_TYPE##_write_node_t *)tree; \
      gen_assignment(s, tree, (mrc_node *)cast->value, 0, val); \
      break; \
    } \
  case NODE_TYPE##_TARGET_NODE: \
    { \
      gen_assignment(s, tree, NULL, 0, val); \
      break; \
    }
    case_WRITE_NODE(PM_INSTANCE_VARIABLE, pm_instance_variable)
    case_WRITE_NODE(PM_LOCAL_VARIABLE, pm_local_variable)
    case_WRITE_NODE(PM_CONSTANT, pm_constant)
    case_WRITE_NODE(PM_GLOBAL_VARIABLE, pm_global_variable)
    case_WRITE_NODE(PM_CLASS_VARIABLE, pm_global_variable)
    case_WRITE_NODE(PM_CONSTANT_PATH, pm_constant_path)
    case PM_MULTI_WRITE_NODE:
    {
      CAST(multi_write);
      size_t len = 0, n = 0, post = 0;
      CAST3(array, cast->value, t);
      int rhs = cursp();

      if (!val && nint((mrc_node *)t) == PM_ARRAY_NODE && !(t->base.flags & PM_ARRAY_NODE_FLAGS_CONTAINS_SPLAT) ) {
        /* fixed rhs */
        len = t->elements.size;
        for (size_t i = 0; i < len; i++) {
          codegen(s, t->elements.nodes[i], VAL);
        }
        if (0 < cast->lefts.size) {
          n = 0;
          for (size_t i = 0; i < cast->lefts.size; i++) {
            if (i < len) {
              gen_assignment(s, cast->lefts.nodes[i], NULL, rhs+n, NOVAL);
              n++;
            }
            else {
              genop_1(s, OP_LOADNIL, rhs+n);
              gen_assignment(s, cast->lefts.nodes[i], NULL, rhs+n, NOVAL);
            }
          }
        }
        post = cast->rights.size;
        if (cast->rest) {
          int rn;
          if (len < post + n) {
            rn = 0;
          }
          else {
            rn = len - post - n;
          }
          if (cursp() == rhs+n) {
            genop_2(s, OP_ARRAY, cursp(), rn);
          }
          else {
            genop_3(s, OP_ARRAY2, cursp(), rhs+n, rn);
          }
          gen_assignment(s, ((pm_splat_node_t *)cast->rest)->expression, NULL, cursp(), NOVAL);
          n += rn;
        }
        if (0 < post) {
          for (size_t i = 0; i < post; i++) {
            if (n < len) {
              gen_assignment(s, cast->rights.nodes[i], NULL, rhs+n, NOVAL);
            }
            else {
              genop_1(s, OP_LOADNIL, cursp());
              gen_assignment(s, cast->rights.nodes[i], NULL, cursp(), NOVAL);
              n++;
            }
          }
        }
        pop_n(len);
      }
      else {
        /* variable rhs */
        codegen(s, cast->value, VAL);
        gen_massignment(s, tree, rhs, val);
        if (!val) {
          pop();
        }
      }
      break;
    }
    case PM_CONSTANT_PATH_OPERATOR_WRITE_NODE:
    {
      codegen_error(s, "constant re-assignment");
      break;
    }
    case PM_LOCAL_VARIABLE_OPERATOR_WRITE_NODE:
    case PM_GLOBAL_VARIABLE_OPERATOR_WRITE_NODE:
    case PM_INSTANCE_VARIABLE_OPERATOR_WRITE_NODE:
    case PM_CLASS_VARIABLE_OPERATOR_WRITE_NODE:
    case PM_CONSTANT_OPERATOR_WRITE_NODE:
    {
      mrc_sym name = -1, binary_operator = -1;
      mrc_node *value = NULL;
      int op_set = -1, op_get = -1, depth = -1;
#define CAST_OP_WRITE(type) \
  CAST(type); \
  name = cast->name; \
  value = cast->value; \
  binary_operator = cast->binary_operator
      switch (nt) {
        case PM_LOCAL_VARIABLE_OPERATOR_WRITE_NODE:
          {
            CAST_OP_WRITE(local_variable_operator_write);
            depth = cast->depth;
            break;
          }
        case PM_GLOBAL_VARIABLE_OPERATOR_WRITE_NODE:
          {
            CAST_OP_WRITE(global_variable_operator_write);
            op_set = OP_SETGV; op_get = OP_GETGV;
            break;
          }
        case PM_CLASS_VARIABLE_OPERATOR_WRITE_NODE:
          {
            CAST_OP_WRITE(class_variable_operator_write);
            op_set = OP_SETCV; op_get = OP_GETCV;
            break;
          }
        case PM_INSTANCE_VARIABLE_OPERATOR_WRITE_NODE:
          {
            CAST_OP_WRITE(instance_variable_operator_write);
            op_set = OP_SETIV; op_get = OP_GETIV;
            break;
          }
        case PM_CONSTANT_OPERATOR_WRITE_NODE:
          {
            CAST_OP_WRITE(constant_operator_write);
            op_set = OP_SETCONST; op_get = OP_GETCONST;
            break;
          }
        default: codegen_error(s, "Not implemented (#5)");
      }
      switch (nt) {
        case PM_LOCAL_VARIABLE_OPERATOR_WRITE_NODE:
          gen_lvar(s, name, depth);
          break;
        case PM_GLOBAL_VARIABLE_OPERATOR_WRITE_NODE:
        case PM_INSTANCE_VARIABLE_OPERATOR_WRITE_NODE:
        case PM_CLASS_VARIABLE_OPERATOR_WRITE_NODE:
        case PM_CONSTANT_OPERATOR_WRITE_NODE:
          genop_2(s, op_get, cursp(), new_sym(s, name));
          push();
          break;
        default: codegen_error(s, "Not implemented (#6)");
      }
      codegen(s, (mrc_node *)value, VAL);
      push(); pop();
      pop(); pop();

      gen_binary_operator(s, binary_operator);
      switch (nt) {
        case PM_LOCAL_VARIABLE_OPERATOR_WRITE_NODE:
          gen_assignment_lvar(s, cursp(), name, depth, val);
          break;
        case PM_GLOBAL_VARIABLE_OPERATOR_WRITE_NODE:
        case PM_INSTANCE_VARIABLE_OPERATOR_WRITE_NODE:
        case PM_CLASS_VARIABLE_OPERATOR_WRITE_NODE:
        case PM_CONSTANT_OPERATOR_WRITE_NODE:
          gen_setxv(s, op_set, cursp(), name, val);
          break;
        default: codegen_error(s, "Not implemented (#7)");
      }
      if (val) push();
      break;
    }
    case PM_CALL_OPERATOR_WRITE_NODE:
    case PM_CALL_OR_WRITE_NODE:
    case PM_CALL_AND_WRITE_NODE:
    {
#define CAST_CALL_WRITE(type) \
  CAST(type); \
  receiver = (mrc_node *)cast->receiver; \
  value = (mrc_node *)cast->value; \
  read_name = cast->read_name; \
  write_name = cast->write_name;
      mrc_node *receiver = NULL, *value = NULL;
      mrc_sym read_name = -1, write_name = -1, binary_operator = -1, op_jmp = -1;
      uint32_t pos = -1;
      switch (nt) {
        case PM_CALL_OPERATOR_WRITE_NODE:
          {
            CAST_CALL_WRITE(call_operator_write);
            binary_operator = cast->binary_operator;
            break;
          }
        case PM_CALL_OR_WRITE_NODE:
          {
            CAST_CALL_WRITE(call_or_write);
            op_jmp = OP_JMPIF;
            break;
          }
        case PM_CALL_AND_WRITE_NODE:
          {
            CAST_CALL_WRITE(call_and_write);
            op_jmp = OP_JMPNOT;
            break;
          }
        default: codegen_error(s, "Not implemented (call_operator|or|and_write)");
      }
      int base;
      int idx, vsp = -1;
      if (val) {
        vsp = cursp();
        push();
      }
      codegen(s, receiver, VAL);
      idx = new_sym(s, read_name);
      base = cursp()-1;
      /* copy receiver and arguments */
      gen_move(s, cursp(), base, 1);
      push_n(2); pop_n(2); /* space for receiver, arguments and a block */
      genop_3(s, OP_SEND, cursp(), idx, 0);

      if (-1 != (int32_t)binary_operator) {
        push();
        codegen(s, value, VAL);
        push(); pop();
        pop(); pop();
        gen_binary_operator(s, binary_operator);
      }
      else { /* OR or AND */
        if (0 <= vsp) {
          gen_move(s, vsp, cursp(), 0);
        }
        pos = genjmp2_0(s, op_jmp, cursp(), val);
        codegen(s, value, VAL);
        pop();
      }
      if (0 <= vsp) {
        gen_move(s, vsp, cursp(), 0);
      }
      pop();
      idx = new_sym(s, write_name);
      genop_3(s, OP_SEND, cursp(), idx, 1);
      if (0 < pos) { dispatch(s, pos); }
      break;
    }
    case PM_INDEX_OPERATOR_WRITE_NODE:
    case PM_INDEX_OR_WRITE_NODE:
    case PM_INDEX_AND_WRITE_NODE:
    {
#define CAST_INDEX_WRITE(type) \
  CAST(type); \
  receiver = (mrc_node *)cast->receiver; \
  value = (mrc_node *)cast->value; \
  arguments = (mrc_node *)cast->arguments;
      mrc_node *receiver, *value, *arguments;
      mrc_sym binary_operator = -1;
      mrc_sym op_jmp = -1;
      switch (nt) {
        case PM_INDEX_OPERATOR_WRITE_NODE:
          {
            CAST_INDEX_WRITE(index_operator_write);
            binary_operator = cast->binary_operator;
            break;
          }
        case PM_INDEX_OR_WRITE_NODE:
          {
            CAST_INDEX_WRITE(index_or_write);
            op_jmp = OP_JMPIF;
            break;
          }
        case PM_INDEX_AND_WRITE_NODE:
          {
            CAST_INDEX_WRITE(index_and_write);
            op_jmp = OP_JMPNOT;
            break;
          }
        default:
          {
            codegen_error(s, "Not implemented (index_operator|or|and_write)");
            return;
          }
      }
      int base, nargs = 0;
      int idx, callargs = -1, vsp = -1;
      int32_t pos = -1;
      if (val) {
        vsp = cursp();
        push();
      }
      codegen(s, (mrc_node *)receiver, VAL);
      idx = new_sym(s, MRC_OPSYM_2(aref));
      base = cursp()-1;
      nargs = gen_values(s, (mrc_node *)arguments, VAL, 13);
      if (nargs >= 0) {
        callargs = nargs;
      }
      else { /* varargs */
        push();
        nargs = 1;
        callargs = CALL_MAXARGS;
      }
      /* copy receiver and arguments */
      gen_move(s, cursp(), base, 1);
      for (int i = 0; i < nargs; i++) {
        gen_move(s, cursp()+i+1, base+i+1, 1);
      }
      push_n(nargs + 2); pop_n(nargs + 2); /* space for receiver, arguments and a block */
      genop_3(s, OP_SEND, cursp(), idx, callargs);
      if (-1 != (int32_t)binary_operator) {
        push();
        codegen(s, value, VAL);
        push(); pop();
        pop(); pop();
        gen_binary_operator(s, binary_operator);
      }
      else { /* OR or AND */
        if (0 <= vsp) {
          gen_move(s, vsp, cursp(), 0);
        }
        pos = genjmp2_0(s, op_jmp, cursp(), val);
        codegen(s, value, VAL);
        pop();
        dispatch(s, pos);
      }
      if (val && vsp >= 0) {
        gen_move(s, vsp, cursp(), 0);
      }
      if (callargs == CALL_MAXARGS) {
        pop();
        genop_2(s, OP_ARYPUSH, cursp(), 1);
      }
      else {
        pop_n(callargs);
        callargs++;
      }
      pop();
      idx = new_sym(s, MRC_OPSYM_2(aset));
      genop_3(s, OP_SEND, cursp(), idx, callargs);
      if (0 <= pos) { dispatch(s, pos); }
      break;
    }
    case PM_LOCAL_VARIABLE_OR_WRITE_NODE:
    case PM_LOCAL_VARIABLE_AND_WRITE_NODE:
    case PM_INSTANCE_VARIABLE_OR_WRITE_NODE:
    case PM_INSTANCE_VARIABLE_AND_WRITE_NODE:
    case PM_CLASS_VARIABLE_OR_WRITE_NODE:
    case PM_CLASS_VARIABLE_AND_WRITE_NODE:
    case PM_GLOBAL_VARIABLE_OR_WRITE_NODE:
    case PM_GLOBAL_VARIABLE_AND_WRITE_NODE:
    case PM_CONSTANT_OR_WRITE_NODE:
    case PM_CONSTANT_AND_WRITE_NODE:
    {
      mrc_sym name = 0;
      mrc_node *value = NULL;
      int op_set = -1, op_get = -1, depth = -1;
      int op_jmp = OP_JMPNOT;
#define CAST_OR_WRITE(type) \
  CAST(type); \
  name = cast->name; \
  value = cast->value
      switch (nt) {
        case PM_LOCAL_VARIABLE_OR_WRITE_NODE: op_jmp = OP_JMPIF; /* fall through */
        case PM_LOCAL_VARIABLE_AND_WRITE_NODE:
          {
            CAST_OR_WRITE(local_variable_or_write);
            depth = cast->depth;
            break;
          }
        case PM_INSTANCE_VARIABLE_OR_WRITE_NODE: op_jmp = OP_JMPIF; /* fall through */
        case PM_INSTANCE_VARIABLE_AND_WRITE_NODE:
          {
            CAST_OR_WRITE(instance_variable_or_write);
            op_set = OP_SETIV; op_get = OP_GETIV;
            break;
          }
        case PM_GLOBAL_VARIABLE_OR_WRITE_NODE: op_jmp = OP_JMPIF; /* fall through */
        case PM_GLOBAL_VARIABLE_AND_WRITE_NODE:
          {
            CAST_OR_WRITE(global_variable_or_write);
            op_set = OP_SETGV; op_get = OP_GETGV;
            break;
          }
        case PM_CLASS_VARIABLE_OR_WRITE_NODE: op_jmp = OP_JMPIF; /* fall through */
        case PM_CLASS_VARIABLE_AND_WRITE_NODE:
          {
            CAST_OR_WRITE(class_variable_or_write);
            op_set = OP_SETCV; op_get = OP_GETCV;
            break;
          }
        case PM_CONSTANT_OR_WRITE_NODE: op_jmp = OP_JMPIF; /* fall through */
        case PM_CONSTANT_AND_WRITE_NODE:
          {
            CAST_OR_WRITE(constant_or_write);
            op_set = OP_SETCONST; op_get = OP_GETCONST;
            break;
          }
        default: codegen_error(s, "Not implemented (or_write_node)");
      }
      switch (nt) {
        case PM_LOCAL_VARIABLE_OR_WRITE_NODE:
        case PM_LOCAL_VARIABLE_AND_WRITE_NODE:
          gen_lvar(s, name, depth);
          break;
        case PM_GLOBAL_VARIABLE_OR_WRITE_NODE:
        case PM_GLOBAL_VARIABLE_AND_WRITE_NODE:
        case PM_INSTANCE_VARIABLE_OR_WRITE_NODE:
        case PM_INSTANCE_VARIABLE_AND_WRITE_NODE:
        case PM_CLASS_VARIABLE_AND_WRITE_NODE:
        case PM_CONSTANT_AND_WRITE_NODE:
          genop_2(s, op_get, cursp(), new_sym(s, name));
          push();
          break;
        case PM_CLASS_VARIABLE_OR_WRITE_NODE:
        case PM_CONSTANT_OR_WRITE_NODE:
          {
            int catch_entry, begin, end;
            int noexc, exc;
            struct loopinfo *lp;

            lp = loop_push(s, LOOP_BEGIN);
            lp->pc0 = new_label(s);
            catch_entry = catch_handler_new(s);
            begin = s->pc;
            exc = cursp();
            genop_2(s, op_get, cursp(), new_sym(s, name));
            push();
            end = s->pc;
            noexc = genjmp_0(s, OP_JMP);
            lp->type = LOOP_RESCUE;
            catch_handler_set(s, catch_entry, MRC_CATCH_RESCUE, begin, end, s->pc);
            genop_1(s, OP_EXCEPT, exc);
            genop_1(s, OP_LOADF, exc);
            dispatch(s, noexc);
            loop_pop(s, NOVAL);
            break;
          }
        default: codegen_error(s, "Not implemented (or_write_node)");
      }
      uint32_t pos;
      pop();
      pos = genjmp2_0(s, op_jmp, cursp(), val);
      codegen(s, value, VAL);
      pop();
      switch (nt) {
        case PM_LOCAL_VARIABLE_OR_WRITE_NODE:
        case PM_LOCAL_VARIABLE_AND_WRITE_NODE:
          gen_assignment_lvar(s, cursp(), name, depth, val);
          break;
        case PM_GLOBAL_VARIABLE_OR_WRITE_NODE:
        case PM_GLOBAL_VARIABLE_AND_WRITE_NODE:
        case PM_INSTANCE_VARIABLE_OR_WRITE_NODE:
        case PM_INSTANCE_VARIABLE_AND_WRITE_NODE:
        case PM_CLASS_VARIABLE_OR_WRITE_NODE:
        case PM_CLASS_VARIABLE_AND_WRITE_NODE:
        case PM_CONSTANT_OR_WRITE_NODE:
        case PM_CONSTANT_AND_WRITE_NODE:
          gen_setxv(s, op_set, cursp(), name, val);
          push();
          break;
        default: codegen_error(s, "Not implemented (or_write)");
      }
      dispatch(s, pos);
      break;
    }
    case PM_INTEGER_NODE:
    {
      if (val) {
        CAST(integer);
        if (cast->value.length == 0) {
          if (!cast->value.negative) {
            gen_int(s, cursp(), (mrc_int)cast->value.value);
          }
          else {
            gen_int(s, cursp(), (mrc_int)cast->value.value * -1);
          }
        }
        else {
          if (cast->value.length == 2) {
#ifdef MRC_INT64
            mrc_uint value = ((mrc_uint)cast->value.values[0])|((mrc_uint)cast->value.values[1] << 32);
            if (!cast->value.negative && MRC_INT_MAX < value) goto overflow;
            if (cast->value.negative) {
              if (value < MRC_INT_MIN) goto overflow;
              value *= -1;
            }
            gen_int(s, cursp(), value);
#else
            goto overflow;
#endif
          }
          else {
          overflow:
            {
              pm_buffer_t buf = {0};
              pm_integer_string(&buf, &cast->value);
              buf.value[buf.length] = '\0';
              if (cast->value.negative) {
                memmove(buf.value, buf.value+1, buf.length);
                buf.length--;
              }
              int off = new_litbint(s, buf.value, 10, cast->value.negative);
              genop_2(s, OP_LOADL, cursp(), off);
              pm_buffer_free(&buf);
            }
          }
        }
        push();
      }
      break;
    }
#ifndef MRC_NO_FLOAT
  case PM_FLOAT_NODE:
  {
    if (val) {
      CAST(float);
      int off = new_lit_float(s, (mrc_float)cast->value);
      genop_2(s, OP_LOADL, cursp(), off);
      push();
    }
    break;
  }
#endif
    case PM_CALL_NODE:
    {
      CAST(call);
      gen_call(s, tree, val, (cast->base.flags & PM_CALL_NODE_FLAGS_SAFE_NAVIGATION) ? 1 : 0);
      break;
    }
    case PM_ARRAY_NODE:
    case PM_ARGUMENTS_NODE: /* TODO: Confirm when multi args in return */
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
        CAST(symbol);
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
    case PM_SPLAT_NODE:
    {
      CAST(splat);
      codegen(s, (mrc_node *)cast->expression, val);
      break;
    }
    case PM_STRING_NODE:
    {
      if (val) {
        CAST(string);
        char *p = (char *)cast->unescaped.source;
        mrc_int len = cast->unescaped.length;
        int off = new_lit_str(s, p, len);

        genop_2(s, OP_STRING, cursp(), off);
        push();
      }
      break;
    }
    case PM_X_STRING_NODE:
    {
      CAST(x_string);
      char *p = (char *)cast->unescaped.source;
      mrc_int len = cast->unescaped.length;
      int off = new_lit_str(s, p, len);
      int sym = new_sym(s, MRC_OPSYM_2(tick));

      genop_1(s, OP_LOADSELF, cursp());
      push();
      genop_2(s, OP_STRING, cursp(), off);
      push(); push();
      pop_n(3);
      genop_3(s, OP_SEND, cursp(), sym, 1);
      if (val) push();
      break;
    }
    case PM_REGULAR_EXPRESSION_NODE:
    {
      if (val) {
        CAST(regular_expression);
        char *p1 = (char *)cast->unescaped.source;
        char p2[4] = {0, 0, 0, 0};
        char p3[2] = {0, 0};
        regex_set_flags(cast->base.flags, p2, p3);
        int sym = new_sym(s, MRC_SYM_1(Regexp));
        int off = new_lit_str(s, p1, cast->unescaped.length);
        int argc = 1;

        genop_1(s, OP_OCLASS, cursp());
        genop_2(s, OP_GETMCNST, cursp(), sym);
        push();
        genop_2(s, OP_STRING, cursp(), off);
        push();
        if (p2[0] || p3[0]) {
          if (p2[0]) { /* opt */
            off = new_lit_cstr(s, p2);
            genop_2(s, OP_STRING, cursp(), off);
          }
          else {
            genop_1(s, OP_LOADNIL, cursp());
          }
          push();
          argc++;
          if (p3[0]) { /* enc */
            off = new_lit_str(s, p3, 1);
            genop_2(s, OP_STRING, cursp(), off);
            push();
            argc++;
          }
        }
        push(); /* space for a block */
        pop_n(argc+2);
        sym = new_sym(s, MRC_SYM_1(compile));
        genop_3(s, OP_SEND, cursp(), sym, argc);
        push();
      }
      break;
    }
    case PM_INTERPOLATED_REGULAR_EXPRESSION_NODE:
    {
      CAST(interpolated_regular_expression);
      if (val) {
        int sym = new_sym(s, MRC_SYM_1(Regexp));
        int argc = 1;

        genop_1(s, OP_OCLASS, cursp());
        genop_2(s, OP_GETMCNST, cursp(), sym);
        push();

        mrc_bool str_begin = FALSE;
        if (nint(cast->parts.nodes[0]) != PM_STRING_NODE) {
          genop_2(s, OP_STRING, cursp(), new_lit_cstr(s, ""));
          push();
          str_begin = TRUE;
        }
        for (size_t i = 0; i < cast->parts.size; i++) {
          codegen(s, cast->parts.nodes[i], VAL);
          pop();
          if (str_begin || 0 < i) {
            pop();
            genop_1(s, OP_STRCAT, cursp());
          }
          push();
        }

        char p2[4] = {0, 0, 0, 0};
        char p3[2] = {0, 0};
        regex_set_flags(cast->base.flags, p2, p3);
        if (p2[0]) { /* opt */
          genop_2(s, OP_STRING, cursp(), new_lit_cstr(s, p2));
          push();
          argc++;
        }
        if (p3[0]) { /* enc */
          genop_2(s, OP_STRING, cursp(), new_lit_cstr(s, p3));
          push();
          argc++;
        }
        push(); /* space for a block */
        pop_n(argc+2);
        sym = new_sym(s, MRC_SYM_1(compile));
        genop_3(s, OP_SEND, cursp(), sym, argc);
        push();
      }
      else {
        for (size_t i = 0; i < cast->parts.size; i++) {
          if (nint(cast->parts.nodes[i]) != PM_STRING_NODE) {
            codegen(s, cast->parts.nodes[i], NOVAL);
          }
        }
      }
      break;
    }
    case PM_BACK_REFERENCE_READ_NODE:
    {
      if (val) {
        int sym = new_sym(s, MRC_SYM_2(back_ref));
        genop_2(s, OP_GETGV, cursp(), sym);
        push();
      }
      break;
    }
    case PM_NUMBERED_REFERENCE_READ_NODE:
    {
      if (val) {
        CAST(numbered_reference_read);
        char buf[100];
        buf[0] = '$';
        sprintf(buf+1, "%d", cast->number);
        int sym = new_sym(s, nsym(s->c->p, (const uint8_t *)buf, strlen(buf)));
        genop_2(s, OP_GETGV, cursp(), sym);
        push();
      }
      break;
    }
    case PM_EMBEDDED_STATEMENTS_NODE:
    {
      CAST(embedded_statements);
      codegen(s, (mrc_node *)cast->statements, val);
      break;
    }
    case PM_INTERPOLATED_STRING_NODE:
    case PM_INTERPOLATED_SYMBOL_NODE:
    {
      size_t i;
      mrc_node **nodes;
      uint32_t size;
      if (nt == PM_INTERPOLATED_SYMBOL_NODE) {
        CAST(interpolated_symbol);
        nodes = (mrc_node **)cast->parts.nodes;
        size = cast->parts.size;
      }
      else {
        CAST(interpolated_string);
        nodes = (mrc_node **)cast->parts.nodes;
        size = cast->parts.size;
      }
      mrc_bool str_begin = FALSE;
      if (val) {
        if (nint(nodes[0]) != PM_STRING_NODE) {
          genop_2(s, OP_STRING, cursp(), new_lit_cstr(s, ""));
          push();
          str_begin = TRUE;
        }
        for (i = 0; i < size; i++) {
          codegen(s, nodes[i], VAL);
          pop();
          if (str_begin || 0 < i) {
            pop();
            genop_1(s, OP_STRCAT, cursp());
          }
          push();
        }
      }
      else {
        /* example:
         *  def my_method
         *    "Hey, #{something} happens!"
         *    return 1
         *  end
         * # The return value of the interpolated string will not be used.
         * # (This is a case when val is FALSE)
         * # So we ignore `'Hey, '` and `' happens!'`.
         * # However, we need to evaluate `something` as it may have side effects.
         */
        for (i = 0; i < size; i++) {
          if (nint(nodes[i]) != PM_STRING_NODE) {
            codegen(s, nodes[i], NOVAL);
            pop();
          }
        }
      }
      if (nt == PM_INTERPOLATED_SYMBOL_NODE) {
        if (val) {
          pop();
          genop_1(s, OP_INTERN, cursp());
          push();
        }
      }
      break;
    }
    case PM_INTERPOLATED_X_STRING_NODE:
    {
      size_t i;
      CAST(interpolated_x_string);
      int sym = new_sym(s, MRC_SYM_1(Kernel));

      genop_1(s, OP_LOADSELF, cursp());
      push();
      codegen(s, (mrc_node *)cast->parts.nodes[0], VAL);
      for (i = 1; i < cast->parts.size; i++) {
        codegen(s, (mrc_node *)cast->parts.nodes[i], VAL);
        pop(); pop();
        genop_1(s, OP_STRCAT, cursp());
        push();
      }
      push();
      pop_n(3);
      sym = new_sym(s, MRC_OPSYM_2(tick));
      genop_3(s, OP_SEND, cursp(), sym, 1);
      if (val) push();
      break;
    }
    case PM_SINGLETON_CLASS_NODE:
    {
      CAST(singleton_class);
      int idx;
      codegen(s, cast->expression, VAL);
      pop();
      genop_1(s, OP_SCLASS, cursp());
      if (cast->body == NULL) {
        genop_1(s, OP_LOADNIL, cursp());
      }
      else {
        idx = scope_body(s, tree, val);
        genop_2(s, OP_EXEC, cursp(), idx);
      }
      if (val) {
        push();
      }
      break;
    }
    case PM_DEF_NODE:
    {
      CAST(def);
      int sym = new_sym(s, cast->name);
      int idx = lambda_body(s, (mrc_node *)cast, cast->body, &cast->locals, 0);

      if (cast->receiver == NULL) {
        genop_1(s, OP_TCLASS, cursp());
        push();
      }
      else {
        codegen(s, cast->receiver, VAL);
        pop();
        genop_1(s, OP_SCLASS, cursp());
        push();
      }
      genop_2(s, OP_METHOD, cursp(), idx);
      push(); pop();
      pop();
      genop_2(s, OP_DEF, cursp(), sym);
      if (val) push();
      break;
    }
    case PM_LAMBDA_NODE:
    {
      if (val) {
        CAST(lambda);
        mrc_node *parameters = NULL;
        if ((pm_block_parameters_node_t *)cast->parameters) {
          parameters = (mrc_node *)cast->parameters;
        }
        int idx = lambda_body(s, parameters, cast->body, &cast->locals, 1);
        genop_2(s, OP_LAMBDA, cursp(), idx);
        push();
      }
      break;
    }
    case PM_BLOCK_NODE:
    {
      if (val) {
        CAST(block);
        mrc_node *parameters = NULL;
        if ((pm_block_parameters_node_t *)cast->parameters) {
          parameters = (mrc_node *)cast->parameters;
        }
        int idx = lambda_body(s, parameters, cast->body, &cast->locals, 1);
        genop_2(s, OP_BLOCK, cursp(), idx);
        push();
      }
      break;
    }
    case PM_IF_NODE:
    case PM_UNLESS_NODE:
    {
      mrc_node *predicate, *subsequent, *statements;
      if (nt == PM_IF_NODE) {
        CAST(if);
        predicate = (mrc_node *)cast->predicate;
        subsequent = (mrc_node *)cast->subsequent;
        statements = (mrc_node *)cast->statements;
      }
      else { /* unless */
        CAST(unless);
        predicate = (mrc_node *)cast->predicate;
        subsequent = (mrc_node *)cast->statements; /* opposite */
        statements = (mrc_node *)cast->else_clause; /* opposite */
      }
      uint32_t pos1, pos2;
      mrc_bool nil_p = FALSE;

      if (!predicate) {
        codegen(s, subsequent, val);
        goto exit;
      }
      if (true_always(predicate)) {
        codegen(s, statements, val);
        goto exit;
      }
      if (false_always(predicate)) {
        codegen(s, subsequent, val);
        goto exit;
      }
      if (nint(predicate) == PM_CALL_NODE) {
        pm_call_node_t *n = (pm_call_node_t *)predicate;
        mrc_sym mid = n->name;
        mrc_sym sym_nil_p = MRC_SYM_2(nil_p);
        if (mid == sym_nil_p && n->arguments == NULL) {
          nil_p = TRUE;
          codegen(s, (mrc_node *)n->receiver, VAL);
        }
      }
      if (!nil_p) {
        codegen(s, predicate, VAL);
      }
      pop();
      if (val || statements) {
        if (nil_p) {
          pos2 = genjmp2_0(s, OP_JMPNIL, cursp(), val);
          pos1 = genjmp_0(s, OP_JMP);
          dispatch(s, pos2);
        }
        else {
          pos1 = genjmp2_0(s, OP_JMPNOT, cursp(), val);
        }
        codegen(s, statements, val);
        if (val) pop();
        if (subsequent || val) {
          pos2 = genjmp_0(s, OP_JMP);
          dispatch(s, pos1);
          codegen(s, subsequent, val);
          dispatch(s, pos2);
        }
        else {
          dispatch(s, pos1);
        }
      }
      else {                   /* empty then-part */
        if (subsequent) {
          if (nil_p) {
            pos1 = genjmp2_0(s, OP_JMPNIL, cursp(), val);
          }
          else {
            pos1 = genjmp2_0(s, OP_JMPIF, cursp(), val);
          }
          codegen(s, subsequent, val);
          dispatch(s, pos1);
        }
        else if (val && !nil_p) {
          genop_1(s, OP_LOADNIL, cursp());
          push();
        }
      }
      break;
    }
    case PM_ELSE_NODE:
    {
      CAST(else);
      codegen(s, (mrc_node *)cast->statements, val);
      break;
    }
    case PM_AND_NODE:
    {
      CAST(and);
      uint32_t pos;
      if (true_always(cast->left)) {
        codegen(s, cast->right, val);
        goto exit;
      }
      if (false_always(cast->left)) {
        codegen(s, cast->left, val);
        goto exit;
      }
      codegen(s, cast->left, VAL);
      pop();
      pos = genjmp2_0(s, OP_JMPNOT, cursp(), val);
      codegen(s, cast->right, val);
      dispatch(s, pos);
      break;
    }
    case PM_OR_NODE:
    {
      CAST(or);
      uint32_t pos;
      if (true_always(cast->left)) {
        codegen(s, cast->left, val);
        goto exit;
      }
      if (false_always(cast->left)) {
        codegen(s, cast->right, val);
        goto exit;
      }
      codegen(s, cast->left, VAL);
      pop();
      pos = genjmp2_0(s, OP_JMPIF, cursp(), val);
      codegen(s, cast->right, val);
      dispatch(s, pos);
      break;
    }
    case PM_PARENTHESES_NODE:
    {
      CAST(parentheses);
      codegen(s, cast->body, val);
      break;
    }
    case PM_WHILE_NODE:
    case PM_UNTIL_NODE:
    {
      CAST(while); /* Compatible with until? */
      if (true_always(cast->predicate)) {
        if (nt == PM_UNTIL_NODE) {
          if (val) {
            genop_1(s, OP_LOADNIL, cursp());
            push();
          }
          goto exit;
        }
      }
      else if (false_always(cast->predicate)) {
        if (nt == PM_WHILE_NODE) {
          if (val) {
            genop_1(s, OP_LOADNIL, cursp());
            push();
          }
          goto exit;
        }
      }

      uint32_t pos = JMPLINK_START;
      struct loopinfo *lp = loop_push(s, LOOP_NORMAL);

      if (!val) lp->reg = -1;
      lp->pc0 = new_label(s);
      codegen(s, cast->predicate, VAL);
      pop();
      if (nt == PM_WHILE_NODE) {
        pos = genjmp2_0(s, OP_JMPNOT, cursp(), NOVAL);
      }
      else {
        pos = genjmp2_0(s, OP_JMPIF, cursp(), NOVAL);
      }
      lp->pc1 = new_label(s);
      codegen(s, (mrc_node *)cast->statements, NOVAL);
      genjmp(s, OP_JMP, lp->pc0);
      dispatch(s, pos);
      loop_pop(s, val);
      break;
    }
    case PM_FOR_NODE:
    {
      for_body(s, tree);
      if (val) push();
      break;
    }
    case PM_CASE_NODE:
    {
      CAST(case);
      int head = 0;
      uint32_t pos1, pos2, pos3, tmp;

      pos3 = JMPLINK_START;
      if (cast->predicate) {
        head = cursp();
        codegen(s, (mrc_node *)cast->predicate, VAL);
      }
      for (size_t i = 0; i < cast->conditions.size; i++) {
        pm_when_node_t *when = (pm_when_node_t *)cast->conditions.nodes[i];
        pos1 = pos2 = JMPLINK_START;
        for (size_t j = 0; j < when->conditions.size; j++) {
          mrc_node *cond = when->conditions.nodes[j];
          mrc_bool splat = FALSE;
          if (nint(cond) == PM_SPLAT_NODE) {
            splat = TRUE;
            codegen(s, (mrc_node *)((pm_splat_node_t *)cond)->expression, VAL);
          }
          else {
            codegen(s, cond, VAL);
          }
          if (head) {
            gen_move(s, cursp(), head, 0);
            push(); push(); pop(); pop(); pop();
            if (splat) {
              genop_3(s, OP_SEND, cursp(), new_sym(s, MRC_SYM_1(__case_eqq)), 1);
            }
            else {
              genop_3(s, OP_SEND, cursp(), new_sym(s, MRC_OPSYM_2(eqq)), 1);
            }
          }
          else {
            pop();
          }
          tmp = genjmp2(s, OP_JMPIF, cursp(), pos2, !head);
          pos2 = tmp;
        }
        pos1 = genjmp_0(s, OP_JMP);
        dispatch_linked(s, pos2);
        codegen(s, (mrc_node *)when->statements, val);
        if (val) pop();
        tmp = genjmp(s, OP_JMP, pos3);
        pos3 = tmp;
        dispatch(s, pos1);
      }
      if (cast->else_clause) {
        codegen(s, (mrc_node *)cast->else_clause, val);
        if (val) pop();
        tmp = genjmp(s, OP_JMP, pos3);
        pos3 = tmp;
      }
      if (val) {
        uint32_t pos = cursp();
        genop_1(s, OP_LOADNIL, pos);
        if (pos3 != JMPLINK_START) dispatch_linked(s, pos3);
        if (head) pop();
        if (cursp() != pos) {
          gen_move(s, cursp(), pos, 0);
        }
        push();
      }
      else {
        if (pos3 != JMPLINK_START) dispatch_linked(s, pos3);
        if (head) pop();
      }
      break;
    }
    case PM_SELF_NODE:
    {
      if (val) {
        genop_1(s, OP_LOADSELF, cursp());
        push();
      }
      break;
    }
    case PM_TRUE_NODE:
    {
      if (val) {
        genop_1(s, OP_LOADT, cursp());
        push();
      }
      break;
    }
    case PM_FALSE_NODE:
    {
      if (val) {
        genop_1(s, OP_LOADF, cursp());
        push();
      }
      break;
    }
    case PM_NIL_NODE:
    {
      if (val) {
        genop_1(s, OP_LOADNIL, cursp());
        push();
      }
      break;
    }
    case PM_CONSTANT_PATH_NODE:
    {
      CAST(constant_path);
      int sym = new_sym(s, cast->name);
      if (cast->parent) {
        codegen(s, cast->parent, VAL);
        pop();
      }
      else { /* NODE_COLON3 */
        genop_1(s, OP_OCLASS, cursp());
      }
      genop_2(s, OP_GETMCNST, cursp(), sym);
      if (val) push();
      break;
    }
    case PM_CLASS_NODE:
    {
      int idx;
      CAST(class);
      mrc_node *cpath = (mrc_node *)cast->constant_path;
      switch (nint(cpath)) {
        case PM_CONSTANT_READ_NODE:
          {
            genop_1(s, OP_LOADNIL, cursp());
            push();
            break;
          }
        case PM_CONSTANT_PATH_NODE:
          {
            mrc_node *parent = ((pm_constant_path_node_t *)cpath)->parent;
            codegen(s, parent, VAL);
            break;
          }
        default:
          codegen_error(s, "Invalid constant path node");
      }

      if (cast->superclass) {
        codegen(s, cast->superclass, VAL);
      }
      else {
        genop_1(s, OP_LOADNIL, cursp());
        push();
      }
      pop(); pop();
      idx = new_sym(s, cast->name);
      genop_2(s, OP_CLASS, cursp(), idx);
      if (!cast->body) {
        genop_1(s, OP_LOADNIL, cursp());
      }
      else {
        idx = scope_body(s, tree, val);
        genop_2(s, OP_EXEC, cursp(), idx);
      }
      if (val) {
        push();
      }
      break;
    }
    case PM_MODULE_NODE:
    {
      int idx;
      CAST(module);
      switch nint(cast->constant_path) {
        case PM_CONSTANT_PATH_NODE:
          {
            CAST3(constant_path, cast->constant_path, cpath);
            // TODO: they should be wrong
            if (cpath->parent == (mrc_node *)1) {
              genop_1(s, OP_LOADNIL, cursp());
              push();
            }
            else if (cpath->parent == (mrc_node *)0) {
              genop_1(s, OP_OCLASS, cursp());
              push();
            }
            else {
              codegen(s, cpath->parent, VAL);
            }
            break;
          }
        case PM_CONSTANT_READ_NODE:
          // CAST3(constant_read, cast->constant_path, read);
          genop_1(s, OP_LOADNIL, cursp());
          push();
          break;
        default:
          codegen_error(s, "Invalid constant path node");
      }
      pop();
      idx = new_sym(s, cast->name);
      genop_2(s, OP_MODULE, cursp(), idx);
      if (!cast->body) {
        genop_1(s, OP_LOADNIL, cursp());
      }
      else {
        idx = scope_body(s, tree, val);
        genop_2(s, OP_EXEC, cursp(), idx);
      }
      if (val) {
        push();
      }
      break;
    }
    case PM_ALIAS_METHOD_NODE:
    {
      CAST(alias_method);
      CAST3(symbol, cast->new_name, new_name);
      CAST3(symbol, cast->old_name, old_name);
      int a = new_sym(s, nsym(s->c->p, new_name->unescaped.source, new_name->unescaped.length));
      int b = new_sym(s, nsym(s->c->p, old_name->unescaped.source, old_name->unescaped.length));
      genop_2(s, OP_ALIAS, a, b);
      if (val) {
        genop_1(s, OP_LOADNIL, cursp());
        push();
      }
      break;
    }
    case PM_UNDEF_NODE:
    {
      CAST(undef);
      for (size_t i = 0; i < cast->names.size; i++) {
        CAST3(symbol, cast->names.nodes[i], name);
        int symbol = new_sym(s, nsym(s->c->p, name->unescaped.source, name->unescaped.length));
        genop_1(s, OP_UNDEF, symbol);
      }
      if (val) {
        genop_1(s, OP_LOADNIL, cursp());
        push();
      }
      break;
    }
    case PM_SUPER_NODE:
    {
      CAST(super);
      mrc_codegen_scope *s2 = s;
      int lv = 0;
      int n = 0, nk = 0, st = 0;

      push();
      while (!s2->mscope) {
        lv++;
        s2 = s2->prev;
        if (!s2) break;
      }
      CAST3(arguments, cast->arguments, arguments);
      if (arguments) {
        st = n = gen_values(s, (mrc_node *)arguments, VAL, 14);
        if (n < 0) {
          st = 1; n = 15;
          push();
        }
        /* keyword arguments */
        for (size_t i = 0; i < arguments->arguments.size; i++) {
          mrc_node *t = (mrc_node *)arguments->arguments.nodes[i];
          if (nint(t) == PM_KEYWORD_HASH_NODE) {
            nk = gen_hash(s, t, VAL, 14);
            if (nk < 0) {st++; nk = 15;}
            else st += nk*2;
            n |= nk<<4;
          }
        }
        /* block argument */
        if (cast->block) {
          codegen(s, (mrc_node *)cast->block, VAL);
        }
        else if (s2) gen_blkmove(s, s2->ainfo, lv);
        else {
          genop_1(s, OP_LOADNIL, cursp());
          push();
        }
      }
      else { /* `super()` parentheses without argument */
        if (s2) gen_blkmove(s, s2->ainfo, lv);
        else {
          genop_1(s, OP_LOADNIL, cursp());
          push();
        }
      }
      st++;
      pop_n(st+1);
      genop_2(s, OP_SUPER, cursp(), n);
      if (val) push();
      break;
    }
    case PM_FORWARDING_SUPER_NODE:
    {
      CAST(forwarding_super);
      mrc_codegen_scope *s2 = s;
      int lv = 0;
      uint16_t ainfo = 0;
      int n = CALL_MAXARGS;
      int sp = cursp();

      push();        /* room for receiver */
      while (!s2->mscope) {
        lv++;
        s2 = s2->prev;
        if (!s2) break;
      }
      if (s2 && s2->ainfo > 0) {
        ainfo = s2->ainfo;
      }
      if (ainfo > 0) {
        genop_2S(s, OP_ARGARY, cursp(), (ainfo<<4)|(lv & 0xf));
        push(); push(); push();   /* ARGARY pushes 3 values at most */
        pop(); pop(); pop();
        /* keyword arguments */
        if (ainfo & 0x1) {
          n |= CALL_MAXARGS<<4;
          push();
        }
        /* block argument */
        if (cast->block) {
          push();
          codegen(s, (mrc_node *)cast->block, VAL);
        }
      }
      else {
        /* block argument */
        if (cast->block) {
          codegen(s, (mrc_node *)cast->block, VAL);
        }
        else {
          gen_blkmove(s, 0, lv);
        }
        n = 0;
      }
      s->sp = sp;
      genop_2(s, OP_SUPER, cursp(), n);
      if (val) push();
      break;
    }
    case PM_RETURN_NODE:
    {
      CAST(return);
      if (cast->arguments) {
        gen_retval(s, (mrc_node *)cast->arguments);
      }
      else {
        genop_1(s, OP_LOADNIL, cursp());
      }
      if (s->loop) {
        gen_return(s, OP_RETURN_BLK, cursp());
      }
      else {
        gen_return(s, OP_RETURN, cursp());
      }
      if (val) push();
      break;
    }
    case PM_YIELD_NODE:
    {
      CAST(yield);
      mrc_codegen_scope *s2 = s;
      int lv = 0, ainfo = -1;
      int n = 0, sendv = 0;

      while (!s2->mscope) {
        lv++;
        s2 = s2->prev;
        if (!s2) break;
      }
      if (s2) {
        ainfo = (int)s2->ainfo;
      }
      if (ainfo < 0) codegen_error(s, "invalid yield (SyntaxError)");
      push();
      if (cast->arguments) {
        n = gen_values(s, (mrc_node *)cast->arguments, VAL, 14);
        if (n < 0) {
          n = sendv = 1;
          push();
        }
      }
      push(); pop(); /* space for a block */
      pop_n(n+1);
      genop_2S(s, OP_BLKPUSH, cursp(), (ainfo<<4)|(lv & 0xf));
      if (sendv) n = CALL_MAXARGS;
      genop_3(s, OP_SEND, cursp(), new_sym(s, MRC_SYM_1(call)), n);
      if (val) push();
      break;
    }
    case PM_BREAK_NODE:
    {
      CAST(break);
      loop_break(s, (mrc_node *)cast->arguments);
      if (val) push();
      break;
    }
    case PM_NEXT_NODE:
    {
      CAST(next);
      if (!s->loop) {
        raise_error(s, "unexpected next");
      }
      else if (s->loop->type == LOOP_NORMAL) {
        codegen(s, (mrc_node *)cast->arguments, NOVAL);
        genjmp(s, OP_JMPUW, s->loop->pc0);
      }
      else {
        if ((mrc_node *)cast->arguments) {
          codegen(s, (mrc_node *)cast->arguments, VAL);
          pop();
        }
        else {
          genop_1(s, OP_LOADNIL, cursp());
        }
        gen_return(s, OP_RETURN, cursp());
      }
      if (val) push();
      break;
    }
    case PM_REDO_NODE:
    {
      if (!s->loop || s->loop->type == LOOP_BEGIN || s->loop->type == LOOP_RESCUE) {
        raise_error(s, "unexpected redo");
      }
      else {
        genjmp(s, OP_JMPUW, s->loop->pc1);
      }
      if (val) push();
      break;
    }
    case PM_RETRY_NODE:
    {
      const char *msg = "unexpected retry";
      const struct loopinfo *lp = s->loop;
      while (lp && lp->type != LOOP_RESCUE) {
        lp = lp->prev;
      }
      if (!lp) {
        raise_error(s, msg);
      }
      else {
        genjmp(s, OP_JMPUW, lp->pc0);
      }
      if (val) push();
      break;
    }
    case PM_BEGIN_NODE:
    {
      CAST(begin);
      int noexc;
      uint32_t exend, pos1;
      struct loopinfo *lp;
      int catch_entry, begin, end;

      /* for ensure */
      int ensure_catch_entry = -1, ensure_begin = 0;
      if (cast->ensure_clause && cast->ensure_clause->statements) {
        ensure_catch_entry = catch_handler_new(s);
        ensure_begin = s->pc;
      }

      lp = loop_push(s, LOOP_BEGIN);
      lp->pc0 = new_label(s);
      catch_entry = catch_handler_new(s);
      begin = s->pc;
      /* begin */
      gen_begin(s, (mrc_node *)cast, VAL);
      pop();
      lp->type = LOOP_RESCUE;
      end = s->pc;
      noexc = genjmp_0(s, OP_JMP);
      catch_handler_set(s, catch_entry, MRC_CATCH_RESCUE, begin, end, s->pc);
      exend = JMPLINK_START;
      pos1 = JMPLINK_START;
      if (cast->rescue_clause) {
        int exc = cursp();
        genop_1(s, OP_EXCEPT, exc);
        push();
        /* rescue */
        gen_rescue(s, (mrc_node *)cast->rescue_clause, &pos1, &exc, &exend, val);
        if (pos1 != JMPLINK_START) {
          dispatch(s, pos1);
          genop_1(s, OP_RAISEIF, exc);
        }
      }
      pop();
      dispatch(s, noexc);
      if (cast->else_clause) {
        codegen(s, (mrc_node *)cast->else_clause, val);
      }
      else if (val) {
        push();
      }
      dispatch_linked(s, exend);
      loop_pop(s, NOVAL);

      /* ensure */
      if (cast->ensure_clause && cast->ensure_clause->statements) {
        gen_ensure(s, (mrc_node *)cast->ensure_clause, ensure_catch_entry, ensure_begin);
      }
      else {
        /* empty ensure ignored */
      }
      break;
    }
    case PM_BLOCK_ARGUMENT_NODE:
    {
      CAST(block_argument);
      if (!cast->expression) {
        mrc_sym and = MRC_OPSYM_2(and);
        int idx = lv_idx(s, and);
        if (idx == 0) {
          int depth = search_upvar(s, and, &idx);
          gen_getupvar(s, cursp(), and, depth);
        }
        else {
          gen_move(s, cursp(), idx, val);
        }
        if (val) push();
      }
      else {
        codegen(s, cast->expression, val);
      }
      break;
    }
    case PM_POST_EXECUTION_NODE:
    {
      mrc_diagnostic_list_append(s->c, tree->location.start, "END not supported", MRC_GENERATOR_ERROR);
      break;
    }
    case PM_MATCH_REQUIRED_NODE:
    {
      CAST(match_required);
      if (nint(cast->pattern) != PM_LOCAL_VARIABLE_TARGET_NODE) {
        mrc_diagnostic_list_append(s->c, cast->pattern->location.start, "expecting a local variable", MRC_GENERATOR_ERROR);
        break;
      }
      pm_local_variable_target_node_t *lvar = (pm_local_variable_target_node_t *)cast->pattern;
      gen_assignment(s, (mrc_node *)lvar, (mrc_node *)cast->value, 0, val);
      break;
    }
    case PM_RANGE_NODE:
    {
      CAST(range);
      codegen(s, cast->left, val);
      codegen(s, cast->right, val);
      if (val) {
        mrc_code op;
        if (cast->base.flags & PM_RANGE_FLAGS_EXCLUDE_END) {
          op = OP_RANGE_EXC;
        }
        else {
          op = OP_RANGE_INC;
        }
        pop(); pop();
        genop_1(s, op, cursp());
        push();
      }
      break;
    }
    case PM_SOURCE_FILE_NODE:
    {
      if (val) {
        CAST(source_file);
        char *p = (char *)cast->filepath.source;
        mrc_int len = cast->filepath.length;
        int off = new_lit_str(s, p, len);
        genop_2(s, OP_STRING, cursp(), off);
        push();
      }
      break;
    }
    case PM_SOURCE_ENCODING_NODE:
    {
      genop_3(s, OP_SSEND, cursp(), new_sym(s, MRC_SYM_1(__ENCODING__)), 0);
      push();
      { // Workaround: increase nregs in case __ENCODING__ called alone
        // (maybe it is a useless use of a literal in void context)
        push();
        pop();
      }
      break;
    }
    case PM_FORWARDING_ARGUMENTS_NODE:
    {
      //CAST(forwarding_arguments);
      if (val) {
        int idx;
        genop_1(s, OP_LOADNIL, cursp());
        push();
        // *
        idx = lv_idx(s, MRC_OPSYM_2(mul));
        assert(idx != 0);
        gen_move(s, cursp(), idx, val);
        pop();
        genop_1(s, OP_ARYCAT, cursp());
        push();
        // **
        genop_2(s, OP_HASH, cursp(), 0);
        push();
        idx = lv_idx(s, MRC_OPSYM_2(pow));
        assert(idx != 0);
        gen_move(s, cursp(), idx, val);
        pop();
        genop_1(s, OP_HASHCAT, cursp());
        push();
        // &
        idx = lv_idx(s, MRC_OPSYM_2(and));
        assert(idx != 0);
        gen_move(s, cursp(), idx, val);
      }
      break;
    }
    default:
    {
      char buf[256];
      snprintf(buf, sizeof(buf), "Not implemented: %s", pm_node_type_to_str(nt));
      codegen_error(s, buf);
      break;
    }
  }
 exit:
  s->rlev = rlev;
}
