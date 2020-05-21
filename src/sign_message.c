//#include "sign_transaction.h"
#include "handlers.h"
#include "globals.h"
#include "utils.h"
#include "printf.h"
#include "debug.h"
#include "errors.h"
#include "io.h"
#include "ui.h"
#include "iost.h"
#include <stdbool.h>

static const int ACCOUNT_ID_SIZE = 19 * 3 + 2 + 1;
static const int MAX_MEMO_SIZE = 200;

enum TransactionStep {
    Summary = 1,
    Operator = 2,
    Senders = 3,
    Recipients = 4,
    Amount = 5,
    Fee = 6,
    Memo =  7,
    Confirm = 8,
    Deny = 9
};

enum TransactionType {
    Unknown = -1,
    Verify = 0,
    Create = 1,
    Transfer = 2
};

void reformat_operator();
void reformat_senders();
void reformat_recipients();
void reformat_amount();
void reformat_fee();
void reformat_memo();
void handle_transaction_body();

#if defined(TARGET_NANOS)
static struct sign_tx_context_t {
    uint8_t msg_body[MAX_MSG_SIZE];
    uint16_t msg_length;
    uint16_t signature_length;

    // ui common
    uint8_t transfer_to_index;
    uint8_t transfer_from_index;

    // Transaction Summary
    char summary_line_1[DISPLAY_SIZE + 1];
    char summary_line_2[DISPLAY_SIZE + 1];
    char title[DISPLAY_SIZE + 1];
    
    // Account ID: uint64_t.uint64_t.uint64_t
    // Most other entities are shorter
    char full[ACCOUNT_ID_SIZE + 1];
    char partial[DISPLAY_SIZE + 1];
    
    // Steps correspond to parts of the transaction proto
    // type is set based on proto
    enum TransactionStep step;
    enum TransactionType type;

    uint8_t display_index;  // 1 -> Number Screens
    uint8_t display_count;  // Number Screens

    // Parsed transaction
//    HederaTransactionBody transaction;
} context;

// Forward declarations for Nano S UI

// Step 1
unsigned int ui_tx_summary_step_button(
    unsigned int button_mask,
    unsigned int button_mask_counter
);

// Step 2 - 7
void handle_intermediate_left_press();
void handle_intermediate_right_press();
    unsigned int ui_tx_intermediate_step_button(
    unsigned int button_mask,
    unsigned int button_mask_counter
);

// Step 8
unsigned int ui_tx_confirm_step_button(
    unsigned int button_mask,
    unsigned int button_mask_counter
);

// Step 9
unsigned int ui_tx_deny_step_button(
    unsigned int button_mask,
    unsigned int button_mask_counter
);

uint8_t num_screens(size_t length);
void count_screens();
void shift_display();
bool first_screen();
bool last_screen();

// UI Definition for Nano S
// Step 1: Transaction Summary
static const bagl_element_t ui_tx_summary_step[] = {
    UI_BACKGROUND(),
    UI_ICON_RIGHT(RIGHT_ICON_ID, BAGL_GLYPH_ICON_RIGHT),

    // ()       >>
    // Line 1
    // Line 2

    UI_TEXT(LINE_1_ID, 0, 12, 128, context.summary_line_1),
    UI_TEXT(LINE_2_ID, 0, 26, 128, context.summary_line_2)
};

// Step 2 - 7: Operator, Senders, Recipients, Amount, Fee, Memo
static const bagl_element_t ui_tx_intermediate_step[] = {
    UI_BACKGROUND(),
    UI_ICON_LEFT(LEFT_ICON_ID, BAGL_GLYPH_ICON_LEFT),
    UI_ICON_RIGHT(RIGHT_ICON_ID, BAGL_GLYPH_ICON_RIGHT),

    // <<       >>
    // <Title>
    // <Partial>

    UI_TEXT(LINE_1_ID, 0, 12, 128, context.title),
    UI_TEXT(LINE_2_ID, 0, 26, 128, context.partial)
};

