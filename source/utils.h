#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdlib.h>
/* Constants */
#define KB_SIZE		1024.0
#define MB_SIZE		1048576.0
#define GB_SIZE		1073741824.0

/* Macros */
#ifdef __builtin_align_up
# define round_up(x,n)	__builtin_align_up(x, n)
#else
# define round_up(x,n) ((x + n - 1) & ~(n - 1))
#endif

/* Prototypes */
u32 swap32(u32);

static inline void *memalign32(size_t size)
{
    return aligned_alloc(0x20, (size + 0x1F) & ~0x1F);
}

static inline void *memalign64(size_t size)
{
    return aligned_alloc(0x40, (size + 0x3F) & ~0x3F);
}

#endif
