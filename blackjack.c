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
#define MAX_HAND 12
#define DECK_SIZE 52
#define CARD_VALUE(c) ((c) % 13)   /* 0=2, 1=3, ..., 9=10, 10=J, 11=Q, 12=K, (12 or 0 for A in rank) */
#define CARD_SUIT(c) ((c) / 13)    /* 0=S, 1=H, 2=D, 3=C */

typedef enum {
    PhaseDeal,
    PhasePlayerTurn,
    PhaseDealerTurn,
    PhaseResult
} GamePhase;

typedef struct {
    uint8_t deck[DECK_SIZE];
    uint8_t deck_top;
    uint8_t player_hand[MAX_HAND];
    uint8_t player_count;
    uint8_t dealer_hand[MAX_HAND];
    uint8_t dealer_count;
    bool dealer_hole; /* first dealer card hidden until stand */
    GamePhase phase;
    char result_msg[24];
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

static void game_new_round(BlackjackState* s) {
    shuffle_deck(s->deck);
    s->deck_top = 0;
    s->player_count = 0;
    s->dealer_count = 0;
    s->dealer_hole = true;
    s->phase = PhaseDeal;
    s->result_msg[0] = '\0';

    s->player_hand[s->player_count++] = draw_card(s);
    s->dealer_hand[s->dealer_count++] = draw_card(s);
    s->player_hand[s->player_count++] = draw_card(s);
    s->dealer_hand[s->dealer_count++] = draw_card(s);

    uint8_t pv = hand_value(s->player_hand, s->player_count);
    if(pv == 21) {
        s->dealer_hole = false;
        uint8_t dv = hand_value(s->dealer_hand, s->dealer_count);
        if(dv == 21)
            strncpy(s->result_msg, "Push.", sizeof(s->result_msg) - 1);
        else
            strncpy(s->result_msg, "Blackjack! You win!", sizeof(s->result_msg) - 1);
        s->result_msg[sizeof(s->result_msg) - 1] = '\0';
        s->phase = PhaseResult;
    } else {
        s->phase = PhasePlayerTurn;
    }
}

static void game_player_hit(BlackjackState* s) {
    if(s->phase != PhasePlayerTurn || s->player_count >= MAX_HAND) return;
    s->player_hand[s->player_count++] = draw_card(s);
    if(hand_value(s->player_hand, s->player_count) > 21) {
        s->phase = PhaseResult;
        strncpy(s->result_msg, "Bust! Dealer wins.", sizeof(s->result_msg) - 1);
        s->result_msg[sizeof(s->result_msg) - 1] = '\0';
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
    }

    uint8_t pv = hand_value(s->player_hand, s->player_count);
    if(dv > 21) {
        strncpy(s->result_msg, "Dealer bust! You win!", sizeof(s->result_msg) - 1);
    } else if(pv > dv) {
        strncpy(s->result_msg, "You win!", sizeof(s->result_msg) - 1);
    } else if(pv < dv) {
        strncpy(s->result_msg, "Dealer wins.", sizeof(s->result_msg) - 1);
    } else {
        strncpy(s->result_msg, "Push.", sizeof(s->result_msg) - 1);
    }
    s->result_msg[sizeof(s->result_msg) - 1] = '\0';
    s->phase = PhaseResult;
}

static void draw_callback(Canvas* canvas, void* model) {
    BlackjackState* s = (BlackjackState*)model;
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);

    if(s->phase == PhaseDeal || s->phase == PhasePlayerTurn || s->phase == PhaseDealerTurn || s->phase == PhaseResult) {
        canvas_draw_str(canvas, 0, 8, "D:");
        int x = 14;
        for(uint8_t i = 0; i < s->dealer_count; i++) {
            if(i == 0 && s->dealer_hole) {
                canvas_draw_str(canvas, x, 8, "??");
            } else {
                char buf[4];
                snprintf(buf, sizeof(buf), "%s%c", card_rank_str(s->dealer_hand[i]), card_suit_char(s->dealer_hand[i]));
                canvas_draw_str(canvas, x, 8, buf);
            }
            x += 22;
        }
        if(!s->dealer_hole && s->dealer_count > 0) {
            uint8_t dv = hand_value(s->dealer_hand, s->dealer_count);
            char vbuf[8];
            snprintf(vbuf, sizeof(vbuf), "(%u)", dv);
            canvas_draw_str(canvas, x, 8, vbuf);
        }

        canvas_draw_str(canvas, 0, 22, "P:");
        x = 14;
        for(uint8_t i = 0; i < s->player_count; i++) {
            char buf[4];
            snprintf(buf, sizeof(buf), "%s%c", card_rank_str(s->player_hand[i]), card_suit_char(s->player_hand[i]));
            canvas_draw_str(canvas, x, 22, buf);
            x += 22;
        }
        uint8_t pv = hand_value(s->player_hand, s->player_count);
        char vbuf[8];
        snprintf(vbuf, sizeof(vbuf), " (%u)", pv);
        canvas_draw_str(canvas, x, 22, vbuf);
    }

    if(s->phase == PhasePlayerTurn) {
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 0, 42, "OK=Hit  Back=Stand");
    } else if(s->phase == PhaseResult) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 0, 42, s->result_msg);
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 0, 54, "OK=New game  Back=Exit");
    } else if(s->phase == PhaseDeal) {
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 0, 42, "OK=New game  Back=Exit");
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
        if(s->phase == PhaseResult || s->phase == PhaseDeal) {
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
        if(s->phase == PhaseResult || s->phase == PhaseDeal) {
            game_new_round(s);
            view_commit_model(app->view, true);
            return true;
        }
        if(s->phase == PhasePlayerTurn) {
            game_player_hit(s);
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
