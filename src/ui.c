#include "ui.h"
#include "utils.h"
#include <ux.h>
/*
 * Defines the main menu and idle actions for the app
 */


#if defined(TARGET_NANOS)
ux_state_t ux;
unsigned int ux_step;
unsigned int ux_step_count;

static const ux_menu_entry_t menu_main[4];

static const ux_menu_entry_t menu_about[3] = {
    {
        .menu = NULL,
        .callback = NULL,
        .userid = 0,
        .icon = NULL,
        .line1 = "Version",
        .line2 = APPVERSION,
        .text_x = 0,
        .icon_x = 0,
    },

    {
        .menu = menu_main,
        .callback = NULL,
        .userid = 0,
        .icon = &C_icon_back,
        .line1 = "Back",
        .line2 = NULL,
        .text_x = 61,
        .icon_x = 40,
    },

    UX_MENU_END
};

static const ux_menu_entry_t menu_main[4] = {
    {
        .menu = NULL,
        .callback = NULL,
        .userid = 0,
        .icon = NULL,
        .line1 = "Awaiting",
        .line2 = "Commands",
        .text_x = 0,
        .icon_x = 0
    },
    {
        .menu = menu_about,
        .callback = NULL,
        .userid = 0,
        .icon = NULL,
        .line1 = "About",
        .line2 = NULL,
        .text_x = 0,
        .icon_x = 0,
    },

    {
        .menu = NULL,
        .callback = &callback_os_exit,
        .userid = 0,
        .icon = &C_icon_dashboard,
        .line1 = "Quit app",
        .line2 = NULL,
        .text_x = 50,
        .icon_x = 29,
    },

    UX_MENU_END
};

//bagl_element_t ui_background()
//{
//    return ;
//}

//bagl_element_t ui_icon_left(unsigned char user_id, unsigned char icon_id)
//{
//    return {
//        { BAGL_ICON, user_id, 3, 12, 7, 7, 0, 0, 0, 0xFFFFFF, 0, 0, icon_id },
//        NULL,
//        0,
//        0,
//        0,
//        NULL,
//        NULL,
//        NULL
//    };
//}
//bagl_element_t ui_icon_right(unsigned char user_id, unsigned char icon_id)
//{
//    return {
//        { BAGL_ICON, user_id, 117, 13, 8, 6, 0, 0, 0, 0xFFFFFF, 0, 0, icon_id },
//        NULL,
//        0,
//        0,
//        0,
//        NULL,
//        NULL,
//        NULL
//    };
//}
//bagl_element_t ui_text(unsigned char user_id, short x, short y, unsigned short width, const char* const text)
//{
//    return {
//        { BAGL_LABELINE, user_id, x, y, width, 12, 0, 0, 0, 0xFFFFFF, 0, BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0 },
//        text,
//        0,
//        0,
//        0,
//        NULL,
//        NULL,
//        NULL
//    };
//}

#elif defined(TARGET_NANOX)

ux_state_t G_ux;
bolos_ux_params_t G_ux_params;

UX_STEP_NOCB(
    ux_idle_flow_1_step,
    nn,
    {
        "Awaiting",
        "Commands"
    }
);

UX_STEP_NOCB(
    ux_idle_flow_2_step,
    bn,
    {
        "Version",
        APPVERSION,
    }
);

UX_STEP_VALID(
    ux_idle_flow_3_step,
    pb,
    os_sched_exit(-1),
    {
        &C_icon_dashboard_x,
        "Exit"
    }
);

UX_DEF(
    ux_idle_flow,
    &ux_idle_flow_1_step,
    &ux_idle_flow_2_step,
    &ux_idle_flow_3_step
);

#endif // TARGET_NANOX

void ui_idle(void) {
#if defined(TARGET_NANOS)
    UX_MENU_DISPLAY(0, menu_main, NULL);
#elif defined(TARGET_NANOX)
    if (G_ux.stack_count == 0) {
        ux_stack_push();
    }
    ux_flow_init(0, ux_idle_flow, NULL);
#endif // TARGET_NANOS
}

void io_seproxyhal_display(const bagl_element_t* element) {
    io_seproxyhal_display_default(element);
}

void ui_shift_msg(ui_context_t* context) {
    os_memcpy(
        context->partial_msg,
        context->msg_body + context->display_index,
        DISPLAY_SIZE
    );
}

unsigned int ui_compare_button(
    ui_context_t* context,
    const unsigned int button_mask,
    const unsigned int button_mask_counter
) {
    UNUSED(button_mask_counter);
    switch (button_mask) {
        case BUTTON_LEFT: // Left
        case BUTTON_EVT_FAST | BUTTON_LEFT:
            if (context->display_index > 0) {
                context->display_index--;
            }
            ui_shift_msg(context);
            UX_REDISPLAY();
            break;
        case BUTTON_RIGHT: // Right
        case BUTTON_EVT_FAST | BUTTON_RIGHT:
            if (context->display_index < context->msg_length - DISPLAY_SIZE) {
                context->display_index++;
            }
            ui_shift_msg(context);
            UX_REDISPLAY();
            break;
        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT: // Continue
            ui_clear_context(context);
            ui_idle();
            break;
    }
    return 0;
}

const bagl_element_t* ui_prepro_compare(
    ui_context_t* context,
    const bagl_element_t* element
) {
    if (
        (element->component.userid == LEFT_ICON_ID) &&
        (context->display_index == 0)
    ) {
        return NULL; // Hide Left Arrow at Left Edge
    }
    if (
        (element->component.userid == RIGHT_ICON_ID) &&
        (context->display_index == context->msg_length - DISPLAY_SIZE)
    ) {
        return NULL; // Hide Right Arrow at Right Edge
    }
    return element;
}

void ui_compare_msg(
    ui_context_t* context
) {
    // init partial key str from full str
    os_memmove(context->partial_msg, context->msg_body, DISPLAY_SIZE);
    context->partial_msg[DISPLAY_SIZE] = '\0';

    // init display index
    context->display_index = 0;
}

void ui_clear_context(
    ui_context_t* context
) {
    os_memset(context, 0, sizeof(ui_context_t));
}