// Step 8: Confirm
static const bagl_element_t ui_tx_confirm_step[] = {
    UI_BACKGROUND(),
    UI_ICON_LEFT(LEFT_ICON_ID, BAGL_GLYPH_ICON_LEFT),
    UI_ICON_RIGHT(RIGHT_ICON_ID, BAGL_GLYPH_ICON_RIGHT),

    // <<       >>
    //    Confirm
    //    <Check>

    UI_TEXT(LINE_1_ID, 0, 12, 128, "Confirm"),
    UI_ICON(LINE_2_ID, 0, 24, 128, BAGL_GLYPH_ICON_CHECK)
};

// Step 9: Deny
static const bagl_element_t ui_tx_deny_step[] = {
    UI_BACKGROUND(),
    UI_ICON_LEFT(LEFT_ICON_ID, BAGL_GLYPH_ICON_LEFT),

    // <<       ()
    //    Deny
    //      X

    UI_TEXT(LINE_1_ID, 0, 12, 128, "Deny"),
    UI_ICON(LINE_2_ID, 0, 24, 128, BAGL_GLYPH_ICON_CROSS)
};

// Step 1: Transaction Summary
unsigned int ui_tx_summary_step_button(
    unsigned int button_mask,
    unsigned int button_mask_counter
) {
    switch(button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
            if (context.type == Verify) {
                context.step = Senders;
                context.display_index = 1;
                reformat_senders();
            } else {
                context.step = Operator;
                context.display_index = 1;
                reformat_operator();
            }
            UX_DISPLAY(ui_tx_intermediate_step, NULL);
            break;
    }

    return 0;
}

void handle_intermediate_left_press() {
    // Navigate Left (scroll or return to previous step)
    switch (context.step) {
        case Deny:
        case Confirm:
        case Summary:
            PRINTF("Unhandled step %u\n", context.step);
            break;
        case Operator: {
            if (first_screen()) {  // Return to Summary
                context.step = Summary;
                context.display_index = 1;
                UX_DISPLAY(ui_tx_summary_step, NULL);
            } else {  // Scroll Left
                context.display_index--;
                reformat_operator();
                UX_REDISPLAY();
            }
        } break;
        case Senders: {
            if (first_screen()) {  // Return to Operator
                if (context.type == Verify) {
                    context.step = Summary;
                    context.display_index = 1;
                    UX_DISPLAY(ui_tx_summary_step, NULL);
                } else {
                    context.step = Operator;
                    context.display_index = 1;
                    reformat_operator();
                }
            } else {  // Scroll Left
                context.display_index--;
                reformat_senders();
            }
            UX_REDISPLAY();
        } break;
        case Recipients: {
            if (first_screen()) {  // Return to Senders
                context.step = Senders;
                context.display_index = 1;
                reformat_senders();
            } else {  // Scroll Left
                context.display_index--;
                reformat_recipients();
            }
            UX_REDISPLAY();
        } break;
        case Amount: {
            if (first_screen()) {
                if (context.type == Create) {  // Return to Operator
                    context.step = Operator;
                    context.display_index = 1;
                    reformat_operator();
                } else if (context.type == Transfer) {  // Return to Recipients
                    context.step = Recipients;
                    context.display_index = 1;
                    reformat_recipients();
                }
            } else {  // Scroll left
                context.display_index--;
                reformat_amount();
            }
            UX_REDISPLAY();
        } break;
        case Fee: {
            if (first_screen()) {  // Return to Amount
                context.step = Amount;
                context.display_index = 1;
                reformat_amount();
            } else {  // Scroll left
                context.display_index--;
                reformat_fee();
            }
            UX_REDISPLAY();
        } break;
        case Memo: {
            if (first_screen()) {  // Return to Fee
                context.step = Fee;
                context.display_index = 1;
                reformat_fee();
            } else {  // Scroll Left
                context.display_index--;
                reformat_memo();
            }
            UX_REDISPLAY();
        } break;
    }
}

