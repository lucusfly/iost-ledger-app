#include "handlers.h"
#include "globals.h"
#include "handlers.h"
#include "utils.h"
#include "io.h"
#include "ui.h"
#include "iost.h"
#include "errors.h"
#include "debug.h"
#include "printf.h"
#include <bagl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
//#include "get_public_key.h"

// Sizes in Characters, not Bytes
// Used for Display Only
static const uint8_t KEY_SIZE = 64;
static const uint8_t DISPLAY_SIZE = 12;
//// Arbitrary IDs for Buttons
//static const uint8_t LEFT_BTN_ID = BUTTON_LEFT;
//static const uint8_t RIGHT_BTN_ID = BUTTON_RIGHT;

static struct get_public_key_context_t {
//    uint32_t key_index;
    uint8_t bip_32_length;
    uint32_t bip_32_path[BIP32_PATH_LENGTH];

    // Lines on the UI Screen
    // L1 Only used for title in Nano X compare
    char ui_approve_l1[40];
    char ui_approve_l2[40];

    cx_ecfp_public_key_t public_key;

    // Public Key Compare
    uint8_t display_index;
    uint8_t full_key[KEY_SIZE + 1];
    uint8_t partial_key[DISPLAY_SIZE + 1];
} ctx;

#if defined(TARGET_NANOS)


static const bagl_element_t ui_get_public_key_compare[] = {
    UI_BACKGROUND(),
    UI_ICON_LEFT(BUTTON_LEFT, BAGL_GLYPH_ICON_LEFT),
    UI_ICON_RIGHT(BUTTON_RIGHT, BAGL_GLYPH_ICON_RIGHT),
    // <=                  =>
    //      Compare:
    //      <partial>
    //
    UI_TEXT(0x00, 0, 12, 128, "Public Key"),
    UI_TEXT(0x00, 0, 26, 128, ctx.partial_key)
};

static const bagl_element_t ui_get_public_key_approve[] = {
    UI_BACKGROUND(),
    UI_ICON_LEFT(0x00, BAGL_GLYPH_ICON_CROSS),
    UI_ICON_RIGHT(0x00, BAGL_GLYPH_ICON_CHECK),
    //
    //    Export Public
    //       Key #123?
    //
    UI_TEXT(0x00, 0, 12, 128, "Export Public"),
    UI_TEXT(0x00, 0, 26, 128, ctx.ui_approve_l2),
};

void shift_partial_key() {
    os_memmove(
        ctx.partial_key,
        ctx.full_key + ctx.display_index,
        DISPLAY_SIZE
    );
}

static unsigned int ui_get_public_key_compare_button(
    unsigned int button_mask, 
    unsigned int button_mask_counter
) {
    UNUSED(button_mask_counter);
    switch (button_mask) {
        case BUTTON_LEFT: // Left
        case BUTTON_EVT_FAST | BUTTON_LEFT:
            if (ctx.display_index > 0) ctx.display_index--;
            shift_partial_key();
            UX_REDISPLAY();
            break;
        case BUTTON_RIGHT: // Right
        case BUTTON_EVT_FAST | BUTTON_RIGHT:
            if (ctx.display_index < KEY_SIZE - DISPLAY_SIZE) ctx.display_index++;
            shift_partial_key();
            UX_REDISPLAY();
            break;
        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT: // Continue
            io_exchange_with_code(EXCEPTION_OK, 32);
            ui_idle();
            break;
    }
    return 0;
}

static const bagl_element_t* ui_prepro_get_public_key_compare(
    const bagl_element_t* element
) {
    if (element->component.userid == BUTTON_LEFT
        && ctx.display_index == 0)
        return NULL; // Hide Left Arrow at Left Edge
    if (element->component.userid == BUTTON_RIGHT
        && ctx.display_index == KEY_SIZE - DISPLAY_SIZE) 
        return NULL; // Hide Right Arrow at Right Edge
    return element;
}

void compare_pk() {
    // init partial key str from full str
    os_memmove(ctx.partial_key, ctx.full_key, DISPLAY_SIZE);
    ctx.partial_key[DISPLAY_SIZE] = '\0';
    
    // init display index
    ctx.display_index = 0;

    // Display compare with button mask
    UX_DISPLAY(
        ui_get_public_key_compare, 
        ui_prepro_get_public_key_compare
    );
}

