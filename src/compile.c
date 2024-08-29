#include "../include/mrc_common.h"
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
  irep = mrc_generate_code(c, ast);
  if (c->capture_errors) {
    return NULL;
  }
  if (c->dump_result) {
    {
      pm_buffer_t buffer = { 0 };
#if defined(MRC_DUMP_PRETTY)
      pm_prettyprint(&buffer, c->p, ast);
      printf("%s\n", buffer.value);
#endif
      pm_buffer_free(&buffer);
    }
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
  pm_lex_callback_t *cb = (pm_lex_callback_t *)mrc_malloc(sizeof(pm_lex_callback_t));
  cb->data = c;
  cb->callback = partial_hook;
  pm_parser_init(p, *source, size, NULL);
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
  uint8_t *buffer = mrc_malloc(INITIAL_BUF_SIZE);
  if (buffer == NULL) return -1;

  int capacity = INITIAL_BUF_SIZE;
  size_t length = 0;

  while (1) {
    int ch = getchar();
    if (ch == EOF) {
      buffer[length] = '\0';
      memccpy(*source + source_length, buffer, 1, length);
      mrc_free(buffer);
      return length;
    }

    buffer[length++] = (uint8_t)ch;

    if (capacity <= length) {
      capacity *= 2;
      uint8_t *new_buffer = mrc_realloc(buffer, capacity);
      if (new_buffer == NULL) {
        mrc_free(buffer);
        return -1;
      }
      buffer = new_buffer;
    }
  }
}

static ssize_t
read_input_files(mrc_ccontext *c, char **filenames, uint8_t **source, mrc_filename_table *filename_table)
{
  int i = 0;
  size_t pos = 0;
  ssize_t length = 0;
  ssize_t each_size;
  FILE *file;
  char *filename = filenames[0];
  while (filename) {
    mrc_filename_table entry = { filenames[i], pos };
    filename_table[i] = entry;
    if (filename[0] == '-' && filename[1] == '\0') {
      if (*source == NULL) {
        *source = (uint8_t *)mrc_malloc(length);
      }
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
        *source = (uint8_t *)mrc_malloc(length + 1);
      }
      else {
        *source = (uint8_t *)mrc_realloc(*source, length + 1);
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
mrc_parse_file_cxt(mrc_ccontext *c, const char **filenames, uint8_t **source)
{
  size_t filecount = 0;
  while (filenames[filecount]) {
    filecount++;
  }
  c->filename_table = (mrc_filename_table *)mrc_malloc(sizeof(mrc_filename_table) * filecount);
  c->filename_table_length = filecount;
  c->current_filename_index = 0;
  ssize_t length = read_input_files(c, (char **)filenames, source, c->filename_table);
  if (length < 0) {
    fprintf(stderr, "cannot open files\n");
    return NULL;
  }
  mrc_pm_parser_init(c->p, source, length, c);
  return pm_parse(c->p);
}

mrc_irep *
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
  pm_string_t string;
  pm_string_owned_init(&string, (uint8_t *)source, length);
  c->filename_table = (mrc_filename_table *)mrc_malloc(sizeof(mrc_filename_table));
  mrc_filename_table entry = { "-e", length };
  c->filename_table[0] = entry;
  c->filename_table_length = 1;
  c->current_filename_index = 0;
  mrc_pm_parser_init(c->p, (uint8_t **)string.source, string.length, c);
  return pm_parse(c->p);
}

mrc_irep *
mrc_load_string_cxt(mrc_ccontext *c, const uint8_t **source, size_t length)
{
  mrc_node *root = mrc_parse_string_cxt(c, source, length);
  mrc_irep *irep = mrc_load_exec(c, root);
  pm_node_destroy(c->p, root);
  return irep;
}
