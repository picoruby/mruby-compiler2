#ifndef MRC_COMMON_H
#define MRC_COMMON_H

#include <stdint.h>

#ifdef MRC_CUSTOM_ALLOC
  #include <mrc_custom_alloc.h>
#else
  #include <stdlib.h>
  #define xmalloc   malloc
  #define xrealloc  realloc
  #define xcalloc   calloc
  #define xfree     free
#endif

typedef void mrb_state;

#ifdef MRB_USE_CXX_ABI
#define MRC_USE_CXX_ABI
#endif

#ifdef __cplusplus
#ifdef MRC_USE_CXX_ABI
#define MRC_BEGIN_DECL
#define MRC_END_DECL
#else
# define MRC_BEGIN_DECL extern "C" {
# define MRC_END_DECL }
#endif
#else
/** Start declarations in C mode */
# define MRC_BEGIN_DECL
/** End declarations in C mode */
# define MRC_END_DECL
#endif


#if defined(__cplusplus) || (defined(__bool_true_false_are_defined) && __bool_true_false_are_defined)
typedef bool mrc_bool;

# ifndef FALSE
#  define FALSE false
# endif
# ifndef TRUE
#  define TRUE true
# endif
#else
# if __STDC_VERSION__ >= 199901L
typedef _Bool mrc_bool;
# else
typedef uint8_t mrc_bool;
# endif

# ifndef FALSE
#  define FALSE 0
# endif
# ifndef TRUE
#  define TRUE 1
# endif
#endif

#ifdef MRB_NO_FLOAT
#define MRC_NO_FLOAT
#endif
#ifdef MRB_USE_FLOAT32
#define MRC_USE_FLOAT32
#endif

#ifndef MRC_NO_FLOAT
#ifdef MRC_USE_FLOAT32
  typedef float mrc_float;
#else
  typedef double mrc_float;
#endif
#endif

typedef uint32_t mrc_sym;
typedef uint8_t mrc_code;

#endif  /* MRC_COMMON_H */
