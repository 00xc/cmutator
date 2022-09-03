#ifndef __MUTATORMTT_H
#define __MUTATORMTT_H

#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>

#include "rng.h"

typedef struct {
	unsigned char* input;
	size_t input_size;
	const size_t max_input_size;
	Rng rng;
	const int printable;
} Mutator;

/*
 * Initializes a new mutator for inputs with maximum size of `max_input_size`
 * Returns 1 on success, 0 on failure.
 */
int mutator_init(Mutator* self, size_t max_input_size, u64 seed, int printable);

/*
 * Sets a new input to mutate. `size` must be equal or smaller than the `max_input_size`
 * set with `mutator_new`.
 * Returns 1 on success, 0 on failure.
 */
int mutator_set_input(Mutator* self, void* input, size_t size);

/*
 * Clears the input previously set with `mutator_set_input`.
 */
void mutator_clear_input(Mutator* self);

/*
 * Perform `passes` rounds mutating the input, each with a random strategy.
 */
void mutator_mutate(Mutator* self, unsigned int passes);

/*
 * Frees the memory allocated by `mutator_init`
 */
void mutator_free(Mutator* self);

#endif
