#ifndef __MIC_H__
#define __MIC_H__

#ifdef RUN_MIC
#include <offload.h>
#endif

// Helper for memory management on phi.
#define ALLOC alloc_if(1) free_if(0)
#define FREE alloc_if(0) free_if(1)
#define REUSE alloc_if(0) free_if(0)

#endif /* __MIC_H__ */
