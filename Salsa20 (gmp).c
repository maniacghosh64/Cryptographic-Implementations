#include <stdint.h>  // For fixed-width integer types like uint32_t
#include <stdio.h>   // For printf in the main function
#include <x86intrin.h>  // For __rdtsc() to measure CPU cycles (x86-specific)
#include <limits.h>

// Macro for left rotation of a 32-bit value 'a' by 'b' bits.
// This is a common operation in ARX-based ciphers like Salsa20.
// It shifts left and ORs with the bits that wrapped around.
#define ROTL(a, b) (((a) << (b)) | ((a) >> (32 - (b))))

// Macro for the quarter-round (QR) function.
// This is the basic building block of Salsa20.
// It takes four 32-bit words (a, b, c, d) and updates them using additions,
// XORs, and rotations. The specific rotation amounts (7, 9, 13, 18) were
// chosen for good diffusion and security.
#define QR(a, b, c, d) (       \
    b ^= ROTL(a + d, 7),      \
    c ^= ROTL(b + a, 9),      \
    d ^= ROTL(c + b, 13),     \
    a ^= ROTL(d + c, 18)      \
)

// Number of rounds for full Salsa20 (20 rounds for security).
// Salsa20/r uses r rounds; 20 is the standard for 256-bit security.
#define ROUNDS 20

// Core Salsa20 block function.
// Inputs: 'in' is a 16-element array of 32-bit words (512 bits total),
//         derived from the 256-bit key, 64-bit nonce, and 64-bit counter.
// Outputs: 'out' is the 512-bit keystream block.
// This function performs the Salsa20 hash: mixes the input over 20 rounds,
// then adds back the original input to make it a one-way function.
void salsa20_block(uint32_t out[16], const uint32_t in[16]) {
    int i;
    uint32_t x[16];  // Working copy of the input to mix in-place.

    // Copy input to working array.
    // This preserves the original 'in' for the final addition step.
    for (i = 0; i < 16; ++i) {
        x[i] = in[i];
    }

    // Perform 20 rounds of mixing.
    // Loops in steps of 2: each iteration does an "odd" (column) round
    // followed by an "even" (row) round. This double-round structure
    // alternates between updating columns and rows of the 4x4 matrix
    // (viewed as 32-bit words) for full diffusion.
    for (i = 0; i < ROUNDS; i += 2) {
        // Odd round: Apply QR to each column of the 4x4 matrix.
        // This mixes vertically.
        QR(x[ 0], x[ 4], x[ 8], x[12]);  // Column 0
        QR(x[ 5], x[ 9], x[13], x[ 1]);  // Column 1
        QR(x[10], x[14], x[ 2], x[ 6]);  // Column 2
        QR(x[15], x[ 3], x[ 7], x[11]);  // Column 3

        // Even round: Apply QR to each row of the 4x4 matrix.
        // This mixes horizontally.
        QR(x[ 0], x[ 1], x[ 2], x[ 3]);  // Row 0
        QR(x[ 5], x[ 6], x[ 7], x[ 4]);  // Row 1
        QR(x[10], x[11], x[ 8], x[ 9]);  // Row 2
        QR(x[15], x[12], x[13], x[14]);  // Row 3
    }

    // Final step: Add the mixed working array back to the original input.
    // This "feedforward" makes the function non-invertible and produces
    // the keystream. Without it, the rounds would be reversible.
    for (i = 0; i < 16; ++i) {
        out[i] = x[i] + in[i];
    }
}

// Example main function to demonstrate usage and measure performance.
// This sets up a dummy input (all zeros for simplicity), runs the block
// function 100,000 times, and measures CPU cycles using RDTSC (x86 only).
// Compile with: gcc -O3 -o salsa20 salsa20.c
// Run: ./salsa20
// Note: RDTSC measures CPU cycles but can vary due to caching, Turbo Boost, etc.
// For accurate results, run on a quiet system and average multiple trials.
int main() {
    uint32_t in[16] = {0};  // Dummy input (in practice, set from key/nonce/counter).
    uint32_t out[16];
    unsigned long long min_cycles = ULLONG_MAX;
    unsigned long long max_cycles = 0;
    unsigned long long total_cycles = 0;
    unsigned long long start, end;
    int i;

    for (i = 0; i < 100000; ++i) {
        start = __rdtsc();  // Read timestamp counter before the call.
        salsa20_block(out, in);
        end = __rdtsc();    // Read after.
        unsigned long long cycles = end - start;
        if (cycles < min_cycles) min_cycles = cycles;
        if (cycles > max_cycles) max_cycles = cycles;
        total_cycles += cycles;
    }

    double avg_cycles = (double)total_cycles / 100000;
    printf("Average cycles per run: %.2f\n", avg_cycles);
    printf("Minimum cycles: %llu\n", min_cycles);
    printf("Maximum cycles: %llu\n", max_cycles);

    return 0;
}