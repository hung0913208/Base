#ifndef BASE_ATOMIC_H_
#define BASE_ATOMIC_H_
#include <Type.h>
#include <string.h>
#include <unistd.h>

#if __cplusplus
extern "C" {
#endif

/* @NOTE: Pause instruction to prevent excess processor bus usage */
#define RELAX() asm volatile("pause\n": : :"memory")
//#define RELAX() usleep(0)

/* @NOTE: Atomic exchange (of various sizes) */
static inline ULLong XCHG64(Void* ptr, ULLong x) {
  __asm__ __volatile__("xchgq %0,%1"
                       :"=r" ((ULLong) x)
                       :"m" (*(volatile LLong *)ptr), "0" ((ULLong) x)
                       :"memory");

  return x;
}

/* @NOTE: Atomic exchange (of various sizes) */
static inline UInt XCHG32(Void *ptr, UInt x) {
   __asm__ __volatile__("xchgl %0,%1"
                        :"=r" ((UInt) x)
                        :"m" (*(volatile unsigned *)ptr), "0" (x)
                        :"memory");

  return x;
}

static inline UShort XCHG16(Void *ptr, UShort x) {
  __asm__ __volatile__("xchgw %0,%1"
                       :"=r" ((UShort) x)
                       :"m" (*(volatile UShort *)ptr), "0" (x)
                       :"memory");
  return x;
}

#if __cplusplus
}
#endif

#if defined(__GNUC__)
/* @NOTE: Atomic compare and exchange */
#define CMPXCHG(P, O, N) __sync_bool_compare_and_swap((P), (O), (N))

/* @NOTE: Atomic functions */
#define XADD(P, V) __sync_fetch_and_add((P), (V))
#define INC(P) __sync_add_and_fetch((P), 1)
#define DEC(P) __sync_add_and_fetch((P), -1)
#define ADD(P, V) __sync_add_and_fetch((P), (V))
#define SET(P, V) __sync_or_and_fetch((P), (V))
#define CLR(P, V) __sync_and_and_fetch((P), ~(V))
#define ACK(P, V) __sync_lock_test_and_set((P), (V))
#define REL(P) __sync_lock_release((P))
#define BARRIER() __sync_synchronize()
#else
/* @NOTE: Atomic compare and exchange */
#define CMPXCHG(P, O, N) InterlockedCompareExchange((P), (O), (N))

/* @NOTE: Atomic functions */
#define XADD(P, V) InterlockedExchangeAdd((P), (V))
#define INC(P) InterlockedIncrement((P))
#define DEC(P) InterlockedDecrement((P))
#define SET(P, BIT) InterlockedOr((P), 1<<(BIT))
#define CLR(P, BIT) InterlockedAnd((P), ~(1<<(BIT)))
#define ACK(P, V) XCHG((P), (V))
#define REL(P) XCHG((P), 0)
#endif

/* @NOTE: generic atomic functions */
#define CMP(P, O) CMPXCHG((P), (O), *(P))
#define MCOPY(D, O, S) memcpy((D), (O), (S))

#ifndef BARRIER
/* @NOTE: Compile read-write barrier */
#define BARRIER() asm volatile("": : :"memory")
#endif  // BARRIER
#endif  // BASE_ATOMIC_H_
