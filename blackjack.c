/**
 * Blackjack for Flipper Zero
 * Classic Hit/Stand vs the dealer. OK=Hit, Back=Stand. Back on result screen exits.
 */
#include <furi.h>
#include <gui/gui.h>
#include <gui/view.h>
#include <gui/view_dispatcher.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TAG "blackjack"
#define MAX_HAND 6
#define DECK_SIZE 52
#define CARD_VALUE(c) ((c) % 13)   /* 0=2, 1=3, ..., 9=10, 10=J, 11=Q, 12=K, (12 or 0 for A in rank) */
#define CARD_SUIT(c) ((c) / 13)    /* 0=S, 1=H, 2=D, 3=C */

typedef enum {
    PhaseBetting,
    PhaseDeal,
    PhasePlayerTurn,
    PhaseDealerTurn,
    PhaseShowFinalCards,
    PhaseResult
} GamePhase;

#define STARTING_BALANCE 3125
#define MIN_BET 5
#define MAX_BET 500
#define BET_INCREMENT 5

typedef struct {
    uint8_t deck[DECK_SIZE];
    uint8_t deck_top;
    uint8_t player_hand[MAX_HAND];
    uint8_t player_count;
    uint8_t dealer_hand[MAX_HAND];
    uint8_t dealer_count;
    bool dealer_hole; /* first dealer card hidden until stand */
    GamePhase phase;
    char result_msg[32];
    uint16_t balance; /* player balance in dollars */
    uint16_t current_bet; /* current bet amount */
    bool is_blackjack; /* true if player got blackjack */
} BlackjackState;

/* Best hand value (Ace 1 or 11). Returns 0-31; >21 is bust. */
static uint8_t hand_value(const uint8_t* hand, uint8_t count) {
    uint8_t total = 0;
    uint8_t aces = 0;
    for(uint8_t i = 0; i < count; i++) {
        uint8_t v = hand[i] % 13;
        if(v >= 9) total += 10; /* 10, J, Q, K */
        else if(v == 12) { total += 11; aces++; } /* A */
        else total += v + 2; /* 2..9 */
    }
    while(total > 21 && aces) {
        total -= 10;
        aces--;
    }
    return total;
}

static void shuffle_deck(uint8_t* deck) {
    for(int i = 0; i < DECK_SIZE; i++) deck[i] = (uint8_t)i;
    for(int i = DECK_SIZE - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        uint8_t t = deck[i];
        deck[i] = deck[j];
        deck[j] = t;
    }
}

static uint8_t draw_card(BlackjackState* s) {
    if(s->deck_top >= DECK_SIZE) {
        shuffle_deck(s->deck);
        s->deck_top = 0;
    }
    return s->deck[s->deck_top++];
}

static const char* card_rank_str(uint8_t card) {
    static const char* r[] = { "2","3","4","5","6","7","8","9","10","J","Q","K","A" };
    return r[card % 13];
}

static char card_suit_char(uint8_t card) {
    static const char suits[] = { 'S','H','D','C' };
    return suits[card / 13];
}

/* Draw a card graphic (16x22px) - cards overlap, black with white text */
static void draw_card_graphic(Canvas* canvas, int x, int y, uint8_t card, bool hidden) {
    if(hidden) {
        /* Draw card back (filled rectangle) */
        canvas_draw_box(canvas, x, y, 16, 22);
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_dot(canvas, x + 3, y + 5);
        canvas_draw_dot(canvas, x + 12, y + 16);
        canvas_set_color(canvas, ColorBlack);
    } else {
        /* Draw card front (filled black rectangle) */
        canvas_draw_box(canvas, x, y, 16, 22);
        /* Draw rank and suit together in white */
        canvas_set_color(canvas, ColorWhite);
        canvas_set_font(canvas, FontPrimary);
        char card_str[4];
        snprintf(card_str, sizeof(card_str), "%s%c", card_rank_str(card), card_suit_char(card));
        canvas_draw_str(canvas, x + 2, y + 14, card_str);
        canvas_set_color(canvas, ColorBlack);
    }
}

