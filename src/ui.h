#ifndef LEDGER_APP_IOST_UI_H
#define LEDGER_APP_IOST_UI_H

#include "glyphs.h"
//#include <bagl.h>
//#include "globals.h"

struct bagl_element_s;

// Sizes in Characters, not Bytes
// Used for Display Only
#define DISPLAY_SIZE 18 // characters @ 11pt sys font

// User IDs for BAGL Elements
#define LINE_1_ID 5
#define LINE_2_ID 6
#define LEFT_ICON_ID 1
#define RIGHT_ICON_ID 2

#if defined(TARGET_NANOS)
#define UI_BAGLE_ELEMENT(text) (const char* const)(text)
#elif defined(TARGET_NANOX)
#define UI_BAGLE_ELEMENT(text) (const char* const)(text)
#elif defined(TARGET_BLUE)
#define UI_BAGLE_ELEMENT(text) (const char* const)(text),0,0,0,NULL,NULL,NULL
#endif // TARGET_NANOX

#if defined(TARGET_NANOS)

// Common UI element definit1ons for Ledger Nano S, Ledger Nano X and Ledger Blue
#define UI_BACKGROUND() {{BAGL_RECTANGLE,0,0,0,128,32,0,0,BAGL_FILL,0,0xFFFFFF,0,0},UI_BAGLE_ELEMENT(NULL)}
#define UI_ICON_LEFT(userid, glyph) {{BAGL_ICON,userid,3,12,7,7,0,0,0,0xFFFFFF,0,0,glyph},UI_BAGLE_ELEMENT(NULL)}
#define UI_ICON_RIGHT(userid, glyph) {{BAGL_ICON,userid,117,13,8,6,0,0,0,0xFFFFFF,0,0,glyph},UI_BAGLE_ELEMENT(NULL)}
#define UI_TEXT(userid, x, y, w, text) {{BAGL_LABELINE,userid,x,y,w,12,0,0,0,0xFFFFFF,0,BAGL_FONT_OPEN_SANS_REGULAR_11px|BAGL_FONT_ALIGNMENT_CENTER,0},UI_BAGLE_ELEMENT(text)}
#define UI_ICON(userid, x, y, w, glyph) {{BAGL_ICON,userid,x,y,w,6,0,0,0,0xFFFFFF,0,BAGL_FONT_OPEN_SANS_REGULAR_11px|BAGL_FONT_ALIGNMENT_CENTER,glyph},NULL}

#elif defined(TARGET_NANOX)

#include "ux.h"
// Common UI element definitions for Nano X

#endif // TARGET_NANOS

//extern bagl_element_t ui_background();
//extern bagl_element_t ui_icon_left(unsigned char user_id, unsigned char icon_id);
//extern bagl_element_t ui_icon_right(unsigned char user_id, unsigned char icon_id);
//extern bagl_element_t ui_text(unsigned char userid, short x, short y, unsigned short width, const char* const text);

extern void ui_idle(void);
extern void seproxyhal_display(const struct bagl_element_s* element);

typedef struct ui_context
{
    uint8_t msg_body[DISPLAY_SIZE * DISPLAY_SIZE];
    uint16_t msg_length;

    // Lines on the UI Screen
    // L1 Only used for title in Nano X compare
    char approve_l2[DISPLAY_SIZE + 1];
    // Public Key Compare
    uint8_t display_index;
    uint8_t partial_msg[DISPLAY_SIZE + 1];
} ui_context_t;

extern void ui_shift_msg(
    ui_context_t* context
);
extern unsigned int ui_compare_button(
    ui_context_t* context,
    const unsigned int button_mask,
    const unsigned int button_mask_counter
);
extern const bagl_element_t* ui_prepro_compare(
    ui_context_t* context,
    const bagl_element_t* element
);
extern void ui_compare_msg(
    ui_context_t* context
);
extern void ui_clear_context(
    ui_context_t* context
);

#endif // LEDGER_APP_IOST_UI_H