void handle_intermediate_right_press() {
    // Navigate Right (scroll or continue to next step)
    switch (context.step) {
        case Deny:
        case Confirm:
        case Summary:
            PRINTF("Unhandled step %u\n", context.step);
            break;
        case Operator: {
            if (last_screen()) {
                if (context.type == Create) {  // Continue to Amount
                    context.step = Amount;
                    context.display_index = 1;
                    reformat_amount();
                } else {  // Continue to Senders
                    context.step = Senders;
                    context.display_index = 1;
                    reformat_senders();
                }
            } else {  // Scroll Right
                context.display_index++;
                reformat_operator();
            }
            UX_REDISPLAY();
        } break;
        case Senders: {
            if (last_screen()) {
                if (context.type == Verify) {  // Continue to Confirm
                    context.step = Confirm;
                    UX_DISPLAY(ui_tx_confirm_step, NULL);
                } else {  // Continue to Recipients
                    context.step = Recipients;
                    context.display_index = 1;
                    reformat_recipients();
                }
            } else {  // Scroll Right
                context.display_index++;
                reformat_senders();
            }
            UX_REDISPLAY();
        } break;
        case Recipients: {
            if (last_screen()) {  // Continue to Amount
                context.step = Amount;
                context.display_index = 1;
                reformat_amount();
            } else {  // Scroll Right
                context.display_index++;
                reformat_recipients();
            }
            UX_REDISPLAY();
        } break;
        case Amount: {
            if (last_screen()) {  // Continue to Fee
                context.step = Fee;
                context.display_index = 1;
                reformat_fee();
            } else {  // Scroll Right
                context.display_index++;
                reformat_amount();
            }
            UX_REDISPLAY();
        } break;
        case Fee: {
            if (last_screen()) {  // Continue to Memo
                context.step = Memo;
                context.display_index = 1;
                reformat_memo();
            } else {  // Scroll Right
                context.display_index++;
                reformat_fee();
            }
            UX_REDISPLAY();
        } break;
        case Memo: {
            if (last_screen()) {  // Continue to Confirm
                context.step = Confirm;
                context.display_index = 1;
                UX_DISPLAY(ui_tx_confirm_step, NULL);
            } else {  // Scroll Right
                context.display_index++;
                reformat_memo();
                UX_REDISPLAY();
            }
        } break;
    }
}

// Step 2 - 7: Operator, Senders, Recipients, Amount, Fee, Memo
unsigned int ui_tx_intermediate_step_button(
    unsigned int button_mask,
    unsigned int button_mask_counter
) {
    switch(button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
            handle_intermediate_left_press();
            break;
        case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
            handle_intermediate_right_press();
            break;
        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
            // Skip to confirm screen
            context.step = Confirm;
            UX_DISPLAY(ui_tx_confirm_step, NULL);
            break;
    }

    return 0;
}

unsigned int ui_tx_confirm_step_button(
    unsigned int button_mask,
    unsigned int button_mask_counter
) {
    switch(button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
            if (context.type == Verify) {  // Return to Senders
                context.step = Senders;
                context.display_index = 1;
                reformat_senders();
            } else { // Return to Memo
                context.step = Memo;
                context.display_index = 1;
                reformat_memo();
            }
            UX_DISPLAY(ui_tx_intermediate_step, NULL);
            break;
        case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
            // Continue to Deny
            context.step = Deny;
            UX_DISPLAY(ui_tx_deny_step, NULL);
            break;
        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
            // Exchange Signature (OK)
            io_exchange_status(SW_OK, 64);
            ui_idle();
            break;
    }

    return 0;
}

unsigned int ui_tx_deny_step_button(
    unsigned int button_mask,
    unsigned int button_mask_counter
) {
    switch(button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
            // Return to Confirm
            context.step = Confirm;
            UX_DISPLAY(ui_tx_confirm_step, NULL);
            break;
        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
            // Reject
            context.step = Unknown;
            io_exchange_status(SW_USER_REJECTED, 0);
            ui_idle();
            break;
    }

    return 0;
}

uint8_t num_screens(size_t length) {
    // Number of screens is len / display size + 1 for overflow
    if (length == 0) return 1;
    uint8_t screens = length / DISPLAY_SIZE;
    if (length % DISPLAY_SIZE > 0) screens += 1;
    return screens;
}

void count_screens() {
    context.display_count = num_screens(strlen(context.full));
}

void shift_display() {
    // Slide window (partial) along full entity (full) by DISPLAY_SIZE chars
    os_memset(context.partial, '\0', DISPLAY_SIZE + 1);
    os_memmove(
        context.partial,
        context.full + (DISPLAY_SIZE * (context.display_index - 1)),
        DISPLAY_SIZE
    );
}

bool last_screen() {
    return context.display_index == context.display_count;
}

