#include <assert.h>

#include "mutator.h"
#include "rng.h"

u64 rng_next(Rng* self) {
	u64 out = self->seed;

	self->seed ^= self->seed << 13;
	self->seed ^= self->seed >> 17;
	self->seed ^= self->seed << 43;
	return out;
}

u64 rng_rand(Rng* self, u64 min, u64 max) {

	assert(min <= max);

	if (min == max)
		return min;

	if (min == 0 && max == U64_MAX)
		return rng_next(self);

	return min + (rng_next(self) % (max - min + 1));
}

u64 rng_exp(Rng* self, u64 min, u64 max) {

	if (self->exp_disabled)
		return rng_rand(self, min, max);

	if (rng_rand(self, 0, 1) == 0)
		return rng_rand(self, min, max);
	
	return rng_rand(self, min, rng_rand(self, min, max));
}
