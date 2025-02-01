#ifdef MRC_CUSTOM_ALLOC

#include <prism_xallocator.h>
#include <string.h>

#ifdef MRC_TARGET_MRUBY

#include <mruby.h>
#define picorb_alloc(c, size)           mrb_malloc(c, size)
#define picorb_calloc(c, nmemb, size)   mrb_calloc(c, nmemb, size)
#define picorb_realloc(c, ptr, size)    mrb_realloc(c, ptr, size)
#define picorb_free(c, ptr)             mrb_free(c, ptr)

#else /* MRC_TARGET_MRUBYC */

#include <mrubyc.h>
#define picorb_alloc    mrbc_raw_alloc
#define picorb_calloc   mrbc_raw_calloc
#define picorb_free     mrbc_raw_free
/*
 * mrbc_raw_realloc() warns if ptr is NULL.
 * So, we need to implement our own realloc.
 */
void*
picorb_realloc(void *ptr, unsigned int size)
{
  if (ptr == NULL) {
    return mrbc_raw_alloc(size);
  } else {
    return mrbc_raw_realloc(ptr, size);
  }
}

#endif /* MRC_TARGET_MRUBYC */

#endif // MRC_CUSTOM_ALLOC
