#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "magic.h"
#include "strategy.h"

#define ARR_SIZE(x) sizeof(x)/sizeof(x[0])
#define SWAP(a, b) do { a ^= b; b ^= a; a ^= b; } while (0)

typedef unsigned char uchar;
typedef void (*mut_function)(Mutator*);

static void block_swap(uchar* x, uchar* y, size_t len) {
	size_t i;

	for (i = 0; i < len; ++i) {
		SWAP(x[i], y[i]);
	}
}

static inline uchar make_printable(uchar c) {
	return (c - 32) % 95 + 32;
}

static inline u64 get_random_offset(Mutator* m, int plusone) {

	if (m->input_size == 0)
		return 0;

	return rng_exp(&m->rng, 0, m->input_size - (plusone == 0));
}

static inline size_t umin(u64 x, u64 y) {
	if (x < y) 
		return x;
	return y;
}

/*
 * Expands the input with `amount` uninitialized bytes at `offset`
 * |---|-----------|
 *     ^offset
 *
 *       amount
 *      <------->
 * |---|xxxxxxxxx|-----------|
 *     ^offset
 */
static inline void make_space(Mutator* m, size_t offset, size_t amount) {

	if (amount == 0)
		return;

	memmove(m->input + offset + amount, m->input + offset, m->input_size - offset);
	m->input_size += amount;
}

static inline u64 sat_sub_u64(u64 x, u64 y) {
	u64 res = x - y;
	res &= -(res <= x);
	return res;
}

/* Shift a chunk of the input to overwrite a lower block */
static void shrink(Mutator* m) {
	size_t offset, max_remove, remove;
	Rng* rng = &m->rng;

	if (m->input_size == 0)
		return;

	/* Get random offset and max. number of bytes that can be removed */
	offset = get_random_offset(m, 0);
	max_remove = m->input_size - offset;

	/* 15 out of 16 times we remove 16 bytes at most */
	if (rng_rand(rng, 0, 16) != 0) {
		max_remove = umin(max_remove, 16);
	}

	/* Actual amount of bytes to remove */
	remove = rng_exp(rng, 1, max_remove);

	/* Remove bytes */
	memmove(m->input + offset, m->input + offset + remove, m->input_size - (offset + remove));
	m->input_size -= remove;
}

/*
 * Add a new block of data at random offset in the input. If the input is printable, it is filled with spaces, otherwise
 * null bytes.
 */
static void expand(Mutator* m) {
	size_t offset, max_expand, expand;
	Rng* rng = &m->rng;

	if (m->input_size >= m->max_input_size)
		return;

	/* Get random offset and max. number of bytes that can be added */
	offset = get_random_offset(m, 1);
	max_expand = m->max_input_size - m->input_size;

	/* 15 out of 16 times we expand 16 bytes at most */
	if (rng_rand(rng, 0, 16) != 0) {
		max_expand = umin(max_expand, 16);
	}

	/* Actual amount to expand */
	expand = rng_exp(rng, 1, max_expand);

	/* Make space and fill it */
	make_space(m, offset, expand);
	memset(m->input + offset, (m->printable) ? ' ' : '\0', expand);
}

/* Flip a random bit in a single byte of the input */
static void bit(Mutator* m) {
	size_t offset;
	Rng* rng = &m->rng;

	if (m->input_size == 0)
		return;

	offset = get_random_offset(m, 0);
	m->input[offset] ^= (1 << rng_rand(rng, 0, 7));
}

/* Increase by 1 a random byte of the input */
static void inc_byte(Mutator* m) {
	size_t offset;

	if (m->input_size == 0)
		return;

	offset = get_random_offset(m, 0);
	m->input[offset] += 1;
}

/* Decrease by 1 a random byte of the input */
static void dec_byte(Mutator* m) {
	size_t offset;

	if (m->input_size == 0)
		return;

	offset = get_random_offset(m, 0);
	m->input[offset] -= 1;
}

/* Negate a random byte of the input */
static void neg_byte(Mutator* m) {
	size_t offset;

	if (m->input_size == 0)
		return;

	offset = get_random_offset(m, 0);
	m->input[offset] = ~(m->input[offset]);
}