bool first_screen() {
    return context.display_index == 1;
}

void reformat_operator() {
    iost_snprintf(
        context.full,
        ACCOUNT_ID_SIZE,
        "%llu.%llu.%llu",
        0,//ctx.transaction.transactionID.accountID.shardNum,
        0,//ctx.transaction.transactionID.accountID.realmNum,
        0//ctx.transaction.transactionID.accountID.accountNum
    );

    count_screens();
    
    iost_snprintf(
        context.title,
        DISPLAY_SIZE,
        "Operator (%u/%u)",
        context.display_index,
        context.display_count
    );

    shift_display();
}

void reformat_accounts(char* title_part, uint8_t transfer_index) {
    iost_snprintf(
        context.full,
        ACCOUNT_ID_SIZE,
        "%llu.%llu.%llu",
        0,//ctx.transaction.data.cryptoTransfer.transfers.accountAmounts[transfer_index].accountID.shardNum,
        0,//ctx.transaction.data.cryptoTransfer.transfers.accountAmounts[transfer_index].accountID.realmNum,
        0//ctx.transaction.data.cryptoTransfer.transfers.accountAmounts[transfer_index].accountID.accountNum
    );

    count_screens();

    iost_snprintf(
        context.title,
        DISPLAY_SIZE,
        "%s (%u/%u)",
        title_part,
        context.display_index,
        context.display_count
    );
}

void reformat_senders() {
    if (context.type == Verify) {
        reformat_accounts("Account", 0);
    } else {
        reformat_accounts("Sender", context.transfer_from_index);
    }

    shift_display();
}

void reformat_recipients() {
    reformat_accounts("Recipient", context.transfer_to_index);
    shift_display();
}

void reformat_amount() {
    switch (context.type) {
        case Unknown:
        case Verify:
            PRINTF("Unhandled type %u\n", context.type);
            break;
        case Create:
            iost_snprintf(
                context.full,
                DISPLAY_SIZE * 3,
                "%s hbar",
                "X.X"//hedera_format_tinybar(ctx.transaction.data.cryptoCreateAccount.initialBalance)
            );
            break;
        case Transfer:
            iost_snprintf(
                context.full,
                DISPLAY_SIZE * 3,
                "%s hbar",
                "X.X"//hedera_format_tinybar(ctx.transaction.data.cryptoTransfer.transfers.accountAmounts[ctx.transfer_to_index].amount)
            );
            break;
    }

    count_screens();

    iost_snprintf(
        context.title,
        DISPLAY_SIZE,
        "%s (%u/%u)",
        context.type == Create ? "Balance" : "Amount",
        context.display_index,
        context.display_count
    );

    shift_display();
}

void reformat_fee() {
    iost_snprintf(
        context.full,
        DISPLAY_SIZE * 3,
        "%s hbar",
        "X.X"//hedera_format_tinybar(ctx.transaction.transactionFee)
    );

    count_screens();

    iost_snprintf(
        context.title,
        DISPLAY_SIZE,
        "Fee (%u/%u)",
        context.display_index,
        context.display_count
    );

    shift_display();
}

void reformat_memo() {
    iost_snprintf(
        context.full,
        MAX_MEMO_SIZE,
        "%s",
        "MEMO"//ctx.transaction.memo
    );

    if (strlen(context.full) > MAX_MEMO_SIZE) {
        // :grimacing:
        THROW(SW_WRONG_LENGTH);
    }

    count_screens();

    iost_snprintf(
        context.title,
        DISPLAY_SIZE,
        "Memo (%u/%u)",
        context.display_index,
        context.display_count
    );

    shift_display();
}

