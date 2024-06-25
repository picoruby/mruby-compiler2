#define nint(node) PM_NODE_TYPE(node)

static int
lv_idx(mrc_codegen_scope *s, mrc_sym id)
{
  for (int n = 0; n < s->lv->size; n++) {
    if (s->lv->ids[n] == id) return n + 1;
  }
  return 0;
}

#define CAST3(name, from, to) \
  pm_##name##_node_t *to = (pm_##name##_node_t *)from
#define CAST(name) CAST3(name,tree,cast)

static void gen_massignment(mrc_codegen_scope *s, mrc_node *tree, int rhs, int val);

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
      printf("Not implemented (#1) %s\n", pm_node_type_to_str(nint(tree)));
      break;
    }
  }

  switch (nint(tree)) {
    case PM_LOCAL_VARIABLE_WRITE_NODE:
    case PM_LOCAL_VARIABLE_TARGET_NODE:
    case PM_REQUIRED_PARAMETER_NODE:
    {
      CAST(local_variable_write);
      idx = lv_idx(s, cast->name);
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
    default:
    {
      printf("Not implemented (#2) %s\n", pm_node_type_to_str(nint(tree)));
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
    case PM_MODULE_NODE:
    {
      CAST3(module, tree, module);
      nlv = &module->locals;
      statements = module->body;
      break;
    }
    default:
    {
      printf("Not implemented (#3) %s\n", pm_node_type_to_str(nint(tree)));
      break;
    }
  }
  mrc_codegen_scope *scope = scope_new(s->c, s, nlv);

  codegen(scope, statements, VAL);
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
    for (int i = 0; i < cast->arguments.size; i++) {
      t = (mrc_node *)cast->arguments.nodes[i];
      codegen(s, t, NOVAL);
      n++;
    }
    return n;
  }

  for (int i = 0; i < cast->arguments.size; i++) {
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
  for (int i = 0; i < elements.size; i++) {
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
    for (int i = 0; i < arguments->arguments.size; i++) {
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
  if (cast->rest || 0 < post || nint(tree)==PM_MULTI_TARGET_NODE) {
                              //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
                              // def ((a,b),c)
    gen_move(s, cursp(), rhs, val);
    push_n(post+1);
    pop_n(post+1);
    genop_3(s, OP_APOST, cursp(), n, post);
    if (cast->rest) { /* rest */
      gen_assignment(s, ((pm_splat_node_t *)cast->rest)->expression, NULL, cursp(), NOVAL);
    }
    for (int i = 1; i <= post; i++) {
      gen_assignment(s, cast->rights.nodes[i], NULL, cursp()+i, NOVAL);
    }
    if (val) {
      gen_move(s, cursp(), rhs, 0);
    }
  }
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

static void
loop_break(mrc_codegen_scope *s, mrc_node *tree)
{
//  if (!s->loop) {
//    codegen(s, tree, NOVAL);
//    raise_error(s, "unexpected break");
//  }
//  else {
//    struct loopinfo *loop;
//
//    loop = s->loop;
//    if (tree) {
//      if (loop->reg < 0) {
//        codegen(s, tree, NOVAL);
//      }
//      else {
//        gen_retval(s, tree);
//      }
//    }
//    while (loop) {
//      if (loop->type == LOOP_BEGIN) {
//        loop = loop->prev;
//      }
//      else if (loop->type == LOOP_RESCUE) {
//        loop = loop->prev;
//      }
//      else{
//        break;
//      }
//    }
//    if (!loop) {
//      raise_error(s, "unexpected break");
//      return;
//    }
//
//    if (loop->type == LOOP_NORMAL) {
//      int tmp;
//
//      if (loop->reg >= 0) {
//        if (tree) {
//          gen_move(s, loop->reg, cursp(), 0);
//        }
//        else {
//          genop_1(s, OP_LOADNIL, loop->reg);
//        }
//      }
//      tmp = genjmp(s, OP_JMPUW, loop->pc2);
//      loop->pc2 = tmp;
//    }
//    else {
//      if (!tree) {
//        genop_1(s, OP_LOADNIL, cursp());
//      }
//      gen_return(s, OP_BREAK, cursp());
//    }
//  }
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

static int
lambda_body(mrc_codegen_scope *s, mrc_node *tree, int blk)
{
  CAST(def); // TODO: sdef, lambda, block
  pm_parameters_node_t *parameters = cast->parameters;
  mrc_codegen_scope *parent = s;

  int ma, mma, oa, ra, pa, ppa, ka, kd, ba, i;
  pm_constant_id_list_t lv;
  pm_constant_id_t null_mark = pm_constant_pool_insert_constant(&s->c->p->constant_pool, NULL, 0);

  // Create lv regs from Prism's locals
  if (parameters == NULL) {
    pm_constant_id_list_init_capacity(&lv, 2);
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
    pm_constant_id_list_init_capacity(&lv, nregs);
    // mandatory
    for (i = 0; i < ma; i++) {
      if (nint(parameters->requireds.nodes[i]) == PM_MULTI_TARGET_NODE) {
        pm_constant_id_list_append(&lv, null_mark);
      } else {
        pm_constant_id_list_append(&lv, ((pm_required_parameter_node_t *)parameters->requireds.nodes[i])->name);
      }
    }
    // optional
    for (i = 0; i < oa; i++) {
      pm_constant_id_list_append(&lv, ((pm_optional_parameter_node_t *)parameters->optionals.nodes[i])->name);
    }
    // rest
    if (ra) {
      if (((pm_rest_parameter_node_t *)parameters->rest)->name) {
        pm_constant_id_list_append(&lv, ((pm_rest_parameter_node_t *)parameters->rest)->name);
      } else {
        pm_constant_id_t astr = pm_constant_pool_insert_constant(&s->c->p->constant_pool, (const uint8_t *)"*", 1);
        pm_constant_id_list_append(&lv, astr);
      }
    }
    // post
    for (i = 0; i < pa; i++) {
      if (nint(parameters->posts.nodes[i]) == PM_MULTI_TARGET_NODE) {
        pm_constant_id_list_append(&lv, null_mark);
      } else {
        pm_constant_id_list_append(&lv, ((pm_required_parameter_node_t *)parameters->posts.nodes[i])->name);
      }
    }
    // keywords and block
    if (ka || kd) {
      // keyword rest
      if (!kd || (kd && !((pm_keyword_rest_parameter_node_t *)parameters->keyword_rest)->name) ) {
        pm_constant_id_t dastr = pm_constant_pool_insert_constant(&s->c->p->constant_pool, (const uint8_t *)"**", 2);
        pm_constant_id_list_append(&lv, dastr);
      }
      else {
        pm_constant_id_list_append(&lv, ((pm_keyword_rest_parameter_node_t *)parameters->keyword_rest)->name);
      }
      // block
      if (ba) {
        pm_constant_id_list_append(&lv, ((pm_block_parameter_node_t *)parameters->block)->name);
      }
      else {
        pm_constant_id_list_append(&lv, null_mark);
      }
      // keywords
      for (i = 0; i < ka; i++) {
        if (nint(parameters->keywords.nodes[i]) == PM_REQUIRED_KEYWORD_PARAMETER_NODE) {
          pm_constant_id_list_append(&lv, ((pm_required_keyword_parameter_node_t *)parameters->keywords.nodes[i])->name);
        }
        else {
          pm_constant_id_list_append(&lv, ((pm_optional_keyword_parameter_node_t *)parameters->keywords.nodes[i])->name);
        }
      }
    }
    // mandatory multi
    if (0 < mma) {
      pm_constant_id_list_append(&lv, null_mark);
    }
    for (i = 0; i < ma; i++) {
      if (nint(parameters->requireds.nodes[i]) == PM_MULTI_TARGET_NODE) {
        CAST3(multi_target, parameters->requireds.nodes[i], m);
        for (int j = 0; j < m->lefts.size; j++) {
          pm_constant_id_list_append(&lv, ((pm_required_parameter_node_t *)m->lefts.nodes[j])->name);
        }
      }
    }
    for (i = 0; i < pa; i++) {
      if (nint(parameters->posts.nodes[i]) == PM_MULTI_TARGET_NODE) {
        CAST3(multi_target, parameters->posts.nodes[i], m);
        for (int j = 0; j < m->lefts.size; j++) {
          pm_constant_id_list_append(&lv, ((pm_required_parameter_node_t *)m->lefts.nodes[j])->name);
        }
      }
    }
  }
  if (!pm_constant_id_list_includes(&lv, null_mark)) {
    pm_constant_id_list_append(&lv, null_mark);
  }

  // free Prism's locals
  pm_constant_id_list_free(&cast->locals);
  cast->locals.ids = NULL;
  cast->locals.size = cast->locals.capacity = 0;

  s = scope_new(s->c, s, &lv);
  pop();

  s->mscope = !blk;

  if (blk) {
    struct loopinfo *lp = loop_push(s, LOOP_BLOCK);
    lp->pc0 = new_label(s);
  }
  if (parameters == NULL) {
    genop_W(s, OP_ENTER, 0);
    s->ainfo = 0;
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
      // TODO: `a=1;->(v=a){}`
      idx = lv_idx(s, id);
      //idx = id;
      //if (idx > 0) {
        gen_move(s, idx, cursp(), 0);
      //}
      //else {
      //  gen_getupvar(s, cursp(), id);
      //}
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
          //genop_2(s, OP_KEY_P, lv_idx(s, kwd_sym), new_sym(s, kwd_sym));
          //jmpif_key_p = genjmp2_0(s, OP_JMPIF, lv_idx(s, kwd_sym), NOVAL);
          genop_2(s, OP_KEY_P, kwd_sym, new_sym(s, kwd_sym));
          jmpif_key_p = genjmp2_0(s, OP_JMPIF, kwd_sym, NOVAL);
          codegen(s, ((pm_optional_keyword_parameter_node_t *)kwd)->value, VAL);
          pop();
          idx = lv_idx(s, kwd_sym);
          //  idx = kwd_sym;
          //if (idx > 0) {
            gen_move(s, idx, cursp(), 0);
          //}
          //else {
          //  gen_getupvar(s, cursp(), kwd_sym);
          //}
          jmp_def_set = genjmp_0(s, OP_JMP);
          dispatch(s, jmpif_key_p);
        }
        genop_2(s, OP_KARG, lv_idx(s, kwd_sym), new_sym(s, kwd_sym));
        //genop_2(s, OP_KARG, kwd_sym, new_sym(s, kwd_sym));
        if (jmp_def_set != -1) {
          dispatch(s, jmp_def_set);
        }
      }
      if (!kd) {
        genop_0(s, OP_KEYEND);
      }
    }

    /* argument destructuring */
    if (margs) {
      pos = 1;
      push();
      for (i = 0; i < ma; i++) {
        if (nint(margs[i]) == PM_MULTI_TARGET_NODE) {
          gen_massignment(s, margs[i], pos, NOVAL);
        }
        pos++;
      }
    }
    if (pargs) {
      pos = ma+oa+ra+1;
      for (i = 0; i < pa; i++) {
        if (nint(pargs[i]) == PM_MULTI_TARGET_NODE) {
          gen_massignment(s, pargs[i], pos, NOVAL);
        }
        pos++;
      }
    }
  }

  codegen(s, cast->body, VAL);
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
  switch (nint(tree)) {
    case PM_PROGRAM_NODE: {
      scope_body(s, tree, val);
      break;
    }
    case PM_STATEMENTS_NODE:
    {
      CAST(statements);
      size_t last_index = cast->body.size;
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
        if (cast->depth == 0) {
          gen_move(s, cursp(), lv_idx(s, cast->name), val);
        }
        else {
          // TODO
          // gen_getupvar(s, cursp(), nsym(tree));
        }
        push();
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
      int len = 0, n = 0, post = 0;
      CAST3(array, cast->value, t);
      int rhs = cursp();

      if (!val && nint((mrc_node *)t) == PM_ARRAY_NODE && !(t->base.flags & PM_ARRAY_NODE_FLAGS_CONTAINS_SPLAT) ) {
        /* fixed rhs */
        n = 1;
        len = t->elements.size;
        for (int i = 0; i < len; i++) {
          codegen(s, t->elements.nodes[i], VAL);
        }
        if (0 < cast->lefts.size) {
          n = 0;
          for (int i = 0; i < cast->lefts.size; i++) {
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
          for (int i = 0; i < post; i++) {
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
    case PM_INTEGER_NODE:
    {
      CAST(integer);
      // todo: check overflow
      if (cast->value.length == 0) {
        if (!cast->value.negative) {
          gen_int(s, cursp(), cast->value.value);
        }
        else {
          // todo: negative
        }
      }
      else {
        // todo: big integer
      }
      push();
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
    case PM_STRING_NODE:
    {
      if (val) {
        CAST(string);
        char *p = (char*)cast->unescaped.source;
        mrc_int len = cast->unescaped.length;
        int off = new_lit_str(s, p, len);

        genop_2(s, OP_STRING, cursp(), off);
        push();
      }
      break;
    }
    case PM_DEF_NODE:
    {
      CAST(def);
      int sym = new_sym(s, cast->name);
      int idx = lambda_body(s, tree, 0);

      genop_1(s, OP_TCLASS, cursp());
      push();
      genop_2(s, OP_METHOD, cursp(), idx);
      push(); pop();
      pop();
      genop_2(s, OP_DEF, cursp(), sym);
      if (val) push();
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
    case PM_CLASS_NODE:
    {
      int idx;
      CAST(class);
      CAST3(constant_path, cast->constant_path, cpath);
      if (cpath->parent == 1) {
        genop_1(s, OP_LOADNIL, cursp());
        push();
      }
      else if (cpath->parent == 0) {
        genop_1(s, OP_OCLASS, cursp());
        push();
      }
      else {
        codegen(s, cpath->parent, VAL);
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
        idx = scope_body(s, cast, val);
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
      CAST3(constant_path, cast->constant_path, cpath);
      if (cpath->parent == 1) {
        genop_1(s, OP_LOADNIL, cursp());
        push();
      }
      else if (cpath->parent == 0) {
        genop_1(s, OP_OCLASS, cursp());
        push();
      }
      else {
        codegen(s, cpath->parent, VAL);
      }
      pop();
      idx = new_sym(s, cast->name);
      genop_2(s, OP_MODULE, cursp(), idx);
      if (!cast->body) {
        genop_1(s, OP_LOADNIL, cursp());
      }
      else {
        idx = scope_body(s, cast, val);
        genop_2(s, OP_EXEC, cursp(), idx);
      }
      if (val) {
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
    default:
    {
      printf("Not implemented (#4) %s\n", pm_node_type_to_str(nint(tree)));
      break;
    }
  }
}
