# AES-128 from-scratch (ECB mode, hex I/O)
# - Encrypt/decrypt a single 128-bit block
# - ECB mode over many blocks with PKCS#7 padding
# - Inputs/outputs are hex strings (case-insensitive, with or without "0x")

from typing import List

# Rijndael S-box
SBOX = [
    0x63,0x7C,0x77,0x7B,0xF2,0x6B,0x6F,0xC5,0x30,0x01,0x67,0x2B,0xFE,0xD7,0xAB,0x76,
    0xCA,0x82,0xC9,0x7D,0xFA,0x59,0x47,0xF0,0xAD,0xD4,0xA2,0xAF,0x9C,0xA4,0x72,0xC0,
    0xB7,0xFD,0x93,0x26,0x36,0x3F,0xF7,0xCC,0x34,0xA5,0xE5,0xF1,0x71,0xD8,0x31,0x15,
    0x04,0xC7,0x23,0xC3,0x18,0x96,0x05,0x9A,0x07,0x12,0x80,0xE2,0xEB,0x27,0xB2,0x75,
    0x09,0x83,0x2C,0x1A,0x1B,0x6E,0x5A,0xA0,0x52,0x3B,0xD6,0xB3,0x29,0xE3,0x2F,0x84,
    0x53,0xD1,0x00,0xED,0x20,0xFC,0xB1,0x5B,0x6A,0xCB,0xBE,0x39,0x4A,0x4C,0x58,0xCF,
    0xD0,0xEF,0xAA,0xFB,0x43,0x4D,0x33,0x85,0x45,0xF9,0x02,0x7F,0x50,0x3C,0x9F,0xA8,
    0x51,0xA3,0x40,0x8F,0x92,0x9D,0x38,0xF5,0xBC,0xB6,0xDA,0x21,0x10,0xFF,0xF3,0xD2,
    0xCD,0x0C,0x13,0xEC,0x5F,0x97,0x44,0x17,0xC4,0xA7,0x7E,0x3D,0x64,0x5D,0x19,0x73,
    0x60,0x81,0x4F,0xDC,0x22,0x2A,0x90,0x88,0x46,0xEE,0xB8,0x14,0xDE,0x5E,0x0B,0xDB,
    0xE0,0x32,0x3A,0x0A,0x49,0x06,0x24,0x5C,0xC2,0xD3,0xAC,0x62,0x91,0x95,0xE4,0x79,
    0xE7,0xC8,0x37,0x6D,0x8D,0xD5,0x4E,0xA9,0x6C,0x56,0xF4,0xEA,0x65,0x7A,0xAE,0x08,
    0xBA,0x78,0x25,0x2E,0x1C,0xA6,0xB4,0xC6,0xE8,0xDD,0x74,0x1F,0x4B,0xBD,0x8B,0x8A,
    0x70,0x3E,0xB5,0x66,0x48,0x03,0xF6,0x0E,0x61,0x35,0x57,0xB9,0x86,0xC1,0x1D,0x9E,
    0xE1,0xF8,0x98,0x11,0x69,0xD9,0x8E,0x94,0x9B,0x1E,0x87,0xE9,0xCE,0x55,0x28,0xDF,
    0x8C,0xA1,0x89,0x0D,0xBF,0xE6,0x42,0x68,0x41,0x99,0x2D,0x0F,0xB0,0x54,0xBB,0x16
]

# Inverse S-box
INV_SBOX = [0]*256
for i, v in enumerate(SBOX):
    INV_SBOX[v] = i

# Round constants (Rcon) for AES-128 (10 rounds -> need 10 constants)
RCON = [0x00,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1B,0x36]

# --------- finite field helpers ---------
def xtime(a: int) -> int:
    a <<= 1
    if a & 0x100:
        a ^= 0x11B
    return a & 0xFF

def gf_mul(a: int, b: int) -> int:
    # multiply in GF(2^8) with AES polynomial x^8+x^4+x^3+x+1 (0x11B)
    res = 0
    for _ in range(8):
        if b & 1:
            res ^= a
        carry = a & 0x80
        a = (a << 1) & 0xFF
        if carry:
            a ^= 0x1B
        b >>= 1
    return res

# --------- core transformations ---------
def sub_bytes(state: List[int]) -> None:
    for i in range(16):
        state[i] = SBOX[state[i]]

def inv_sub_bytes(state: List[int]) -> None:
    for i in range(16):
        state[i] = INV_SBOX[state[i]]

