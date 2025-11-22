#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <limits.h>
#include <x86intrin.h>  // For __rdtsc() to measure CPU cycles (x86-specific)

#define ROTL32(v, n) ((v << n) | (v >> (32 - n)))

#define QUARTERROUND(a, b, c, d) \
    a += b; d ^= a; d = ROTL32(d, 16); \
    c += d; b ^= c; b = ROTL32(b, 12); \
    a += b; d ^= a; d = ROTL32(d, 8); \
    c += d; b ^= c; b = ROTL32(b, 7);
    
// A helper function for us to visualize our states
void print_state(const uint32_t state[16]) {
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            printf("%08x ", state[4 * row + col]);
        }
        printf("\n");
    }
    printf("\n");
}

void chacha20_block(uint32_t output[16], const uint32_t input[16]) {
    int i;
    uint32_t state[16];
    for (i = 0; i < 16; i++) state[i] = input[i];
    printf("Initial state:\n");
    print_state(state);

    for (i = 0; i < 10; i++) {
        QUARTERROUND(state[0], state[4], state[8], state[12]);
        QUARTERROUND(state[1], state[5], state[9], state[13]);
        QUARTERROUND(state[2], state[6], state[10], state[14]);
        QUARTERROUND(state[3], state[7], state[11], state[15]);
        QUARTERROUND(state[0], state[5], state[10], state[15]);
        QUARTERROUND(state[1], state[6], state[11], state[12]);
        QUARTERROUND(state[2], state[7], state[8], state[13]);
        QUARTERROUND(state[3], state[4], state[9], state[14]);
    }
    printf("State after 20 rounds:\n");
    print_state(state);

    for (i = 0; i < 16; i++) output[i] = state[i] + input[i];
    printf("Output after adding state with input:\n");
    print_state(output);
}

/*
state[16]: A 16-word (512-bit) array to hold the initialized state.
key[32]: A 32-byte (256-bit) secret key.
nonce[12]: A 12-byte (96-bit) nonce.
counter: A 32-bit counter, typically starting at 0 or 1.
*/
void initialize_state(uint32_t state[16], const uint8_t key[32], const uint8_t nonce[12], uint32_t counter) {
    state[0] = 0x61707865; // "expa", 65,78,70,61 respective ASCII vaues
    state[1] = 0x3320646e; // "nd 3"
    state[2] = 0x79622d32; // "2-by"
    state[3] = 0x6b206574; // "te k"
    // ASCII representation of the string "expand 32-byte k"

    // Next, each 32-bit word is 
    for (int i = 0; i < 8; i++) {
        state[4 + i] = ((uint32_t)key[4 * i]) | ((uint32_t)key[4 * i + 1] << 8) |
                       ((uint32_t)key[4 * i + 2] << 16) | ((uint32_t)key[4 * i + 3] << 24);
    }

    state[12] = counter;

    for (int i = 0; i < 3; i++) {
        state[13 + i] = ((uint32_t)nonce[4 * i]) | ((uint32_t)nonce[4 * i + 1] << 8) |
                        ((uint32_t)nonce[4 * i + 2] << 16) | ((uint32_t)nonce[4 * i + 3] << 24);
    }
}

void chacha20_encrypt(uint8_t *plaintext, uint8_t *ciphertext, size_t len, const uint8_t key[32], const uint8_t nonce[12], uint32_t counter) {
    uint32_t state[16];
    uint32_t keystream[16];
    size_t offset = 0;

    while (len > 0) {
        initialize_state(state, key, nonce, counter);
        chacha20_block(keystream, state);

        size_t block_size = len < 64 ? len : 64;
        for (size_t i = 0; i < block_size; i++) {
            ciphertext[offset + i] = plaintext[offset + i] ^ ((uint8_t *)keystream)[i];
        }

        len -= block_size;
        offset += block_size;
        counter++;
    }
}

int main() {
    uint8_t key[32] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };

    uint8_t nonce[12] = {
    0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x4a,
    0x00, 0x00, 0x00, 0x00
    };
    uint8_t plaintext[114] = {
    0x4c, 0x61, 0x64, 0x69, 0x65, 0x73, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x47, 0x65, 0x6e, 0x74, 0x6c,
    0x65, 0x6d, 0x65, 0x6e, 0x20, 0x6f, 0x66, 0x20, 0x74, 0x68, 0x65, 0x20, 0x63, 0x6c, 0x61, 0x73,
    0x73, 0x20, 0x6f, 0x66, 0x20, 0x27, 0x39, 0x39, 0x3a, 0x20, 0x49, 0x66, 0x20, 0x49, 0x20, 0x63,
    0x6f, 0x75, 0x6c, 0x64, 0x20, 0x6f, 0x66, 0x66, 0x65, 0x72, 0x20, 0x79, 0x6f, 0x75, 0x20, 0x6f,
    0x6e, 0x6c, 0x79, 0x20, 0x6f, 0x6e, 0x65, 0x20, 0x74, 0x69, 0x70, 0x20, 0x66, 0x6f, 0x72, 0x20,
    0x74, 0x68, 0x65, 0x20, 0x66, 0x75, 0x74, 0x75, 0x72, 0x65, 0x2c, 0x20, 0x73, 0x75, 0x6e, 0x73,
    0x63, 0x72, 0x65, 0x65, 0x6e, 0x20, 0x77, 0x6f, 0x75, 0x6c, 0x64, 0x20, 0x62, 0x65, 0x20, 0x69,
    0x74, 0x2e
    };

    size_t len = sizeof plaintext;
    uint8_t ciphertext[sizeof plaintext] = {0};
    uint8_t decrypted [sizeof plaintext] = {0};

    unsigned long long min_cycles = ULLONG_MAX;
    unsigned long long max_cycles = 0;
    unsigned long long total_cycles = 0;
    unsigned long long start, end;
    int trials = 1;
    int i;

    for (i = 0; i < trials; ++i) {
        start = __rdtsc();  // Read timestamp counter before the calls
        chacha20_encrypt(plaintext, ciphertext, len, key, nonce, 1);
        chacha20_encrypt(ciphertext, decrypted, len, key, nonce, 1);
        end = __rdtsc();    // Read after
        unsigned long long cycles = end - start;
        if (cycles < min_cycles) min_cycles = cycles;
        if (cycles > max_cycles) max_cycles = cycles;
        total_cycles += cycles;
    }

    double avg_cycles = (double)total_cycles / trials;

    // Perform one final encryption/decryption for printing (outside loop)
    chacha20_encrypt(plaintext, ciphertext, len, key, nonce, 1);
    chacha20_encrypt(ciphertext, decrypted, len, key, nonce, 1);

    printf("Plaintext:  %s\n", plaintext);

    printf("Ciphertext (hex): ");
    for (size_t i = 0; i < len; i++) {
        printf("%02x ", ciphertext[i]);
    }
    printf("\n");

    printf("Decrypted:  %s\n", decrypted);

    // Verify decryption
    if (strncmp((char*)plaintext, (char*)decrypted, len) == 0) {
        printf("Decryption successful: plaintext matches decrypted text.\n");
    } else {
        printf("Decryption failed: plaintext does not match decrypted text.\n");
    }

    printf("Average clock cycles: %.2f\n", avg_cycles);
    printf("Minimum clock cycles: %llu\n", min_cycles);
    printf("Maximum clock cycles: %llu\n", max_cycles);

    return 0;
}