#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <x86intrin.h> // for __rdtsc and __rdtscp
#include <math.h>

#define PRIME_BITS 512
#define MSG_BITS 1023
#define TRIALS 1000000

// rdtsc serialization: begin uses cpuid + rdtsc, end uses rdtscp + cpuid
static inline uint64_t rdtsc_serialized_begin(void) {
    unsigned int eax, ebx, ecx, edx;
    asm volatile("cpuid"
                 : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                 : "a"(0)
                 : "memory");
    return __rdtsc();
}

static inline uint64_t rdtsc_serialized_end(void) {
    unsigned int aux;
    uint64_t t = __rdtscp(&aux);
    asm volatile("cpuid" : : : "rax", "rbx", "rcx", "rdx", "memory");
    return t;
}

#define INIT_STATS(minv, maxv, totalv) \
    minv = UINT64_MAX; maxv = 0; totalv = 0

#define UPDATE_STATS(val, minv, maxv, totalv) \
    if (val < minv) minv = val; \
    if (val > maxv) maxv = val; \
    totalv += (uint64_t)(val)

// convert 128-bit unsigned to long double safely
static long double u128_to_ld(__uint128_t v) {
    unsigned long long low = (unsigned long long)v;
    unsigned long long high = (unsigned long long)(v >> 64);
    return (long double)high * powl(2.0L, 64) + (long double)low;
}

