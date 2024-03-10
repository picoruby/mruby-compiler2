#ifndef MRC_PARSER_H
#define MRC_PARSER_H

#include "mrc_common.h"

MRC_BEGIN_DECL

#if defined(MRC_PARSER_PRISM)
  #include "prism.h" // in lib/prism/include
  typedef pm_node_t mrc_node;
  typedef pm_parser_t mrc_parser_state;
  typedef struct {
    pm_parser_t parser;
    pm_options_t options;
    pm_string_t input;
    bool parsed;
  } pm_parse_result_t;
#elif defined(MRC_PARSER_KANEKO)
  // TODO
#else
  #error "No parser defined. Please define MRC_PARSER_PRISM or MRC_PARSER_KANEKO."
#endif

MRC_END_DECL

#endif // MRC_PARSER_H