/* Add or substract to a random offset, with a random integer size (u8 through u64) */
static void add_sub(Mutator* m) {
	size_t offset, remain, intsize, range, delta, tmp, i;
	Rng* rng = &m->rng;

	if (m->input_size == 0)
		return;

	offset = get_random_offset(m, 0);
	remain = m->input_size - offset;

	/* Random size of the add or subtract (1, 2, 4, or 8 bytes) */
	if (remain >= 8) intsize = 1 << rng_rand(rng, 0, 3);
	else if (remain >= 4) intsize = 1 << rng_rand(rng, 0, 2);
	else if (remain >= 2) intsize = 1 << rng_rand(rng, 0, 1);
	else intsize = 1;

	/* Maximum number to add or substract */
	switch (intsize) {
		case 1: range = 16; break;
		case 2: range = 4096; break;
		case 4: range = 1024 * 1024; break;
		case 8: range = 256 * 1024 * 1024; break;
		default: assert(1 == 0);
	}

	/* Convert range to random number in [-range, +range] */
	delta = (int)(rng_rand(rng, 0, range * 2)) - (int)range;

	/* Read bytes as int of size `intsize` */
	memcpy(&tmp, m->input + offset, intsize);

	/* TODO: swap endianness randomly */
	tmp += delta;
	memcpy(m->input + offset, &tmp, intsize);

	if (m->printable) {
		for (i = offset; i < offset + intsize; ++i) {
			m->input[i] = make_printable(m->input[i]);
		}	
	}
}

/* Set a random amount of bytes at a random offset with a single byte */
static void set(Mutator* m) {
	char chr;
	size_t offset, len;
	Rng* rng = &m->rng;

	if (m->input_size == 0)
		return;

	offset = get_random_offset(m, 0);
	len = rng_exp(rng, 1, m->input_size - offset);

	chr = rng_rand(rng, 0, 255);
	if (m->printable)
		chr = make_printable(chr);

	memset(m->input + offset, chr, len);
}

/* Swap two blocks of the input */
static void swap(Mutator* m) {
	size_t off1, off2, len;

	if (m->input_size == 0)
		return;

	off1 = get_random_offset(m, 0);
	off2 = get_random_offset(m, 0);
	len = rng_exp(&(m->rng), 1, umin(m->input_size - off1, m->input_size - off2));

	/* Make off1 be the smaller one */
	if (off2 < off1) {
		SWAP(off1, off2);
	}

	if (off1 < off2 && off1 + len >= off2) {
		/* Block 1 overlaps into block 2 */
		size_t overlap_len = off1 + len - off2;

		memmove(m->input + off2, m->input + off1, overlap_len);
		off1 += overlap_len;
		off2 += overlap_len;
		len -= overlap_len;
	}

	block_swap(m->input + off1, m->input + off2, len);
}

/* Overwrite a random block of the input with another block */
static void copy(Mutator* m) {
	size_t src, dst, len;

	if (m->input_size == 0)
		return;

	src = get_random_offset(m, 0);
	dst = get_random_offset(m, 0);

	len = rng_exp(&(m->rng), 1, umin(m->input_size - src, m->input_size - dst));
	memmove(m->input + dst, m->input + src, len);
}

static void inter_splice(Mutator* m) {
	size_t src, dst, len, split_point;

	if (m->input_size == 0)
		return;

	src = get_random_offset(m, 0);
	dst = get_random_offset(m, 1);

	if (src == dst)
		return;

	/* Random size of splice, capped to max. input size */
	len = rng_exp(&(m->rng), 1, m->input_size - src);
	len = umin(len, m->max_input_size - m->input_size);

	if (len == 0)
		return;

	make_space(m, dst, len);
	memset(m->input + dst, 0, len);
	split_point = umin(sat_sub_u64(dst, src), len);

	memcpy(m->input + dst, m->input + src, split_point);
	memcpy(m->input + dst + split_point, m->input + src + split_point + len, len - split_point);
}

/* Insert 1 or 2 random bytes, making space for them */
static void insert_rand(Mutator* m) {
	char ch[2];
	size_t offset, len, i;
	Rng* rng = &m->rng;

	if (m->printable) {
		ch[0] = rng_rand(rng, 0, 94) + 32;
		ch[1] = rng_rand(rng, 0, 94) + 32;
	} else {
		ch[0] = rng_rand(rng, 0, 255);
		ch[1] = rng_rand(rng, 0, 255);
	}

	/* Length is random (1 or 2), and capped to max. remaining space */
	offset = get_random_offset(m, 0);
	len = rng_rand(rng, 1, 2);
	len = umin(len, m->max_input_size - m->input_size);

	/* Make space for the new bytes and copy them */
	make_space(m, offset, len);
	for (i = offset; i < offset + len; ++i) {
		m->input[i] = ch[i % len];
		if (m->printable)
			m->input[i] = make_printable(m->input[i]);
	}
}