/* Draw chip stack that grows with bet amount (1 chip per $25) */
static void draw_chip_stack(Canvas* canvas, int x, int y, uint16_t bet_amount) {
    uint8_t chip_count = (bet_amount / 25) + 1; /* At least 1 chip, +1 per $25 */
    if(chip_count > 8) chip_count = 8; /* Max 8 chips visible */
    
    for(uint8_t i = 0; i < chip_count; i++) {
        int chip_x = x + (i * 6); /* Space chips 6px apart */
        /* Draw overlapping circles to show stack */
        canvas_draw_circle(canvas, chip_x, y, 4);
        canvas_draw_circle(canvas, chip_x, y + 1, 4);
        canvas_draw_circle(canvas, chip_x, y + 2, 4);
        /* Add center dot */
        canvas_draw_dot(canvas, chip_x, y + 1);
    }
}

/* Forward declarations */
static void game_deal_cards(BlackjackState* s);

static void game_start_betting(BlackjackState* s) {
    s->phase = PhaseBetting;
    s->current_bet = MIN_BET;
    if(s->current_bet > s->balance) {
        s->current_bet = (s->balance < MIN_BET) ? 0 : s->balance;
    }
    s->result_msg[0] = '\0';
    s->is_blackjack = false;
}

static void game_place_bet(BlackjackState* s) {
    if(s->current_bet == 0 || s->current_bet > s->balance) return;
    s->balance -= s->current_bet;
    game_deal_cards(s);
}

static void game_deal_cards(BlackjackState* s) {
    shuffle_deck(s->deck);
    s->deck_top = 0;
    s->player_count = 0;
    s->dealer_count = 0;
    s->dealer_hole = true;
    s->phase = PhaseDeal;

    s->player_hand[s->player_count++] = draw_card(s);
    s->dealer_hand[s->dealer_count++] = draw_card(s);
    s->player_hand[s->player_count++] = draw_card(s);
    s->dealer_hand[s->dealer_count++] = draw_card(s);

    uint8_t pv = hand_value(s->player_hand, s->player_count);
    if(pv == 21) {
        s->is_blackjack = true;
        s->dealer_hole = false;
        s->phase = PhaseShowFinalCards;
        uint8_t dv = hand_value(s->dealer_hand, s->dealer_count);
        if(dv == 21) {
            s->balance += s->current_bet; /* return bet on push */
            snprintf(s->result_msg, sizeof(s->result_msg), "Push. +$%u", s->current_bet);
        } else {
            /* Blackjack pays 3:2 */
            uint16_t payout = s->current_bet + (s->current_bet * 3 / 2);
            s->balance += payout;
            snprintf(s->result_msg, sizeof(s->result_msg), "Blackjack! +$%u", payout);
        }
        s->result_msg[sizeof(s->result_msg) - 1] = '\0';
    } else {
        s->phase = PhasePlayerTurn;
    }
}

static void game_new_round(BlackjackState* s) {
    if(s->balance == 0) {
        /* Reset balance if broke */
        s->balance = STARTING_BALANCE;
    }
    game_start_betting(s);
}

static void game_player_hit(BlackjackState* s) {
    if(s->phase != PhasePlayerTurn || s->player_count >= MAX_HAND) return;
    s->player_hand[s->player_count++] = draw_card(s);
    uint8_t pv = hand_value(s->player_hand, s->player_count);
    if(pv > 21) {
        s->dealer_hole = false; /* Show dealer cards */
        s->phase = PhaseShowFinalCards;
        /* Result will be calculated in game_show_result */
    } else if(s->player_count == MAX_HAND) {
        /* Player wins with 6 cards without busting */
        s->dealer_hole = false; /* Show dealer cards */
        s->phase = PhaseShowFinalCards;
        /* Result will be calculated in game_show_result */
    }
}

