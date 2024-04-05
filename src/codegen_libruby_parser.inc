
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

static void
codegen(mrc_codegen_scope *s, mrc_node *node, int indent)
{
  int rlev = s->rlev;

  if (!node) {
    if (indent) {
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
      PRINT(stderr, "BEGIN:\n");
      codegen(s, RNODE_BEGIN(node)->nd_body, indent+1);
      return;
    case NODE_SCOPE:
      PRINT(stderr, "SCOPE:\n");
      // codegen(s, RNODE_SCOPE(node)->nd_args, indent+1);
      codegen(s, RNODE_SCOPE(node)->nd_body, indent+1);
      return;
    case NODE_OPCALL:
      PRINT(stderr, "OPCALL:\n");
      PRINT_ID(stderr, RNODE_OPCALL(node)->nd_mid);
      codegen(s, RNODE_OPCALL(node)->nd_recv, indent+1);
      codegen(s, RNODE_OPCALL(node)->nd_args, indent+1);
      return;
    case NODE_LIST:
      PRINT(stderr, "LIST:\n");
      print_array(stderr, s, indent, node);
      return;
    case NODE_FCALL:
      PRINT(stderr, "FCALL:\n");
      PRINT_ID(stderr, RNODE_FCALL(node)->nd_mid);
      codegen(s, RNODE_FCALL(node)->nd_args, indent+1);
      return;
    case NODE_SYM:
      PRINT(stderr, "SYM:\n");
      PRINT_STR(stderr, RNODE_SYM(node)->string);
      return;
    case NODE_STR:
      PRINT(stderr, "STR:\n");
      PRINT_STR(stderr, RNODE_STR(node)->string);
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
