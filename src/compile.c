#include "../include/mrc_parser_util.h"
#include "../include/mrc_irep.h"
#include "../include/mrc_ccontext.h"
#include "../include/mrc_codegen.h"
#include "../include/mrc_dump.h"
#include "../include/mrc_codedump.h"
#include "../include/mrc_opcode.h"
#include "../include/mrc_presym.h"
#include "../include/mrc_diagnostic.h"

static mrc_irep *
mrc_load_exec(mrc_ccontext *c, mrc_node *ast)
{
  mrc_irep *irep;
  /* parse error */
  if (0 < c->p->error_list.size) {
    pm_diagnostic_t *e = (pm_diagnostic_t *)c->p->error_list.head;
    while (e) {
      mrc_diagnostic_list_append(c, e->location.start, e->message, MRC_PARSER_ERROR);
      e = (pm_diagnostic_t *)e->node.next;
    }
    return NULL;
  }
  /* parse warning */
  if (0 < c->p->warning_list.size) {
    pm_diagnostic_t *w = (pm_diagnostic_t *)c->p->warning_list.head;
    while (w) {
      mrc_diagnostic_list_append(c, w->location.start, w->message, MRC_PARSER_WARNING);
      w = (pm_diagnostic_t *)w->node.next;
    }
  }
#if defined(MRC_DUMP_PRETTY) && !defined(MRC_NO_STDIO)
  if (c->dump_result) {
    pm_buffer_t buffer = { 0 };
    pm_prettyprint(&buffer, c->p, ast);
    fprintf(stderr, "%s\n", buffer.value);
    pm_buffer_free(&buffer);
  }
#endif
  irep = mrc_generate_code(c, ast);
  if (c->capture_errors) {
    return NULL;
  }
  if (c->dump_result) {
    mrc_codedump_all(c, irep);
  }

  return irep;
}

static void
partial_hook(void *data, pm_parser_t *p, pm_token_t *token)
{
  mrc_ccontext *c = (mrc_ccontext *)data;
  if (c->current_filename_index + 1 == c->filename_table_length) {
    return;
  }
  uint32_t token_pos = (uint32_t)(token->start - p->start);
  if (token_pos < c->filename_table[c->current_filename_index].start) {
    return;
  }
  if (c->filename_table[c->current_filename_index + 1].start <= token_pos) {
    c->current_filename_index++;
    pm_string_t filename_string;
    pm_string_constant_init(
        &filename_string,
        c->filename_table[c->current_filename_index].filename,
        strlen(c->filename_table[c->current_filename_index].filename));
    p->filepath = filename_string;
  }
}

static void
mrc_pm_parser_init(mrc_parser_state *p, uint8_t **source, size_t size, mrc_ccontext *c)
{
  pm_lex_callback_t *cb = (pm_lex_callback_t *)mrc_malloc(c, sizeof(pm_lex_callback_t));
  cb->data = c;
  cb->callback = partial_hook;
  pm_parser_init(p, *source, size, c->options);
  p->lex_callback = cb;
  mrc_init_presym(&p->constant_pool);
  if (c->filename_table) {
    pm_string_t filename_string;
    pm_string_constant_init(&filename_string, c->filename_table[0].filename,
                                             strlen(c->filename_table[0].filename));
    p->filepath = filename_string;
  }
}

#ifndef MRC_NO_STDIO

#define INITIAL_BUF_SIZE 1024
static ssize_t
append_from_stdin(mrc_ccontext *c, uint8_t **source, size_t source_length)
{
  uint8_t *buffer = mrc_malloc(c, INITIAL_BUF_SIZE);
  if (buffer == NULL) return -1;

  int capacity = INITIAL_BUF_SIZE;
  size_t length = 0;

  while (1) {
    int ch = getchar();
    if (ch == EOF) {
      buffer[length] = '\0';
      if (*source == NULL)
        *source = (uint8_t *)mrc_malloc(c, source_length + length + 1);
      else
        *source = (uint8_t *)mrc_realloc(c, *source, source_length + length + 1);
      memccpy(*source + source_length, buffer, 1, length);
      mrc_free(c, buffer);
      return length;
    }

    buffer[length++] = (uint8_t)ch;

    if (capacity <= length) {
      capacity *= 2;
      uint8_t *new_buffer = mrc_realloc(c, buffer, capacity);
      if (new_buffer == NULL) {
        mrc_free(c, buffer);
        return -1;
      }
      buffer = new_buffer;
    }
  }
}

