#ifndef LEDGER_APP_IOST_IO_H
#define LEDGER_APP_IOST_IO_H

#include <stdint.h>

/* Although SEP-0005 only allows 3 bip32 path elements we support more */
#define APDU_MIN_SIZE 1
#define APDU_MAX_SIZE 5
#define BIP32_PATH_MASK 0x80000000
#define BIP32_PATH_LENGTH APDU_MAX_SIZE
//#define FULL_ADDRESS_LENGTH 54
#define MAX_MSG_SIZE 512

extern uint16_t io_read_bip32(
    const uint8_t* buffer,
    uint16_t buffer_length,
    uint32_t *bip32_path
);
extern void io_set_status(
    const uint16_t sw,
    volatile uint8_t* flags,
    volatile uint16_t* tx
);
extern void io_exchange_status(
    const uint16_t status,
    uint16_t tx
);

extern void io_check_p1p2(
    const uint8_t p1,
    const uint8_t p2
);

extern void io_print_buffer(
    const char* const name,
    const uint8_t is_str,
    const uint8_t* const buffer,
    const uint16_t length
);

#endif // LEDGER_APP_IOST_IO_H
