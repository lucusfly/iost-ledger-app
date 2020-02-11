/*******************************************************************************
*   (c) 2016 Ledger
*   (c) 2018 ZondaX GmbH
*   (c) 2019 IoTeX Foundation
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/
#include "crypto.h"

#include "apdu_codes.h"
#include "zxmacros.h"
#include "bech32.h"

uint8_t bip32_depth;
uint32_t bip32_path[10];
sigtype_t current_sigtype;

uint8_t bech32_hrp_len;
char bech32_hrp[MAX_BECH32_HRP_LEN + 1];

cx_sha3_t sha3 ;

void keys_secp256k1(cx_ecfp_public_key_t *publicKey,
                    cx_ecfp_private_key_t *privateKey,
                    const uint8_t privateKeyData[32]) {
    cx_ecfp_init_private_key(CX_CURVE_256K1, privateKeyData, 32, privateKey);
    cx_ecfp_init_public_key(CX_CURVE_256K1, NULL, 0, publicKey);
    cx_ecfp_generate_pair(CX_CURVE_256K1, publicKey, privateKey, 1);
}

int sign_secp256k1(const uint8_t *message,
                   unsigned int message_length,
                   uint8_t *signature,
                   unsigned int signature_capacity,
                   unsigned int *signature_length,
                   cx_ecfp_private_key_t *privateKey) {
    uint8_t message_digest[HASH_KECCAK_BYTES];

    cx_keccak_init(&sha3, HASH_KECCAK_BYTES * 8);
    cx_hash((cx_hash_t *)&sha3, CX_LAST, message, message_length, message_digest,HASH_KECCAK_BYTES);

    cx_ecfp_public_key_t publicKey;
    cx_ecdsa_init_public_key(CX_CURVE_256K1, NULL, 0, &publicKey);
    cx_ecfp_generate_pair(CX_CURVE_256K1, &publicKey, privateKey, 1);

    unsigned int info = 0;
    *signature_length = cx_ecdsa_sign(
            privateKey,
            CX_RND_RFC6979 | CX_LAST,
            CX_SHA256,
            message_digest,
            CX_SHA256_SIZE,
            signature,
            signature_capacity,
            &info);

    os_memset(&privateKey, 0, sizeof(privateKey));
#ifdef TESTING_ENABLED
    int ret = cx_ecdsa_verify(
            &publicKey,
            CX_LAST,
            CX_SHA256,
            message_digest,
            CX_SHA256_SIZE,
            signature,
            *signature_length);
    if ( ret != 1) return ret;
#endif
    //Re encoding ECDSA signature from ASN.1 TLV:  30(1) L(1) 02(1) Lr(1) r(Lr) 02(1) Ls(1) s(Ls)
    //to:  r(32)s(32)v(1)
    uint8_t rLength, sLength, rOffset, sOffset;
    uint8_t v = 0;
    rLength = signature[3];
    sLength = signature[4 + rLength + 1];
    rOffset = (rLength == 33 ? 1 : 0);
    sOffset = (sLength == 33 ? 1 : 0);

    if (info & CX_ECCINFO_PARITY_ODD) {
      v++;
    }
    if (info & CX_ECCINFO_xGTn) {
      v += 2;
    }

    os_memmove(signature,signature + 4 + rOffset, 32);
    os_memmove(signature + 32, signature + 4 + rLength + 2 + sOffset, 32);
    signature[64] = v;
    *signature_length = 65;

    return 1;
}

void getPubKey(cx_ecfp_public_key_t *publicKey) {
    cx_ecfp_private_key_t privateKey;
    uint8_t privateKeyData[32];

    // Generate keys
    os_perso_derive_node_bip32(CX_CURVE_256K1,
                               bip32_path,
                               bip32_depth,
                               privateKeyData, NULL);

    keys_secp256k1(publicKey, &privateKey, privateKeyData);
    memset(privateKeyData, 0, sizeof(privateKeyData));
    memset(&privateKey, 0, sizeof(privateKey));
}

void ripemd160_32(uint8_t *out, uint8_t *in) {
    cx_ripemd160_t rip160;
    cx_ripemd160_init(&rip160);
    cx_hash(&rip160.header, CX_LAST, in, CX_SHA256_SIZE, out, CX_RIPEMD160_SIZE);
}

void get_pk_compressed(uint8_t *pkc) {
    cx_ecfp_public_key_t publicKey;
    // Modify the last part of the path
    getPubKey(&publicKey);
    // "Compress" public key in place
    publicKey.W[0] = publicKey.W[64] & 1 ? 0x03 : 0x02;
    memcpy(pkc, publicKey.W, PK_COMPRESSED_LEN);
}

void get_bech32_addr(char *bech32_addr) {
    uint8_t key_digest[HASH_KECCAK_BYTES];
    cx_ecfp_public_key_t publicKey;

    getPubKey(&publicKey);
    cx_keccak_init(&sha3, HASH_KECCAK_BYTES * 8);
    cx_hash((cx_hash_t *)&sha3, CX_LAST, &(publicKey.W[1]), publicKey.W_len-1, key_digest,HASH_KECCAK_BYTES);

    // Encode the last 20 bytes([12:32] bytes in the 32 bytes hash value)as address
    bech32EncodeFromBytes(bech32_addr, bech32_hrp, &(key_digest[12]), 20);
}
