#ifndef PRISM_CUSTOM_ALLOCATOR_H
#define PRISM_CUSTOM_ALLOCATOR_H

#if defined(MRC_TARGET_MRUBY)
  #include <mruby.h>

  extern mrb_state *global_mrb;

  #define xmalloc(size)             mrb_malloc(global_mrb, size)
  #define xcalloc(nmemb,size)       mrb_calloc(global_mrb, nmemb, size)
  #define xrealloc(ptr,size)        mrb_realloc(global_mrb, ptr, size)
  #define xfree(ptr)                mrb_free(global_mrb, ptr)

  #define mrc_malloc(c,size)        mrb_malloc(c->mrb, size)
  #define mrc_calloc(c,nmemb,size)  mrb_calloc(c->mrb, nmemb, size)
  #define mrc_realloc(c,ptr,size)   mrb_realloc(c->mrb, ptr, size)
  #define mrc_free(c,ptr)           mrb_free(c->mrb, ptr)
#else
  #include <mrubyc.h>
  // TODO
#endif

#endif

