#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <x86intrin.h>
#include <time.h>

#define PRIME_BITS 256     // size of primes
#define RUNS 100000          // number of MR trials on composite

//------------------------------------------------------------
// Miller-Rabin test (one iteration)
//------------------------------------------------------------
int millerTest(const mpz_t d, const mpz_t n, gmp_randstate_t state) {
    mpz_t a, x, n_minus_1, temp;
    mpz_inits(a, x, n_minus_1, temp, NULL);

    mpz_sub_ui(n_minus_1, n, 1);

    // Random base in [2, n-2]
    mpz_urandomm(a, state, n_minus_1);
    if (mpz_cmp_ui(a, 2) < 0) mpz_add_ui(a, a, 2);

    mpz_powm(x, a, d, n);
    if (mpz_cmp_ui(x, 1) == 0 || mpz_cmp(x, n_minus_1) == 0) {
        mpz_clears(a, x, n_minus_1, temp, NULL);
        return 1;
    }

    mpz_set(temp, d);
    while (mpz_cmp(temp, n_minus_1) != 0) {
        mpz_mul(x, x, x);
        mpz_mod(x, x, n);
        mpz_mul_ui(temp, temp, 2);

        if (mpz_cmp_ui(x, 1) == 0) {
            mpz_clears(a, x, n_minus_1, temp, NULL);
            return 0;
        }
        if (mpz_cmp(x, n_minus_1) == 0) {
            mpz_clears(a, x, n_minus_1, temp, NULL);
            return 1;
        }
    }

    mpz_clears(a, x, n_minus_1, temp, NULL);
    return 0;
}

//------------------------------------------------------------
// Miller-Rabin primality test (k iterations)
//------------------------------------------------------------
int isPrime(const mpz_t n, int k, gmp_randstate_t state) {
    if (mpz_cmp_ui(n, 1) <= 0) return 0;
    if (mpz_cmp_ui(n, 3) <= 0) return 1;

    mpz_t d, n_minus_1;
    mpz_inits(d, n_minus_1, NULL);

    mpz_sub_ui(n_minus_1, n, 1);
    mpz_set(d, n_minus_1);
    while (mpz_even_p(d))
        mpz_divexact_ui(d, d, 2);

    for (int i = 0; i < k; i++) {
        if (!millerTest(d, n, state)) {
            mpz_clears(d, n_minus_1, NULL);
            return 0;
        }
    }

    mpz_clears(d, n_minus_1, NULL);
    return 1;
}

//------------------------------------------------------------
// Generate a random probable prime of given bit size
//------------------------------------------------------------
void generate_prime(mpz_t prime, int bits, gmp_randstate_t state) {
    do {
        mpz_urandomb(prime, state, bits);
        mpz_setbit(prime, bits - 1); // force MSB
        mpz_setbit(prime, 0);        // force odd
    } while (!isPrime(prime, 1, state)); // 20 rounds for generation
}

//------------------------------------------------------------
// Main
//------------------------------------------------------------
int main() {
    gmp_randstate_t state;
    gmp_randinit_mt(state);
    gmp_randseed_ui(state, time(NULL));

    mpz_t p, q, n, d, n_minus_1;
    mpz_inits(p, q, n, d, n_minus_1, NULL);

    // Step 1: generate two 256-bit primes
    generate_prime(p, PRIME_BITS, state);
    generate_prime(q, PRIME_BITS, state);

    // Step 2: multiply to get composite
    mpz_mul(n, p, q);

    // Open file for output
    FILE *fp = fopen("test_output.txt", "w");
    if (!fp) {
        perror("File open failed");
        return 1;
    }

    // Print primes and composite (hex) to file
    gmp_fprintf(fp, "Prime p: %Zx\n\n", p);
    gmp_fprintf(fp, "Prime q: %Zx\n\n", q);
    gmp_fprintf(fp, "Composite n = p * q: %Zx\n\n", n);

    // Step 3: prepare d, n-1 for single-round MR
    mpz_sub_ui(n_minus_1, n, 1);
    mpz_set(d, n_minus_1);
    while (mpz_even_p(d))
        mpz_divexact_ui(d, d, 2);

    // Step 4: run many single-round MR tests
    int lies = 0;
    for (int i = 0; i < RUNS; i++) {
        if (millerTest(d, n, state)) {
            lies++;
        }
    }

    double liar_rate = (double)lies / RUNS;

    // Print results to file
    fprintf(fp, "Out of %d single-round MR trials on composite n:\n", RUNS);
    fprintf(fp, "  Lies (false prime reports): %d\n", lies);
    fprintf(fp, "  Experimental liar rate: %.6f\n", liar_rate);

    fclose(fp);

    // Cleanup
    mpz_clears(p, q, n, d, n_minus_1, NULL);
    gmp_randclear(state);

    return 0;
}