int main(void) {
    gmp_randstate_t state;

    // Better entropy seed: try /dev/urandom first
    unsigned long seed = 0;
    FILE *ur = fopen("/dev/urandom", "rb");
    if (ur) {
        if (fread(&seed, sizeof(seed), 1, ur) != 1) {
            seed = (unsigned long)time(NULL) ^ (unsigned long)getpid();
        }
        fclose(ur);
    } else {
        seed = (unsigned long)time(NULL) ^ (unsigned long)getpid();
    }

    gmp_randinit_mt(state);
    gmp_randseed_ui(state, seed);

    // stats variables
    uint64_t p_min, p_max, q_min, q_max, n_min, n_max, phi_min, phi_max;
    __uint128_t p_total, q_total, n_total, phi_total;

    INIT_STATS(p_min, p_max, p_total);
    INIT_STATS(q_min, q_max, q_total);
    INIT_STATS(n_min, n_max, n_total);
    INIT_STATS(phi_min, phi_max, phi_total);

    int progress_step = TRIALS / 10;
    printf("Progress: [          ] 0%%");
    fflush(stdout);

    for (int t = 0; t < TRIALS; t++) {
        mpz_t p, q, n, phi, e, d, tmp, p1, q1;
        mpz_inits(p, q, n, phi, e, d, tmp, p1, q1, NULL);

        // --- generate p ---
        uint64_t start = 0, end = 0;
        do {
            start = rdtsc_serialized_begin();
            mpz_urandomb(tmp, state, PRIME_BITS);
            mpz_setbit(tmp, PRIME_BITS - 1); // guarantee bit-length
            mpz_setbit(tmp, 0); // Setting the LSB to 1
            mpz_nextprime(p, tmp);
            end = rdtsc_serialized_end();

            // check if (p-1) divisible by e (65537); if so, reject and loop
            mpz_sub_ui(tmp, p, 1);
        } while (mpz_divisible_ui_p(tmp, 65537));
        UPDATE_STATS(end - start, p_min, p_max, p_total);

        // --- generate q ---
        uint64_t q_cycles = 0;
        do {
            start = rdtsc_serialized_begin();
            mpz_urandomb(tmp, state, PRIME_BITS);
            mpz_setbit(tmp, PRIME_BITS - 1); // guarantee bit-length
            mpz_setbit(tmp, 0); // Setting the LSB to 1
            mpz_nextprime(q, tmp);
            end = rdtsc_serialized_end();

            mpz_sub_ui(tmp, q, 1);
            q_cycles = end - start;
        } while (mpz_cmp(p, q) == 0 || mpz_divisible_ui_p(tmp, 65537));
        UPDATE_STATS(q_cycles, q_min, q_max, q_total);

        // --- compute n ---
        start = rdtsc_serialized_begin();
        mpz_mul(n, p, q);
        end = rdtsc_serialized_end();
        UPDATE_STATS(end - start, n_min, n_max, n_total);

        // --- compute phi ---
        start = rdtsc_serialized_begin();
        mpz_sub_ui(p1, p, 1);
        mpz_sub_ui(q1, q, 1);
        mpz_mul(phi, p1, q1);
        end = rdtsc_serialized_end();
        UPDATE_STATS(end - start, phi_min, phi_max, phi_total);

        mpz_set_ui(e, 65537);
        if (mpz_invert(d, e, phi) == 0) {
            fprintf(stderr, "mpz_invert failed on trial %d -- skipping\n", t);
            mpz_clears(p, q, n, phi, e, d, tmp, p1, q1, NULL);
            continue;
        }

        mpz_clears(p, q, n, phi, e, d, tmp, p1, q1, NULL);

        // --- progress update ---
        if ((t + 1) % progress_step == 0) {
            int percent = ((t + 1) * 100) / TRIALS;
            int bars = percent / 10;
            printf("\rProgress: [");
            for (int i = 0; i < bars; i++) printf("#");
            for (int i = bars; i < 10; i++) printf(" ");
            printf("] %d%%", percent);
            fflush(stdout);
        }
    }
    printf("\rProgress: [##########] 100%% Finished!\n");

    // print averages (convert 128-bit totals to long double first)
    long double p_avg_ld   = u128_to_ld(p_total) / (long double)TRIALS;
    long double q_avg_ld   = u128_to_ld(q_total) / (long double)TRIALS;
    long double n_avg_ld   = u128_to_ld(n_total) / (long double)TRIALS;
    long double phi_avg_ld = u128_to_ld(phi_total) / (long double)TRIALS;

    printf("Over %d trials (PRIME_BITS=%d):\n\n", TRIALS, PRIME_BITS);
    printf("Step-1:\n");
    printf("Prime p generation: min=%llu, max=%llu, avg=%.2Lf\n",
           (unsigned long long)p_min, (unsigned long long)p_max, p_avg_ld);
    printf("Prime q generation: min=%llu, max=%llu, avg=%.2Lf\n\n",
           (unsigned long long)q_min, (unsigned long long)q_max, q_avg_ld);
    printf("Step-2:\n");
    printf("n computation:      min=%llu, max=%llu, avg=%.2Lf\n",
           (unsigned long long)n_min, (unsigned long long)n_max, n_avg_ld);
    printf("phi computation:    min=%llu, max=%llu, avg=%.2Lf\n\n",
           (unsigned long long)phi_min, (unsigned long long)phi_max, phi_avg_ld);

    // --- Now do encryption/decryption timing on one random example ---
    mpz_t p, q, n, phi, e, d, tmp, p1, q1, msg, encrypted, rec;
    mpz_inits(p, q, n, phi, e, d, tmp, p1, q1, msg, encrypted, rec, NULL);

    do {
        mpz_urandomb(tmp, state, PRIME_BITS);
        mpz_setbit(tmp, PRIME_BITS - 1);
        mpz_setbit(tmp, 0);
        mpz_nextprime(p, tmp);
        mpz_sub_ui(tmp, p, 1);
    } while (mpz_divisible_ui_p(tmp, 65537));

    do {
        mpz_urandomb(tmp, state, PRIME_BITS);
        mpz_setbit(tmp, PRIME_BITS - 1);
        mpz_setbit(tmp, 0);
        mpz_nextprime(q, tmp);
        mpz_sub_ui(tmp, q, 1);
    } while (mpz_cmp(p, q) == 0 || mpz_divisible_ui_p(tmp, 65537));

    mpz_mul(n, p, q);
    mpz_sub_ui(p1, p, 1);
    mpz_sub_ui(q1, q, 1);
    mpz_mul(phi, p1, q1);
    mpz_set_ui(e, 65537);

    uint64_t start = rdtsc_serialized_begin();
    if (mpz_invert(d, e, phi) == 0) {
        fprintf(stderr, "mpz_invert failed: e has no inverse mod phi\n");
        return 1;
    }
    uint64_t end   = rdtsc_serialized_end();
    printf("Step-3:\n");
    printf("Clock cycles for computing d: %llu\n\n",
           (unsigned long long)(end - start));

    do {
        mpz_urandomb(msg, state, MSG_BITS);
        mpz_setbit(msg, MSG_BITS - 1);
    } while (mpz_cmp(msg, n) >= 0);

    printf("Step-4:\n");
    printf("Starting encryption...\n"); fflush(stdout);
    start = rdtsc_serialized_begin();
    mpz_powm(encrypted, msg, e, n);
    end   = rdtsc_serialized_end();
    printf("Encryption done. Clock cycles for encryption:  %llu\n",
           (unsigned long long)(end - start));

    mpz_t dP, dQ, qInv, m1, m2, h;
    mpz_inits(dP, dQ, qInv, m1, m2, h, NULL);

    mpz_sub_ui(tmp, p, 1);
    mpz_mod(dP, d, tmp);
    mpz_sub_ui(tmp, q, 1);
    mpz_mod(dQ, d, tmp);
    if (mpz_invert(qInv, q, p) == 0) {
        fprintf(stderr, "mpz_invert failed computing qInv\n");
        return 1;
    }

    printf("Starting CRT decryption...\n"); fflush(stdout);
    start = rdtsc_serialized_begin();
    mpz_powm(m1, encrypted, dP, p);
    mpz_powm(m2, encrypted, dQ, q);

    mpz_sub(h, m1, m2);
    mpz_mod(h, h, p);

    mpz_mul(h, h, qInv);
    mpz_mod(h, h, p);

    mpz_mul(tmp, h, q);
    mpz_add(rec, tmp, m2);
    end = rdtsc_serialized_end();
    printf("CRT decryption done. Clock cycles for CRT decryption: %llu\n",
           (unsigned long long)(end - start));

    if (mpz_cmp(msg, rec) != 0) {
        fprintf(stderr, "CRT decryption did NOT recover the original message!\n");
    } else {
        printf("CRT decryption verified. Decrypted message matches with original message!\n");
    }

    gmp_printf("\nExample run:\n");
    gmp_printf("Message (hex):     %Zx\n\n", msg);
    gmp_printf("Encrypted (hex):   %Zx\n\n", encrypted);
    gmp_printf("Decrypted (CRT):   %Zx\n", rec);

    mpz_clears(p, q, n, phi, e, d, tmp, p1, q1, msg, encrypted, rec, NULL);
    mpz_clears(dP, dQ, qInv, m1, m2, h, NULL);
    gmp_randclear(state);
    return 0;
}

