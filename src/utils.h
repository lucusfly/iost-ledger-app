#ifndef LEDGER_APP_IOST_UTILS_H
#define LEDGER_APP_IOST_UTILS_H

#include <stdint.h>

extern uint32_t bin2hex(const uint8_t* const data, const uint32_t inlen, uint8_t* dst);
extern void callback_os_exit(unsigned int exit_code);
extern const uint8_t* chars_2_bytes(const char* const chars);
extern const char* bytes_2_chars(const uint8_t* const bytes);


#endif // LEDGER_APP_IOST_UTILS_H