void handle_transaction_body(const uint32_t key_index) {
    os_memset(context.summary_line_1, '\0', DISPLAY_SIZE + 1);
    os_memset(context.summary_line_2, '\0', DISPLAY_SIZE + 1);
    os_memset(context.full, '\0', ACCOUNT_ID_SIZE + 1);
    os_memset(context.partial, '\0', DISPLAY_SIZE + 1);

    // Step 1, Unknown Type, Screen 1 of 1
    context.step = Summary;
    context.type = Unknown;
    context.display_index = 1;
    context.display_count = 1;

    // <Do Action> 
    // with Key #X?
    iost_snprintf(
        context.summary_line_2,
        DISPLAY_SIZE,
        "with Key #%u?",
        key_index
    );
    iost_snprintf(
        context.summary_line_1,
        DISPLAY_SIZE,
        "Create Account"
    );


    // Handle parsed protobuf message of transaction body
//    switch (ctx.transaction.which_data) {
//        case HederaTransactionBody_cryptoCreateAccount_tag:
//            // Create Account Transaction
//            ctx.type = Create;
//            iost_snprintf(
//                ctx.summary_line_1,
//                DISPLAY_SIZE,
//                "Create Account"
//            );
//            break;

//        case HederaTransactionBody_cryptoTransfer_tag: {
//            // Transfer Transaction
//            if (ctx.transaction.data.cryptoTransfer.transfers.accountAmounts_count > 2) {
//                // Unsupported (number of accounts > 2)
//                THROW(EXCEPTION_MALFORMED_APDU);
//            }

//            if ( // Only 1 Account (Sender), Fee 1 Tinybar, and Value 0 Tinybar
//                ctx.transaction.data.cryptoTransfer.transfers.accountAmounts[0].amount == 0 &&
//                ctx.transaction.data.cryptoTransfer.transfers.accountAmounts_count == 1 &&
//                ctx.transaction.transactionFee == 1) {
//                    // Verify Account Transaction
//                    ctx.type = Verify;
//                    iost_snprintf(
//                        ctx.summary_line_1,
//                        DISPLAY_SIZE,
//                        "Verify Account"
//                    );

//            } else { // Number of Accounts == 2
//                // Some other Transfer Transaction
//                // Determine Sender based on amount
//                ctx.type = Transfer;

//                iost_snprintf(
//                    ctx.summary_line_1,
//                    DISPLAY_SIZE,
//                    "Transfer"
//                );

//                ctx.transfer_to_index = 1;
//                ctx.transfer_from_index = 0;
//                if (ctx.transaction.data.cryptoTransfer.transfers.accountAmounts[0].amount > 0) {
//                    ctx.transfer_to_index = 0;
//                    ctx.transfer_from_index = 1;
//                }
//            }
//        } break;

//        default:
//            // Unsupported
//            THROW(EXCEPTION_MALFORMED_APDU);
//    }

    UX_DISPLAY(ui_tx_summary_step, NULL);
}

#elif defined(TARGET_NANOX)

// Forward declarations for Nano X UI
void x_start_tx_loop();
void x_continue_tx_loop();
void x_end_tx_loop();
unsigned int io_seproxyhal_tx_approve(const bagl_element_t* e);
unsigned int io_seproxyhal_tx_reject(const bagl_element_t* e);

static struct sign_tx_context_t {
    uint8_t msg_body[MAX_TX_SIZE];
    uint16_t signature_length;
    // ui common
    uint8_t transfer_from_index;
    uint8_t transfer_to_index;

    // Transaction Summary
    char summary_line_1[DISPLAY_SIZE + 1];
    char summary_line_2[DISPLAY_SIZE + 1];
    char senders_title[DISPLAY_SIZE + 1];
    char amount_title[DISPLAY_SIZE + 1];
    char partial[DISPLAY_SIZE + 1];
    
    enum TransactionType type;
    
    // Transaction Operator
    char operator[DISPLAY_SIZE * 2 + 1];

    // Transaction Senders
    char senders[DISPLAY_SIZE * 2 + 1];

    // Transaction Recipients
    char recipients[DISPLAY_SIZE * 2 + 1];

    // Transaction Amount
    char amount[DISPLAY_SIZE * 2 + 1];
    
    // Transaction Fee
    char fee[DISPLAY_SIZE * 2 + 1];

    // Transaction Memo
    char memo[MAX_MEMO_SIZE + 1];

    // Parsed transaction
    HederaTransactionBody transaction;
} ctx;

// UI Definition for Nano X

// Confirm Callback
unsigned int io_seproxyhal_tx_approve(const bagl_element_t* e) {
    io_exchange_with_code(EXCEPTION_OK, 64);
    ui_idle();
    return 0;
}

// Reject Callback
unsigned int io_seproxyhal_tx_reject(const bagl_element_t* e) {
    io_exchange_with_code(EXCEPTION_USER_REJECTED, 0);
    ui_idle();
    return 0;
}

