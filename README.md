# cmutator #
A basic input mutator, blatantly based on Brandon Falk's [basic_mutator](https://github.com/gamozolabs/basic_mutator) (which itself is based on [honggfuzz](https://github.com/google/honggfuzz)), written as an exercise.

### TODO ###
* Add coverage-guided fuzzing support.

## Usage ##
Compile as a static library with `make libcmutator.a` and compile with your application:

`gcc <your_program.c> libcmutator.a -I <path_to_cmutator>/src -o <your_program>`

### API ###

The following is the full API. The example program ([main.c](src/main.c)) also serves as documentation.

```c
/*
 * Only the `input` and `input_size` fields should be accessed directly.
 * `input` contains the mutated byte string.
 * `input_size` contains its size, which may vary after mutation.
 */
typedef struct {
	unsigned char* input;
	size_t input_size;
	const size_t max_input_size;
	Rng rng;
	const int printable;
} Mutator;

/*
 * Initializes new mutator for inputs with maximum size of `max_input_size`
 * `seed` is used for the PRNG.
 * `printable` indicates whether the resulting mutated string should contain only printable
 * characters (1) or any value (0).
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
```