static void game_player_stand(BlackjackState* s) {
    if(s->phase != PhasePlayerTurn) return;
    s->phase = PhaseDealerTurn;
    s->dealer_hole = false;

    uint8_t dv = hand_value(s->dealer_hand, s->dealer_count);
    while(dv < 17 && s->dealer_count < MAX_HAND) {
        s->dealer_hand[s->dealer_count++] = draw_card(s);
        dv = hand_value(s->dealer_hand, s->dealer_count);
        /* Dealer busts if they draw 6 cards without winning */
        if(s->dealer_count == MAX_HAND && dv <= 21) {
            /* Dealer has 6 cards but didn't win - they bust */
            dv = 22; /* Force bust condition */
            break;
        }
    }

    /* Show final cards before result */
    s->phase = PhaseShowFinalCards;
}

static void game_show_result(BlackjackState* s) {
    uint8_t pv = hand_value(s->player_hand, s->player_count);
    uint8_t dv = hand_value(s->dealer_hand, s->dealer_count);
    
    if(pv > 21) {
        /* Player busted */
        snprintf(s->result_msg, sizeof(s->result_msg), "Bust! -$%u", s->current_bet);
    } else if(s->player_count == MAX_HAND) {
        /* Player wins with 6 cards */
        uint16_t payout = s->current_bet * 2;
        s->balance += payout;
        snprintf(s->result_msg, sizeof(s->result_msg), "6 cards! +$%u", payout);
    } else if(dv > 21) {
        uint16_t payout = s->current_bet * 2;
        s->balance += payout;
        if(s->dealer_count == MAX_HAND) {
            snprintf(s->result_msg, sizeof(s->result_msg), "Dealer 6 cards! +$%u", payout);
        } else {
            snprintf(s->result_msg, sizeof(s->result_msg), "Dealer bust! +$%u", payout);
        }
    } else if(pv > dv) {
        uint16_t payout = s->current_bet * 2;
        s->balance += payout;
        snprintf(s->result_msg, sizeof(s->result_msg), "You win! +$%u", payout);
    } else if(pv < dv) {
        snprintf(s->result_msg, sizeof(s->result_msg), "Dealer wins. -$%u", s->current_bet);
    } else {
        s->balance += s->current_bet; /* return bet on push */
        snprintf(s->result_msg, sizeof(s->result_msg), "Push. +$%u", s->current_bet);
    }
    s->result_msg[sizeof(s->result_msg) - 1] = '\0';
    s->phase = PhaseResult;
}

