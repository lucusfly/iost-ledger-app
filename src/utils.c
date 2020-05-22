#include "utils.h"
#include <os.h>

uint32_t bin2hex(
        const uint8_t* const data,
        const uint32_t in_len,
        uint8_t* dst
) {
	static uint8_t const hex[] = "0123456789abcdef";

    for (uint32_t i = 0; i < in_len; i++) {
		dst[2*i+0] = hex[(data[i]>>4) & 0x0F];
		dst[2*i+1] = hex[(data[i]>>0) & 0x0F];
	}
    return in_len * 2;
}

void callback_os_exit(unsigned int exit_code)
{
    os_sched_exit(exit_code);
}

const uint8_t* chars_2_bytes(const char* const chars)
{
    return (const uint8_t*)(chars);
}

const char* bytes_2_chars(const uint8_t* const bytes)
{
    return (const char*)(bytes);
}
