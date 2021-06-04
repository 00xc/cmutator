#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mutator.h"

#define NUM_MUTATIONS 4
#define NUM_ROUNDS    5000000

int main() {
	unsigned int i;
	Mutator* m;
	
	if (!mutator_new(&m, 1024, 1337, 1))
		err(EXIT_FAILURE, "mutator_new");

	for (i = 0; i < NUM_ROUNDS; ++i) {

		if (!mutator_set_input(m, "Something", strlen("Something"))) {
			mutator_free(m);
			errx(EXIT_FAILURE, "mutator_set_input");
		}

		mutator_mutate(m, NUM_MUTATIONS);
		assert(m->input_size <= 1024);
		mutator_clear_input(m);
	}

	mutator_free(m);

	printf("Performed %d mutation rounds, %d passes each\n", NUM_ROUNDS, NUM_MUTATIONS);

	return 0;
}