static void draw_callback(Canvas* canvas, void* model) {
    BlackjackState* s = (BlackjackState*)model;
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);

    /* Always show balance at top */
    char bal_buf[16];
    snprintf(bal_buf, sizeof(bal_buf), "$%u", s->balance);
    canvas_draw_str(canvas, 0, 10, bal_buf);

    if(s->phase == PhaseBetting) {
        /* Betting phase - more space for bet amount and chips */
        canvas_set_font(canvas, FontPrimary);
        char bet_buf[20];
        snprintf(bet_buf, sizeof(bet_buf), "Bet: $%u", s->current_bet);
        canvas_draw_str(canvas, 0, 28, bet_buf);
        /* Draw chip stack that grows with bet amount - positioned after bet text */
        int chip_x = canvas_string_width(canvas, bet_buf) + 8;
        draw_chip_stack(canvas, chip_x, 24, s->current_bet);
        /* Help text in footer - double stacked with more spacing */
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 0, 54, "Left/Right=Adjust");
        canvas_draw_str(canvas, 0, 62, "OK=Deal  Back=Exit");
    } else if(s->phase == PhaseDeal || s->phase == PhasePlayerTurn || s->phase == PhaseDealerTurn || s->phase == PhaseShowFinalCards || s->phase == PhaseResult) {
        /* Show bet amount on right side */
        canvas_set_font(canvas, FontSecondary);
        char bet_buf[20];
        snprintf(bet_buf, sizeof(bet_buf), "Bet:$%u", s->current_bet);
        int bet_width = canvas_string_width(canvas, bet_buf);
        canvas_draw_str(canvas, 128 - bet_width, 10, bet_buf);

        /* Draw dealer cards - left side, overlay cards 3+ on bottom half */
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 0, 26, "D:");
        int card_x = 18;
        int card_y = 18;
        for(uint8_t i = 0; i < s->dealer_count; i++) {
            int col = i % 2; /* Column: 0 or 1 */
            int x = card_x + (col * 18); /* 16px card + 2px gap = 18px */
            int y;
            if(i < 2) {
                /* First two cards: normal position */
                int row = i / 2; /* Row: 0 or 1 */
                y = card_y + (row * 24); /* 22px card + 2px gap = 24px */
            } else {
                /* Cards 3+: overlay on bottom half of card above */
                int base_row = (i - 2) / 2; /* Which pair of cards (0, 1, 2) */
                int base_y = card_y + (base_row * 24);
                y = base_y + 11; /* Overlay on bottom half (card is 22px, so +11px) */
            }
            draw_card_graphic(canvas, x, y, s->dealer_hand[i], (i == 0 && s->dealer_hole));
        }
        /* Draw dealer hand value directly underneath "D:" label */
        if(!s->dealer_hole && s->dealer_count > 0) {
            uint8_t dv = hand_value(s->dealer_hand, s->dealer_count);
            char vbuf[8];
            snprintf(vbuf, sizeof(vbuf), "%u", dv);
            canvas_set_font(canvas, FontSecondary);
            canvas_draw_str(canvas, 0, 34, vbuf);
        }

        /* Draw player cards - right side, overlay cards 3+ on bottom half */
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 54, 26, "P:"); /* Moved left to avoid covering first card */
        card_x = 70; /* Start on right side */
        card_y = 18;
        for(uint8_t i = 0; i < s->player_count; i++) {
            int col = i % 2; /* Column: 0 or 1 */
            int x = card_x + (col * 18); /* 16px card + 2px gap = 18px */
            int y;
            if(i < 2) {
                /* First two cards: normal position */
                int row = i / 2; /* Row: 0 or 1 */
                y = card_y + (row * 24); /* 22px card + 2px gap = 24px */
            } else {
                /* Cards 3+: overlay on bottom half of card above */
                int base_row = (i - 2) / 2; /* Which pair of cards (0, 1, 2) */
                int base_y = card_y + (base_row * 24);
                y = base_y + 11; /* Overlay on bottom half (card is 22px, so +11px) */
            }
            draw_card_graphic(canvas, x, y, s->player_hand[i], false);
        }
        /* Draw player hand value directly underneath "P:" label */
        uint8_t pv = hand_value(s->player_hand, s->player_count);
        char vbuf[8];
        snprintf(vbuf, sizeof(vbuf), "%u", pv);
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 54, 34, vbuf);

        /* Help text in footer */
        canvas_set_font(canvas, FontSecondary);
        if(s->phase == PhasePlayerTurn) {
            canvas_draw_str(canvas, 0, 62, "OK=Hit  Back=Stand");
        } else if(s->phase == PhaseShowFinalCards) {
            canvas_draw_str(canvas, 0, 62, "OK=Show Result");
        } else if(s->phase == PhaseResult) {
            /* Draw result box overlay - white box with black outline */
            int box_x = 20;
            int box_y = 20;
            int box_w = 88;
            int box_h = 28;
            /* White box */
            canvas_set_color(canvas, ColorWhite);
            canvas_draw_box(canvas, box_x, box_y, box_w, box_h);
            /* Black outline */
            canvas_set_color(canvas, ColorBlack);
            canvas_draw_frame(canvas, box_x, box_y, box_w, box_h);
            /* Result text inside box */
            canvas_set_font(canvas, FontPrimary);
            uint8_t pv_final = hand_value(s->player_hand, s->player_count);
            uint8_t dv_final = hand_value(s->dealer_hand, s->dealer_count);
            char result_text[32];
            snprintf(result_text, sizeof(result_text), "P:%u D:%u", pv_final, dv_final);
            int text_x = box_x + (box_w - canvas_string_width(canvas, result_text)) / 2;
            canvas_draw_str(canvas, text_x, box_y + 10, result_text);
            /* Result message centered */
            int msg_x = box_x + (box_w - canvas_string_width(canvas, s->result_msg)) / 2;
            canvas_draw_str(canvas, msg_x, box_y + 22, s->result_msg);
            /* Footer instructions */
            canvas_set_font(canvas, FontSecondary);
            canvas_draw_str(canvas, 0, 56, "Left=Change Bet");
            canvas_draw_str(canvas, 0, 62, "OK=Bet Again  Back=Exit");
        }
    }
}

