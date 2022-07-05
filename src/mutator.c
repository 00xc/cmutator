#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mutator.h"
#include "strategy.h"

int mutator_init(Mutator* self, size_t max_input_size, u64 seed, int printable) {

	self->input = calloc(max_input_size, sizeof(char));
	if (self->input == NULL)
		return 0;
	
	*(size_t*)&self->max_input_size = max_input_size;
	*(int*)&self->printable = printable;
	self->rng.seed = seed;
	self->rng.exp_disabled = 0;

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
}