static unsigned int ui_get_public_key_approve_button(
    unsigned int button_mask, 
    unsigned int button_mask_counter
) {
    UNUSED(button_mask_counter);
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT: // REJECT
            io_exchange_with_code(EXCEPTION_USER_REJECTED, 0);
            ui_idle();
            break;

        case BUTTON_EVT_RELEASED | BUTTON_RIGHT: // APPROVE
            compare_pk();
            break;

        default:
            break;
    }

    return 0;
}

#elif defined(TARGET_NANOX)

unsigned int io_seproxyhal_touch_pk_ok(const bagl_element_t *e) {
    io_exchange_with_code(EXCEPTION_OK, 32);
    ui_idle();
    return 0;
}

unsigned int io_seproxyhal_touch_pk_cancel(const bagl_element_t *e) {
     io_exchange_with_code(EXCEPTION_USER_REJECTED, 0);
     ui_idle();
     return 0;
}

UX_STEP_NOCB(
    ux_compare_pk_flow_1_step,
    bn,
    {
        "Export Public",
        ctx.ui_approve_l2
    }
);

UX_STEP_NOCB(
    ux_compare_pk_flow_2_step,
    bnnn_paging,
    {
        .title = ctx.ui_approve_l1,
        .text = (char*) ctx.full_key
    }
);

UX_STEP_VALID(
    ux_compare_pk_flow_3_step,
    pb,
    io_seproxyhal_touch_pk_ok(NULL),
    {
       &C_icon_validate_14,
       "Approve"
    }
);

UX_STEP_VALID(
    ux_compare_pk_flow_4_step,
    pb,
    io_seproxyhal_touch_pk_cancel(NULL),
    {
        &C_icon_crossmark,
        "Reject"
    }
);

UX_DEF(
    ux_compare_pk_flow,
    &ux_compare_pk_flow_1_step,
    &ux_compare_pk_flow_2_step,
    &ux_compare_pk_flow_3_step,
    &ux_compare_pk_flow_4_step
);

#endif // TARGET_NANOX

void get_pk()
{
    // Derive Key
//    iost_derive_keypair(ctx.key_index, NULL, &ctx.public);
    if (iost_derive_keypair(ctx.bip_32_path, ctx.bip_32_length, NULL, &ctx.public_key) != 0) {
        THROW(EXCEPTION_INTERNAL_ERROR);
    }

    // Put Key bytes in APDU buffer
    public_key_to_bytes(&ctx.public_key, G_io_apdu_buffer);

    // Populate Key Hex String
    bin2hex(ctx.full_key, G_io_apdu_buffer, KEY_SIZE);
    ctx.full_key[KEY_SIZE] = '\0';
}

void handle_get_public_key(
        uint8_t p1,
        uint8_t p2,
        const uint8_t* const buffer,
        uint16_t size,
        /* out */ volatile unsigned int* flags,
        /* out */ volatile unsigned int* tx
) {
    UNUSED(p1);
    UNUSED(p2);
    UNUSED(tx);

    // Read BIP32 path
    ctx.bip_32_length = io_read_bip32(buffer, size, ctx.bip_32_path);
//    ctx.key_index = bip_32_path[bip_32_length - 1];
    // ctx.key_index = U4LE(buffer, 0);

    // Title for Nano X compare screen
    iost_snprintf(ctx.ui_approve_l1, 40, "Public Key #%u", ctx.bip_32_path[ctx.bip_32_length - 1]);

    // Complete "Export Public | Key #x?"
    iost_snprintf(ctx.ui_approve_l2, 40, "Key #%u?", ctx.bip_32_path[ctx.bip_32_length - 1]);

    // Populate context with PK
    get_pk();

#if defined(TARGET_NANOS)

    UX_DISPLAY(ui_get_public_key_approve, NULL);

#elif defined(TARGET_NANOX)

    ux_flow_init(0, ux_compare_pk_flow, NULL);

#endif // TARGET

    *flags |= IO_ASYNCH_REPLY;
}
