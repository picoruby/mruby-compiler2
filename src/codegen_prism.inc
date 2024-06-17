
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
  mrc_codegen_scope *scope = scope_new(s->c, s, nlv);

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
    case PM_LOCAL_VARIABLE_READ_NODE:
    {
      if (val) {
        pm_local_variable_read_node_t *cast = (pm_local_variable_read_node_t *)tree;
        if (cast->depth == 0) {
          gen_move(s, cursp(), cast->name, val);
        }
        else {
          // TODO
          // gen_getupvar(s, cursp(), nsym(tree));
        }
        push();
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