def shift_rows(state: List[int]) -> None:
    # state is in column-major: indices: [0..3]=col0, [4..7]=col1, etc.
    # rows are offsets modulo 4 within each column stride
    rows = [state[i::4] for i in range(4)]
    rows[1] = rows[1][1:] + rows[1][:1]
    rows[2] = rows[2][2:] + rows[2][:2]
    rows[3] = rows[3][3:] + rows[3][:3]
    # write back
    for r in range(4):
        state[r::4] = rows[r]

def inv_shift_rows(state: List[int]) -> None:
    rows = [state[i::4] for i in range(4)]
    rows[1] = rows[1][-1:] + rows[1][:-1]
    rows[2] = rows[2][-2:] + rows[2][:-2]
    rows[3] = rows[3][-3:] + rows[3][:-3]
    for r in range(4):
        state[r::4] = rows[r]

def mix_columns(state: List[int]) -> None:
    for c in range(4):
        i = 4*c
        a0, a1, a2, a3 = state[i:i+4]
        state[i+0] = gf_mul(0x02,a0) ^ gf_mul(0x03,a1) ^ a2 ^ a3
        state[i+1] = a0 ^ gf_mul(0x02,a1) ^ gf_mul(0x03,a2) ^ a3
        state[i+2] = a0 ^ a1 ^ gf_mul(0x02,a2) ^ gf_mul(0x03,a3)
        state[i+3] = gf_mul(0x03,a0) ^ a1 ^ a2 ^ gf_mul(0x02,a3)

def inv_mix_columns(state: List[int]) -> None:
    for c in range(4):
        i = 4*c
        a0, a1, a2, a3 = state[i:i+4]
        state[i+0] = gf_mul(0x0e,a0) ^ gf_mul(0x0b,a1) ^ gf_mul(0x0d,a2) ^ gf_mul(0x09,a3)
        state[i+1] = gf_mul(0x09,a0) ^ gf_mul(0x0e,a1) ^ gf_mul(0x0b,a2) ^ gf_mul(0x0d,a3)
        state[i+2] = gf_mul(0x0d,a0) ^ gf_mul(0x09,a1) ^ gf_mul(0x0e,a2) ^ gf_mul(0x0b,a3)
        state[i+3] = gf_mul(0x0b,a0) ^ gf_mul(0x0d,a1) ^ gf_mul(0x09,a2) ^ gf_mul(0x0e,a3)

def add_round_key(state: List[int], round_key: List[int]) -> None:
    for i in range(16):
        state[i] ^= round_key[i]

# --------- key expansion (AES-128: 16-byte key -> 11 round keys) ---------
def rot_word(word: List[int]) -> List[int]:
    return word[1:] + word[:1]

def sub_word(word: List[int]) -> List[int]:
    return [SBOX[b] for b in word]

