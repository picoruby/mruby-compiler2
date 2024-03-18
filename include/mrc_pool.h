#ifndef MRC_POOL_H
#define MRC_POOL_H

#include "mrc_ccontext.h"

MRC_BEGIN_DECL

typedef struct mrc_pool mrc_pool;

mrc_pool *mrc_pool_open(mrc_ccontext *c);
void mrc_pool_close(mrc_pool *pool);
void *mrc_pool_alloc(mrc_pool *pool, size_t len);
void *mrc_pool_realloc(mrc_pool *pool, void *p, size_t oldlen, size_t newlen);

MRC_END_DECL

#endif // MRC_POOL_H