UX_STEP_NOCB(
    ux_tx_flow_1_step,
    bnn,
    {
        "Transaction Summary",
        ctx.summary_line_1,
        ctx.summary_line_2
    }
);

UX_STEP_NOCB(
    ux_tx_flow_2_step,
    bnnn_paging,
    {
        .title = "Operator",
        .text = (char*) ctx.operator
    }
);

UX_STEP_NOCB(
    ux_tx_flow_3_step,
    bnnn_paging,
    {
        .title = (char*) ctx.senders_title,
        .text = (char*) ctx.senders
    }
);

UX_STEP_NOCB(
    ux_tx_flow_4_step,
    bnnn_paging,
    {
        .title = "Recipient",
        .text = (char*) ctx.recipients
    }
);

UX_STEP_NOCB(
    ux_tx_flow_5_step,
    bnnn_paging,
    {
        .title = (char*) ctx.amount_title,
        .text = (char*) ctx.amount
    }
);

UX_STEP_NOCB(
    ux_tx_flow_6_step,
    bnnn_paging,
    {
        .title = "Fee",
        .text = (char*) ctx.fee
    }
);

UX_STEP_NOCB(
    ux_tx_flow_7_step,
    bnnn_paging,
    {
        .title = "Memo",
        .text = (char*) ctx.memo
    }
);

UX_STEP_VALID(
    ux_tx_flow_8_step,
    pb,
    io_seproxyhal_tx_approve(NULL),
    {
        &C_icon_validate_14,
        "Confirm"
    }
);

UX_STEP_VALID(
    ux_tx_flow_9_step,
    pb,
    io_seproxyhal_tx_reject(NULL),
    {
        &C_icon_crossmark,
        "Reject"
    }
);

// Transfer UX Flow
UX_DEF(
    ux_transfer_flow,
    &ux_tx_flow_1_step,
    &ux_tx_flow_2_step,
    &ux_tx_flow_3_step,
    &ux_tx_flow_4_step,
    &ux_tx_flow_5_step,
    &ux_tx_flow_6_step,
    &ux_tx_flow_7_step,
    &ux_tx_flow_8_step,
    &ux_tx_flow_9_step
);

// Create UX Flow
UX_DEF(
    ux_create_flow,
    &ux_tx_flow_1_step,
    &ux_tx_flow_2_step,
    &ux_tx_flow_5_step,
    &ux_tx_flow_6_step,
    &ux_tx_flow_7_step,
    &ux_tx_flow_8_step,
    &ux_tx_flow_9_step
);

// Verify UX Flow
UX_DEF(
    ux_verify_flow,
    &ux_tx_flow_1_step,
    &ux_tx_flow_3_step,
    &ux_tx_flow_8_step,
    &ux_tx_flow_9_step
);