typedef struct {
    View* view;
    ViewDispatcher* view_dispatcher;
} BlackjackApp;

static bool input_callback(InputEvent* event, void* context) {
    BlackjackApp* app = (BlackjackApp*)context;
    if(event->type != InputTypePress) return false;

    BlackjackState* s = (BlackjackState*)view_get_model(app->view);
    if(!s) return false;

    if(event->key == InputKeyBack) {
        if(s->phase == PhaseResult || s->phase == PhaseBetting) {
            view_commit_model(app->view, false);
            view_dispatcher_stop(app->view_dispatcher);
            return true;
        }
        if(s->phase == PhasePlayerTurn) {
            game_player_stand(s);
            view_commit_model(app->view, true);
            return true;
        }
        view_commit_model(app->view, false);
        return false;
    }
    if(event->key == InputKeyOk) {
        if(s->phase == PhaseBetting) {
            game_place_bet(s);
            view_commit_model(app->view, true);
            return true;
        }
        if(s->phase == PhaseShowFinalCards) {
            game_show_result(s);
            view_commit_model(app->view, true);
            return true;
        }
        if(s->phase == PhaseResult) {
            /* Bet Again - deal new hand with same bet */
            game_deal_cards(s);
            view_commit_model(app->view, true);
            return true;
        }
        if(s->phase == PhasePlayerTurn) {
            game_player_hit(s);
            view_commit_model(app->view, true);
            return true;
        }
    }
    if(event->key == InputKeyLeft) {
        if(s->phase == PhaseBetting) {
            if(s->current_bet > MIN_BET) {
                s->current_bet -= BET_INCREMENT;
                if(s->current_bet < MIN_BET) s->current_bet = MIN_BET;
            }
            view_commit_model(app->view, true);
            return true;
        }
        if(s->phase == PhaseResult) {
            /* Change Bet - go back to betting screen */
            game_start_betting(s);
            view_commit_model(app->view, true);
            return true;
        }
    }
    if(event->key == InputKeyRight) {
        if(s->phase == PhaseBetting) {
            uint16_t max_bet = (s->balance < MAX_BET) ? s->balance : MAX_BET;
            if(s->current_bet < max_bet) {
                s->current_bet += BET_INCREMENT;
                if(s->current_bet > max_bet) s->current_bet = max_bet;
            }
            view_commit_model(app->view, true);
            return true;
        }
    }
    view_commit_model(app->view, false);
    return false;
}

int32_t blackjack_app(void* p) {
    UNUSED(p);
    FURI_LOG_I(TAG, "Blackjack start");

    BlackjackApp* app = malloc(sizeof(BlackjackApp));
    memset(app, 0, sizeof(BlackjackApp));
    app->view_dispatcher = view_dispatcher_alloc();
    app->view = view_alloc();

    view_allocate_model(app->view, ViewModelTypeLocking, sizeof(BlackjackState));
    view_set_draw_callback(app->view, draw_callback);
    view_set_context(app->view, app);
    view_set_input_callback(app->view, input_callback);

    BlackjackState* state = (BlackjackState*)view_get_model(app->view);
    state->balance = STARTING_BALANCE;
    state->current_bet = 0;
    game_new_round(state);
    view_commit_model(app->view, true);

    view_dispatcher_add_view(app->view_dispatcher, 0, app->view);
    view_dispatcher_switch_to_view(app->view_dispatcher, 0);

    Gui* gui = furi_record_open(RECORD_GUI);
    view_dispatcher_attach_to_gui(app->view_dispatcher, gui, ViewDispatcherTypeFullscreen);

    view_dispatcher_run(app->view_dispatcher);

    furi_record_close(RECORD_GUI);
    view_dispatcher_remove_view(app->view_dispatcher, 0);
    view_free(app->view);
    view_dispatcher_free(app->view_dispatcher);
    free(app);
    FURI_LOG_I(TAG, "Blackjack exit");
    return 0;
}
