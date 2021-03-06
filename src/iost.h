#ifndef LEDGER_APP_IOST_IOST_H
#define LEDGER_APP_IOST_IOST_H

#include <stdint.h>
#include <stddef.h>

#define IOST_NET_TYPE 44
#define IOST_COIN_ID 291
//#define PUBLIC_KEY_SIZE 32
#define ED25519_KEY_SIZE 32

//struct _Transaction;

// Forward declare to avoid including os.h in a header file
struct cx_ecfp_256_private_key_s;
struct cx_ecfp_256_public_key_s;

//void iost_transaction_add_action(struct _Transaction *tx, const char *contract, const char *abi, const void *data);

extern uint16_t iost_derive_keypair(
    const uint32_t * const bip_32_path,
    const uint16_t bip_32_length,
    /* out */ struct cx_ecfp_256_private_key_s* secret_key,
    /* out */ struct cx_ecfp_256_public_key_s* public_key
);

extern uint16_t iost_sign(
    const uint32_t * const bip_32_path,
    const int bip_32_length,
    const uint8_t* tx,
    uint8_t tx_len,
    uint8_t* signature
);

extern uint16_t iost_hash_bytes(
    const uint8_t * const in_bytes,
    const uint16_t in_length,
    uint8_t* hash_out
);

extern void iost_extract_bytes_from_public_key(
    const struct cx_ecfp_256_public_key_s* public_key,
    uint8_t* bytes_out,
    uint16_t* key_length
);


//extern const char* iost_format_tinybar(const uint64_t tinybar);

#endif // LEDGER_APP_IOST_IOST_H