/* Insert 1 or 2 random bytes, without making space for them */
static void overwrite_rand(Mutator* m) {
	char ch[2];
	size_t offset, len;
	Rng* rng = &m->rng;

	if (m->input_size == 0)
		return;

	if (m->printable) {
		ch[0] = rng_rand(rng, 0, 94) + 32;
		ch[1] = rng_rand(rng, 0, 94) + 32;
	} else {
		ch[0] = rng_rand(rng, 0, 255);
		ch[1] = rng_rand(rng, 0, 255);
	}

	/* Length is 1 or 2, and capped to max. remaining space */
	offset = get_random_offset(m, 0);
	len = umin(m->input_size - offset, 2);
	len = rng_rand(rng, 1, len);

	memcpy(m->input + offset, ch, len);
}

/* Find a byte and repeat it multiple times by overwriting the data after */
static void byte_repeat_overwrite(Mutator* m) {
	size_t offset, amount;

	if (m->input_size == 0)
		return;

	offset = get_random_offset(m, 0);
	amount = rng_exp(&(m->rng), 1, m->input_size - offset) - 1;

	memset(m->input + offset + 1, m->input[offset], amount);
}

/* Find a byte and repeat it multiple times by making space */
static void byte_repeat_insert(Mutator* m) {
	size_t offset, amount;

	if (m->input_size == 0)
		return;

	offset = get_random_offset(m, 0);
	amount = rng_exp(&(m->rng), 1, m->input_size - offset) - 1;
	amount = umin(amount, m->max_input_size - m->input_size);

	make_space(m, offset + 1, amount);
	memset(m->input + offset + 1, m->input[offset], amount);
}

static void magic_overwrite(Mutator* m) {
	size_t offset, amount, i;
	const MagicValue* magic;

	if (m->input_size == 0)
		return;

	offset = get_random_offset(m, 0);
	magic = magic_values + rng_rand(&(m->rng), 0, ARR_SIZE(magic_values) - 1);
	amount = umin(m->input_size - offset, magic->len);

	memcpy(m->input + offset, magic->val, amount);
	if (m->printable) {
		for (i = offset; i < offset + amount; ++i) {
			m->input[i] = make_printable(m->input[i]);
		}
	}
}

static void magic_insert(Mutator* m) {
	size_t offset, amount, i;
	const MagicValue* magic;

	offset = get_random_offset(m, 1);
	magic = magic_values + rng_rand(&(m->rng), 0, ARR_SIZE(magic_values) - 1);
	amount = umin(m->max_input_size - m->input_size, magic->len);

	make_space(m, offset, amount);
	memcpy(m->input + offset, magic->val, amount);
	if (m->printable) {
		for (i = offset; i < offset + amount; ++i) {
			m->input[i] = make_printable(m->input[i]);
		}
	}
}

static void random_overwrite(Mutator* m) {
	size_t offset, amount, i;
	Rng* rng = &m->rng;

	if (m->input_size == 0)
		return;

	offset = get_random_offset(m, 0);
	amount = rng_exp(rng, 1, m->input_size - offset);

	if (m->printable) {
		for (i = offset; i < offset + amount; ++i)
			m->input[i] = rng_rand(rng, 0, 94) + 32;
	} else {
		for (i = offset; i < offset + amount; ++i)
			m->input[i] = rng_rand(rng, 0, 255);
	}
}

static void random_insert(Mutator* m) {
	size_t offset, amount, i;
	Rng* rng = &m->rng;

	offset = get_random_offset(m, 1);
	amount = rng_exp(rng, 0, m->input_size - offset);
	amount = umin(amount, m->max_input_size - m->input_size);

	make_space(m, offset, amount);
	if (m->printable) {
		for (i = offset; i < offset + amount; ++i)
			m->input[i] = rng_rand(rng, 0, 94) + 32;
	} else {
		for (i = offset; i < offset + amount; ++i)
			m->input[i] = rng_rand(rng, 0, 255);
	}
}

static const mut_function funcs[] = {
	shrink,
	expand,
	bit,
	inc_byte,
	dec_byte,
	neg_byte,
	add_sub,
	set,
	swap,
	copy,
	inter_splice,
	insert_rand,
	overwrite_rand,
	byte_repeat_overwrite,
	byte_repeat_insert,
	magic_overwrite,
	magic_insert,
	random_overwrite,
	random_insert,
};

void (*strategy_get_random(Rng* rng))(Mutator*) {
	u64 r;

	r = rng_rand(rng, 0, (sizeof(funcs)/sizeof(funcs[0]))-1);
	return funcs[r];
}