static ssize_t
read_input_files(mrc_ccontext *c, const char **filenames, uint8_t **source, mrc_filename_table *filename_table)
{
  int i = 0;
  size_t pos = 0;
  ssize_t length = 0;
  ssize_t each_size;
  FILE *file;
  const char *filename = filenames[0];
  while (filename) {
    filename_table[i].filename = filenames[i];
    filename_table[i].start = pos;
    if (filename[0] == '-' && filename[1] == '\0') {
      each_size = append_from_stdin(c, source, length);
      if (each_size < 0) {
        fprintf(stderr, "compile.c: cannot read from stdin\n");
        return -1;
      }
      length += each_size;
    }
    else {
      file = NULL;
      file = fopen(filename, "rb");
      if (!file) {
        fprintf(stderr, "compile.c: cannot open program file. (%s)\n", filename);
        return -1;
      }
      fseek(file, 0, SEEK_END);
      each_size = ftell(file);
      fseek(file, 0, SEEK_SET);
      length += each_size;
      if (*source == NULL) {
        *source = (uint8_t *)mrc_malloc(c, length + 1);
      }
      else {
        *source = (uint8_t *)mrc_realloc(c, *source, length + 1);
      }
      if (fread(*source + pos, sizeof(char), each_size, file) != each_size) {
        fprintf(stderr, "compile.c: cannot read program file. (%s)\n", filename);
        fclose(file);
        return -1;
      }
      fclose(file);
      (*source)[length] = '\0';
    }
    pos += each_size;
    filename = filenames[++i];
  }
  return length;
}

static mrc_node *
mrc_pm_parse(mrc_ccontext *c)
{
  mrc_node *node = pm_parse(c->p);

  // save top-level locals for IRB
  pm_program_node_t *program = (pm_program_node_t *)node;
  uint32_t nlocals = program->locals.size;
  pm_options_t *options = (pm_options_t *)mrc_malloc(c, sizeof(pm_options_t));
  memset(options, 0, sizeof(pm_options_t));
  pm_string_t *encoding = &options->encoding;
  pm_string_constant_init(encoding, "UTF-8", 5);
  pm_options_scopes_init(options, 1);
  pm_options_scope_t *options_scope = &options->scopes[0];
  pm_options_scope_init(options_scope, nlocals);
  pm_constant_id_t id;
  pm_constant_t *local;
  pm_string_t *scope_local;
  char *allocated;
  for (int i = 0; i < nlocals; i++) {
    scope_local = &options_scope->locals[i];
    id = program->locals.ids[i];
    local = pm_constant_pool_id_to_constant(&c->p->constant_pool, id);
    allocated = (char *)mrc_malloc(c, local->length);
    memcpy(allocated, local->start, local->length);
    pm_string_constant_init(scope_local, (const char *)allocated, local->length);
  }
  if (c->options && c->options->scopes) {
    for (int i = 0; i < c->options->scopes[0].locals_count; i++) {
      mrc_free(c, (void *)c->options->scopes[0].locals[i].source);
    }
    mrc_free(c, c->options);
  }
  c->options = options;

  return node;
}


static mrc_node *
mrc_parse_file_cxt(mrc_ccontext *c, const char **filenames, uint8_t **source)
{
  size_t filecount = 0;
  while (filenames[filecount]) {
    filecount++;
  }
  c->filename_table = (mrc_filename_table *)mrc_malloc(c, sizeof(mrc_filename_table) * filecount);
  c->filename_table_length = filecount;
  c->current_filename_index = 0;
  ssize_t length = read_input_files(c, filenames, source, c->filename_table);
  if (length < 0) {
    fprintf(stderr, "Cannot open files: ");
    for (size_t i = 0; i < filecount; i++) {
      fprintf(stderr, "%s ", filenames[i]);
    }
    fprintf(stderr, "\n");
    return NULL;
  }
  mrc_pm_parser_init(c->p, source, length, c);
  return mrc_pm_parse(c);
}

MRC_API mrc_irep *
mrc_load_file_cxt(mrc_ccontext *c, const char **filenames, uint8_t **source)
{
  mrc_node *root = mrc_parse_file_cxt(c, filenames, source);
  if (root == NULL) {
    return NULL;
  }
  mrc_irep *irep = mrc_load_exec(c, root);
  pm_node_destroy(c->p, root);
  return irep;
}
#endif

static mrc_node *
mrc_parse_string_cxt(mrc_ccontext *c, const uint8_t **source, size_t length)
{
  c->filename_table = (mrc_filename_table *)mrc_malloc(c, sizeof(mrc_filename_table));
  c->filename_table[0].filename = "-e";
  c->filename_table[0].start = 0;
  c->filename_table_length = 1;
  c->current_filename_index = 0;
  mrc_pm_parser_init(c->p, (uint8_t **)source, length, c);
  return mrc_pm_parse(c);
}

MRC_API mrc_irep *
mrc_load_string_cxt(mrc_ccontext *c, const uint8_t **source, size_t length)
{
  mrc_node *root = mrc_parse_string_cxt(c, source, length);
  mrc_irep *irep = mrc_load_exec(c, root);
  return irep;
}

#if defined(MRC_TARGET_MRUBY)

MRC_API void
mrb_mruby_compiler2_gem_init(mrb_state *mrb)
{
}

MRC_API void
mrb_mruby_compiler2_gem_final(mrb_state *mrb)
{
}

#endif
