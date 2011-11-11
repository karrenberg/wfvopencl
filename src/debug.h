#ifndef DEBUG_H__
#define DEBUG_H__

#ifdef DEBUG
#define WFVOPENCL_DEBUG(x) do { x } while (false)
#else
#define WFVOPENCL_DEBUG(x) ((void)0)
#endif

#ifdef DEBUG_RUNTIME
#define WFVOPENCL_DEBUG_RUNTIME(x) do { x } while (false)
#else
#define WFVOPENCL_DEBUG_RUNTIME(x) ((void)0)
#endif

#ifdef NDEBUG // force debug output disabled
#undef WFVOPENCL_DEBUG
#define WFVOPENCL_DEBUG(x) ((void)0)
#define WFVOPENCL_DEBUG_RUNTIME(x) ((void)0)
#endif

#ifdef DEBUG
#define DEBUG_LA(x) do { x } while (false)
#else
#define DEBUG_LA(x) ((void)0)
#endif

#ifdef NDEBUG // force debug output disabled
#undef DEBUG_LA
#define DEBUG_LA(x) ((void)0)
#endif

#endif
