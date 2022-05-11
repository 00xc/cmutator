#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mutator.h"
#include "strategy.h"

int mutator_new(Mutator** self, size_t max_input_size, u64 seed, int printable) {
	Mutator* out;

	out = calloc(1, sizeof(Mutator));
	if (out == NULL)
		return 0;

	out->input = calloc(max_input_size, sizeof(char));
	if (out->input == NULL) {
		mutator_free(out);
		return 0;
	}
	
	*(size_t*)&out->max_input_size = max_input_size;
	*(int*)&out->printable = printable;
	out->rng.seed = seed;
	out->rng.exp_disabled = 0;

	*self = out;
	return 1;
}

void mutator_clear_input(Mutator* self) {
	self->input_size = 0;
}

int mutator_set_input(Mutator* self, void* input, size_t size) {

	if (size > self->max_input_size)
		return 0;

	self->input_size = size;
	memcpy(self->input, input, size);

	return 1;
}

void mutator_mutate(Mutator* self, unsigned int passes) {
	unsigned int i;
	void (*fn)(Mutator* m);

	for (i = 0; i < passes; ++i) {
		fn  = strategy_get_random(&(self->rng));
		fn(self);	
	}
}

void mutator_free(Mutator* self) {

	if (self == NULL)
		return;

	if (self->input != NULL)
		free(self->input);

	free(self);
}
