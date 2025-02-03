#ifndef PRISM_CUSTOM_ALLOCATOR_H
#define PRISM_CUSTOM_ALLOCATOR_H

#if defined(MRC_TARGET_MRUBY)
  extern mrb_state *global_mrb;

  #define xmalloc(size)             mrb_malloc(global_mrb, size)
  #define xcalloc(nmemb,size)       mrb_calloc(global_mrb, nmemb, size)
  #define xrealloc(ptr,size)        mrb_realloc(global_mrb, ptr, size)
  #define xfree(ptr)                mrb_free(global_mrb, ptr)

  #define mrc_malloc(c,size)        mrb_malloc(c->mrb, size)
  #define mrc_calloc(c,nmemb,size)  mrb_calloc(c->mrb, nmemb, size)
  #define mrc_realloc(c,ptr,size)   mrb_realloc(c->mrb, ptr, size)
  #define mrc_free(c,ptr)           mrb_free(c->mrb, ptr)
#elif defined(MRC_TARGET_MRUBYC)
  #if defined(MRBC_ALLOC_LIBC)
    #define mrc_malloc(c,size)        malloc(size)
    #define mrc_calloc(c,nmemb,size)  calloc(nmemb, size)
    #define mrc_realloc(c,ptr,size)   realloc(ptr, size)
    #define mrc_free(c,ptr)           free(ptr)
  #else
    #define xmalloc(size)             mrbc_raw_alloc(size)
    #define xcalloc(nmemb,size)       mrbc_raw_calloc(nmemb, size)
    #define xrealloc(ptr,size)        mrbc_raw_realloc(ptr, size)
    #define xfree(ptr)                mrbc_raw_free(ptr)

    #define mrc_malloc(c,size)        mrbc_raw_alloc(size)
    #define mrc_calloc(c,nmemb,size)  mrbc_raw_calloc(nmemb, size)
    #define mrc_realloc(c,ptr,size)   mrbc_raw_realloc(ptr, size)
    #define mrc_free(c,ptr)           mrbc_raw_free(ptr)
  #endif
#endif

#endif

