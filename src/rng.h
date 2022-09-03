#ifndef __RNGMTT_H
#define __RNGMTT_H

#ifdef UINT64_MAX
	#define U64_MAX UINT64_MAX
	#define u64fmt PRIu64
	typedef uint64_t u64;
#else
	#define U64_MAX ULLONG_MAX
	#define u64fmt "llu"
	typedef unsigned long long int u64;	
#endif

typedef struct {
	u64 seed;
	int exp_disabled;
} Rng;

u64 rng_next(Rng* self);
u64 rng_rand(Rng* self, u64 min, u64 max);
u64 rng_exp(Rng* self, u64 min, u64 max);

#endif