def key_expansion(key: bytes) -> List[List[int]]:
    assert len(key) == 16
    Nk = 4            # 32-bit words in key
    Nb = 4            # columns in state
    Nr = 10           # rounds

    w: List[List[int]] = []
    # first Nk words are the key itself
    for i in range(Nk):
        w.append([key[4*i + j] for j in range(4)])

    for i in range(Nk, Nb*(Nr+1)):
        temp = w[i-1].copy()
        if i % Nk == 0:
            temp = sub_word(rot_word(temp))
            temp[0] ^= RCON[i//Nk]
        # w[i] = w[i-Nk] XOR temp
        w.append([ (w[i-Nk][j] ^ temp[j]) & 0xFF for j in range(4) ])

    # pack round keys as 16-byte (column-major)
    round_keys: List[List[int]] = []
    for r in range(Nr+1):
        rk = []
        for c in range(4):
            rk.extend(w[4*r + c])
        round_keys.append(rk)
    return round_keys  # 11 round keys

# --------- block encryption / decryption ---------
def encrypt_block(plain16: bytes, key: bytes) -> bytes:
    assert len(plain16) == 16 and len(key) == 16
    round_keys = key_expansion(key)
    state = list(plain16)

    add_round_key(state, round_keys[0])
    for rnd in range(1, 10):
        sub_bytes(state)
        shift_rows(state)
        mix_columns(state)
        add_round_key(state, round_keys[rnd])
    # final round
    sub_bytes(state)
    shift_rows(state)
    add_round_key(state, round_keys[10])

    return bytes(state)

def decrypt_block(cipher16: bytes, key: bytes) -> bytes:
    assert len(cipher16) == 16 and len(key) == 16
    round_keys = key_expansion(key)
    state = list(cipher16)

    add_round_key(state, round_keys[10])
    inv_shift_rows(state)
    inv_sub_bytes(state)
    for rnd in range(9, 0, -1):
        add_round_key(state, round_keys[rnd])
        inv_mix_columns(state)
        inv_shift_rows(state)
        inv_sub_bytes(state)
    add_round_key(state, round_keys[0])

    return bytes(state)

# --------- ECB mode with PKCS#7 padding (bytes) ---------
def pkcs7_pad(data: bytes, block_size: int = 16) -> bytes:
    padlen = block_size - (len(data) % block_size or block_size)
    if padlen == 0:  # never happens due to expression above, but kept for clarity
        padlen = block_size
    return data + bytes([padlen])*padlen

def pkcs7_unpad(data: bytes, block_size: int = 16) -> bytes:
    if len(data) == 0 or len(data) % block_size != 0:
        raise ValueError("Invalid padded data length")
    padlen = data[-1]
    if padlen == 0 or padlen > block_size:
        raise ValueError("Invalid PKCS#7 padding")
    if data[-padlen:] != bytes([padlen])*padlen:
        raise ValueError("Invalid PKCS#7 padding bytes")
    return data[:-padlen]

def ecb_encrypt(plain: bytes, key: bytes) -> bytes:
    plain_padded = pkcs7_pad(plain, 16)
    out = bytearray()
    for i in range(0, len(plain_padded), 16):
        out.extend(encrypt_block(plain_padded[i:i+16], key))
    return bytes(out)

def ecb_decrypt(cipher: bytes, key: bytes) -> bytes:
    if len(cipher) % 16 != 0:
        raise ValueError("Ciphertext length must be multiple of 16 bytes")
    out = bytearray()
    for i in range(0, len(cipher), 16):
        out.extend(decrypt_block(cipher[i:i+16], key))
    return pkcs7_unpad(bytes(out), 16)

# --------- Hex I/O helpers ---------
def hex_to_bytes(h: str) -> bytes:
    h = h.lower().strip()
    if h.startswith("0x"):
        h = h[2:]
    if len(h) % 2 != 0:
        h = "0" + h
    return bytes.fromhex(h)

def bytes_to_hex(b: bytes) -> str:
    return b.hex()

# --------- Convenience wrappers for hex inputs/outputs ---------
def aes128_encrypt_block_hex(plaintext_hex: str, key_hex: str) -> str:
    p = hex_to_bytes(plaintext_hex)
    k = hex_to_bytes(key_hex)
    if len(p) != 16 or len(k) != 16:
        raise ValueError("AES-128 block and key must be 16 bytes (32 hex chars)")
    c = encrypt_block(p, k)
    return bytes_to_hex(c)

def aes128_decrypt_block_hex(ciphertext_hex: str, key_hex: str) -> str:
    c = hex_to_bytes(ciphertext_hex)
    k = hex_to_bytes(key_hex)
    if len(c) != 16 or len(k) != 16:
        raise ValueError("AES-128 block and key must be 16 bytes (32 hex chars)")
    p = decrypt_block(c, k)
    return bytes_to_hex(p)

def aes128_ecb_encrypt_hex(plaintext_hex: str, key_hex: str) -> str:
    p = hex_to_bytes(plaintext_hex)
    k = hex_to_bytes(key_hex)
    if len(k) != 16:
        raise ValueError("AES-128 key must be 16 bytes (32 hex chars)")
    c = ecb_encrypt(p, k)
    return bytes_to_hex(c)

def aes128_ecb_decrypt_hex(ciphertext_hex: str, key_hex: str) -> str:
    c = hex_to_bytes(ciphertext_hex)
    k = hex_to_bytes(key_hex)
    if len(k) != 16:
        raise ValueError("AES-128 key must be 16 bytes (32 hex chars)")
    p = ecb_decrypt(c, k)
    return bytes_to_hex(p)

# --------- Quick self-test with NIST SP 800-38A vector ---------
if __name__ == "__main__":
    # key = "000102030405060708090a0b0c0d0e0f"
    # pt  = "00112233445566778899aabbccddeeff"
    # ct_expected = "69c4e0d86a7b0430d8cdb78070b4c55a"

    key = "2b7e151628aed2a6abf7158809cf4f3c"
    pt = "3243f6a8885a308d313198a2e0370734"
    ct_expected = "3925841d02dc09fbdc118597196a0b32"

    ct = aes128_encrypt_block_hex(pt, key)
    pt2 = aes128_decrypt_block_hex(ct, key)

    print("Encrypt OK:", ct == ct_expected, ct)
    print("Decrypt OK:", pt2 == pt, pt2)

    # ECB example (multi-block, padding)
    msg_hex = "48656c6c6f20576f726c6421"  # "Hello World!"
    ecb_ct = aes128_ecb_encrypt_hex(msg_hex, key)
    ecb_pt = aes128_ecb_decrypt_hex(ecb_ct, key)
    print("ECB round-trip OK:", ecb_pt == msg_hex, ecb_pt)