void handle_transaction_body() {
    os_memset(ctx.summary_line_1, '\0', DISPLAY_SIZE + 1);
    os_memset(ctx.summary_line_2, '\0', DISPLAY_SIZE + 1);
    os_memset(ctx.amount_title, '\0', DISPLAY_SIZE + 1);
    os_memset(ctx.senders_title, '\0', DISPLAY_SIZE + 1);
    os_memset(ctx.operator, '\0', DISPLAY_SIZE * 2 + 1);
    os_memset(ctx.senders, '\0', DISPLAY_SIZE * 2 + 1);
    os_memset(ctx.recipients, '\0', DISPLAY_SIZE * 2 + 1);
    os_memset(ctx.fee, '\0', DISPLAY_SIZE * 2 + 1);
    os_memset(ctx.amount, '\0', DISPLAY_SIZE * 2 + 1);
    os_memset(ctx.memo, '\0', MAX_MEMO_SIZE + 1);

    ctx.type = Unknown;

    // <Do Action> 
    // with Key #X?
    iost_snprintf(
        ctx.summary_line_2,
        DISPLAY_SIZE,
        "with Key #%u?",
        ctx.key_index
    );

    iost_snprintf(
        ctx.operator,
        DISPLAY_SIZE * 2,
        "%llu.%llu.%llu",
        ctx.transaction.transactionID.accountID.shardNum,
        ctx.transaction.transactionID.accountID.realmNum,
        ctx.transaction.transactionID.accountID.accountNum
    );

    iost_snprintf(
        ctx.fee,
        DISPLAY_SIZE * 2,
        "%s hbar",
        hedera_format_tinybar(ctx.transaction.transactionFee)
    );

    iost_snprintf(
        ctx.memo,
        MAX_MEMO_SIZE,
        "%s",
        ctx.transaction.memo
    );

    hedera_sprintf(
        ctx.amount_title,
        "Amount"
    );

    hedera_sprintf(
        ctx.senders_title,
        "Sender"
    );

    // Handle parsed protobuf message of transaction body
    switch (ctx.transaction.which_data) {
        case HederaTransactionBody_cryptoCreateAccount_tag:
            ctx.type = Create;
            // Create Account Transaction
            hedera_sprintf(
                ctx.summary_line_1,
                "Create Account"
            );
            hedera_sprintf(
                ctx.amount_title,
                "Balance"
            );
            iost_snprintf(
                ctx.amount,
                DISPLAY_SIZE * 2,
                "%s hbar",
                hedera_format_tinybar(ctx.transaction.data.cryptoCreateAccount.initialBalance)
            );
            break;

        case HederaTransactionBody_cryptoTransfer_tag: {
            // Transfer Transaction
            if (ctx.transaction.data.cryptoTransfer.transfers.accountAmounts_count > 2) {
                // Unsupported (number of accounts > 2)
                THROW(EXCEPTION_MALFORMED_APDU);
            }

            if ( // Only 1 Account (Sender), Fee 1 Tinybar, and Value 0 Tinybar
                ctx.transaction.data.cryptoTransfer.transfers.accountAmounts[0].amount == 0 && 
                ctx.transaction.data.cryptoTransfer.transfers.accountAmounts_count == 1 &&
                ctx.transaction.transactionFee == 1) {
                    // Verify Account Transaction
                    ctx.type = Verify;
                    hedera_sprintf(
                        ctx.summary_line_1,
                        "Verify Account"
                    );
                    hedera_sprintf(
                        ctx.senders_title,
                        "Account"
                    );
                    iost_snprintf(
                        ctx.senders,
                        DISPLAY_SIZE * 2,
                        "%llu.%llu.%llu",
                        ctx.transaction.data.cryptoTransfer.transfers.accountAmounts[0].accountID.shardNum,
                        ctx.transaction.data.cryptoTransfer.transfers.accountAmounts[0].accountID.realmNum,
                        ctx.transaction.data.cryptoTransfer.transfers.accountAmounts[0].accountID.accountNum
                    );
                    iost_snprintf(
                        ctx.amount,
                        DISPLAY_SIZE * 2,
                        "%s hbar",
                        hedera_format_tinybar(ctx.transaction.data.cryptoTransfer.transfers.accountAmounts[0].amount)
                    );

            } else { // Number of Accounts == 2
                // Some other Transfer Transaction
                // Determine Sender based on amount
                ctx.type = Transfer;
                hedera_sprintf(
                    ctx.summary_line_1,
                    "Transfer"
                );

                ctx.transfer_from_index = 0;
                ctx.transfer_to_index = 1;
                if (ctx.transaction.data.cryptoTransfer.transfers.accountAmounts[0].amount > 0) {
                    ctx.transfer_from_index = 1;
                    ctx.transfer_to_index = 0;
                }

                iost_snprintf(
                    ctx.senders,
                    DISPLAY_SIZE * 2,
                    "%llu.%llu.%llu",
                    ctx.transaction.data.cryptoTransfer.transfers.accountAmounts[ctx.transfer_from_index].accountID.shardNum,
                    ctx.transaction.data.cryptoTransfer.transfers.accountAmounts[ctx.transfer_from_index].accountID.realmNum,
                    ctx.transaction.data.cryptoTransfer.transfers.accountAmounts[ctx.transfer_from_index].accountID.accountNum
                );
                iost_snprintf(
                    ctx.recipients,
                    DISPLAY_SIZE * 2,
                    "%llu.%llu.%llu",
                    ctx.transaction.data.cryptoTransfer.transfers.accountAmounts[ctx.transfer_to_index].accountID.shardNum,
                    ctx.transaction.data.cryptoTransfer.transfers.accountAmounts[ctx.transfer_to_index].accountID.realmNum,
                    ctx.transaction.data.cryptoTransfer.transfers.accountAmounts[ctx.transfer_to_index].accountID.accountNum
                );
                iost_snprintf(
                    ctx.amount,
                    DISPLAY_SIZE * 2,
                    "%s hbar",
                    hedera_format_tinybar(ctx.transaction.data.cryptoTransfer.transfers.accountAmounts[ctx.transfer_to_index].amount)
                );
            }
        } break;

        default:
            // Unsupported
            THROW(EXCEPTION_MALFORMED_APDU);
            break;
    }

    switch (ctx.type) {
        case Verify:
            ux_flow_init(0, ux_verify_flow, NULL);
            break;
        case Create:
            ux_flow_init(0, ux_create_flow, NULL);
            break;
        case Transfer:
            ux_flow_init(0, ux_transfer_flow, NULL);
            break;
    }
}
#endif

