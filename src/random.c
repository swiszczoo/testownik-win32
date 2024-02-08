#include <random.h>

#include <stdint.h>
#include <time.h>

// xoshiro256** implementation
// from https://en.wikipedia.org/wiki/Xorshift#xoshiro

typedef struct {
    uint64_t s[4];
} xoshiro256ss_state;

static uint64_t rol64(uint64_t x, int k)
{
    return (x << k) | (x >> (64 - k));
}

static uint64_t xoshiro256ss(xoshiro256ss_state* state)
{
    uint64_t* s = state->s;
    uint64_t const result = rol64(s[1] * 5, 7) * 9;
    uint64_t const t = s[1] << 17;

    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];

    s[2] ^= t;
    s[3] = rol64(s[3], 45);

    return result;
}

static xoshiro256ss_state prng_state;

void random_init()
{
    prng_state.s[0] = 0x4459b46ca4681396;
    prng_state.s[1] = 0x9ac20f3d4c7540ff;
    prng_state.s[2] = 0x7231328b4bd4b38d;
    prng_state.s[3] = 0xa8fa0dea251f80cc;

    time_t current_time = time(NULL);

    prng_state.s[0] ^= current_time;
    prng_state.s[1] &= current_time;
    prng_state.s[2] ^= current_time / 47;
    prng_state.s[3] += current_time;
}

uint64_t random_next_number()
{
    return xoshiro256ss(&prng_state);
}

uint64_t random_next_range(uint64_t range)
{
    return random_next_number() % range;
}

void random_shuffle_int_array(int* ptr, size_t count)
{
    while(--count) {
        int temp = ptr[count];
        int index = random_next_range(count + 1);
        ptr[count] = ptr[index];
        ptr[index] = temp;
    }
}