void read_message(
    const uint8_t* const buffer,
    const uint16_t buffer_length
) {
    if (context.msg_length + buffer_length > MAX_MSG_SIZE) {
        PRINTF("invalid message size %u + %u > %u\n", context.msg_length, buffer_length, MAX_MSG_SIZE);
        THROW(SW_WRONG_LENGTH);
    }

    PRINTF("Reading %u bytes\n", buffer_length);
    if (buffer_length != 0) {
        os_memmove(context.msg_body + context.msg_length, buffer, buffer_length);
        context.msg_length += buffer_length;
    }
}

void sign_message(
    const int hash_before_sign,
    uint8_t* signature
) {
    if (context.msg_length == 0) {
        PRINTF("empty message\n");
        THROW(SW_WRONG_LENGTH);
    }
    if (context.msg_length > MAX_MSG_SIZE) {
        PRINTF("invalid message size %u > %u\n", context.msg_length, MAX_MSG_SIZE);
        THROW(SW_WRONG_LENGTH);
    }

    // Read BIP32 path
    uint32_t bip_32_path[BIP32_PATH_LENGTH] = {};
    const uint16_t bip_32_length = io_read_bip32(context.msg_body, context.msg_length, bip_32_path);
    const uint16_t msg_offset = 1 + bip_32_length * sizeof(*bip_32_path);
    uint8_t* msg_body = context.msg_body + msg_offset;

    context.msg_length -= msg_offset;

    if (hash_before_sign != 0) {
        context.msg_length = iost_hash_bytes(msg_body, context.msg_length, msg_body);
    }

    context.signature_length = iost_sign(
        bip_32_path,
        bip_32_length,
        msg_body,
        context.msg_length,
        signature
    );
}

// Sign Handler
// Decodes and handles transaction message
void handle_sign_message(
    const uint8_t p1,
    const uint8_t p2,
    const uint8_t* const buffer,
    const uint16_t buffer_length,
    volatile uint8_t* flags,
    volatile uint16_t* tx
) {
    read_message(buffer, buffer_length);

    if ((p2 & P2_MORE) != P2_MORE || buffer_length == 0) {
        sign_message(buffer_length == 0, G_io_apdu_buffer);
    } else {
        // Wait for next call
        THROW(SW_OK);
    }

//    // Make in memory buffer into stream
//    pb_istream_t stream = pb_istream_from_buffer(
//        raw_transaction,
//        raw_transaction_length
//    );

//    // Decode the Transaction
//    if (!pb_decode(
//        &stream,
//        HederaTransactionBody_fields,
//        &ctx.transaction
//    )) {
//        // Oh no couldn't ...
//        THROW(EXCEPTION_MALFORMED_APDU);
//    }
//    handle_transaction_body();

    if (p1 != P1_CONFIRM) {
        *tx += context.signature_length + 1;
        PRINTF("Write out (%u): %.*H", *tx, *tx, G_io_apdu_buffer);
        clear_context_sign_message();
        THROW(SW_OK);
    }

#if defined(TARGET_NANOS)
    UX_DISPLAY(ui_tx_confirm_step, NULL);
#endif

    *flags |= IO_ASYNCH_REPLY;
}
void clear_context_sign_message()
{
    os_memset(&context, 0, sizeof(context));
}
