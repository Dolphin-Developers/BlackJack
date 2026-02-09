/**
 * Blackjack for Flipper Zero
 * Classic Hit/Stand vs the dealer. OK=Hit, Back=Stand. Back on result screen exits.
 * Statistics: Press Right on result screen to view win/loss stats.
 */
#include <furi.h>
#include <gui/gui.h>
#include <gui/view.h>
#include <gui/view_dispatcher.h>
#include <storage/storage.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TAG "blackjack"
#define MAX_HAND 6
#define MAX_SPLIT_HANDS 2  /* Maximum number of hands after split */
#define DECKS 3
#define DECK_SIZE (52 * DECKS)  /* 3 deck shoe = 156 cards */
#define BURN_TOP 1  /* Burn top card */
#define BURN_BOTTOM 20  /* Burn bottom 20 cards */
#define CARD_VALUE(c) ((c) % 13)   /* 0=2, 1=3, ..., 9=10, 10=J, 11=Q, 12=K, (12 or 0 for A in rank) */
#define CARD_SUIT(c) ((c) / 13)    /* 0=S, 1=H, 2=D, 3=C */
#define CARD_RANK(c) ((c) % 13)    /* Get card rank (0-12) for pair detection */

typedef enum {
    PhaseSplash,
    PhaseProfileMenu,
    PhaseBetting,
    PhaseDeal,
    PhasePlayerTurn,
    PhaseSplitPrompt,
    PhaseDealerTurn,
    PhaseShowFinalCards,
    PhaseResult,
    PhaseStatistics,
    PhaseHelp,
    PhaseReshuffle,
    PhaseGuestSavePrompt,  /* Guest: Save to profile? Yes/No */
    PhaseGuestPickProfile  /* Guest: pick slot to save to */
} GamePhase;

#define STARTING_BALANCE 3125
#define MIN_BET 5
#define MAX_BET 500
#define BET_INCREMENT 5
#define MAX_PROFILES 4
#define PROFILE_NAME_LEN 17
#define BLACKJACK_PROFILES_PATH EXT_PATH("apps_data/blackjack/profiles.dat")
#define BLACKJACK_LAST_USED_PATH EXT_PATH("apps_data/blackjack/last_used")
#define PROFILES_FILE_MAGIC "BJ1"
#define SPLASH_OPTIONS 5  /* Continue, New profile, Guest, Practice, Help */

typedef struct {
    uint8_t deck[DECK_SIZE];
    uint8_t deck_top;
    uint8_t deck_bottom; /* Track position for burning bottom cards */
    bool reshuffle_announced; /* Track if reshuffle was announced */
    uint8_t player_hand[MAX_HAND];
    uint8_t player_count;
    uint8_t player_hand2[MAX_HAND]; /* Second hand after split */
    uint8_t player_count2;
    uint8_t dealer_hand[MAX_HAND];
    uint8_t dealer_count;
    bool dealer_hole; /* first dealer card hidden until stand */
    GamePhase phase;
    char result_msg[32];
    uint16_t balance; /* player balance in dollars */
    uint16_t current_bet; /* current bet amount */
    uint16_t bet_hand2; /* bet for second hand after split */
    bool is_blackjack; /* true if player got blackjack */
    bool can_double_down; /* true if player can double down (first 2 cards, enough balance) */
    bool can_split; /* true if player can split (first 2 cards are pair, enough balance) */
    bool is_split; /* true if hands are split */
    uint8_t active_hand; /* 0 = first hand, 1 = second hand */
    GamePhase prev_phase; /* Previous phase before help screen */
    /* Statistics tracking */
    uint16_t games_played;
    uint16_t games_won;
    uint16_t games_lost;
    uint16_t games_pushed;
    uint8_t stat_scroll; /* Scroll offset for stats menu (0 = top, loops) */
    uint8_t profile_menu_selection;
    uint8_t current_profile_slot;
    char profile_names[MAX_PROFILES][PROFILE_NAME_LEN];
    uint8_t splash_selection;   /* 0=Continue, 1=New, 2=Guest, 3=Practice, 4=Help */
    bool is_guest;
    bool practice_mode;
} BlackjackState;

#define STAT_LINES 5   /* Games, Wins, Losses, Pushes, Win Rate */
#define STAT_VISIBLE 3 /* Lines visible at once */
#define STAT_MAX_SCROLL (STAT_LINES > STAT_VISIBLE ? STAT_LINES - STAT_VISIBLE : 0)

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

/* Get soft value (with ace as 11) and hard value (with ace as 1) */
static void hand_values_soft_hard(const uint8_t* hand, uint8_t count, uint8_t* soft, uint8_t* hard) {
    uint8_t total = 0;
    uint8_t aces = 0;
    for(uint8_t i = 0; i < count; i++) {
        uint8_t v = hand[i] % 13;
        if(v >= 9) total += 10; /* 10, J, Q, K */
        else if(v == 12) { total += 11; aces++; } /* A */
        else total += v + 2; /* 2..9 */
    }
    *soft = total;
    *hard = total;
    if(aces > 0 && total <= 21) {
        *hard = total - 10; /* Hard value (ace as 1) */
    }
    /* If soft > 21, adjust */
    while(*soft > 21 && aces) {
        *soft -= 10;
        aces--;
    }
}

/* Wizard of Odds basic strategy hint (simplified). Returns recommended action. */
static const char* wizard_strategy_hint(const uint8_t* hand, uint8_t count, uint8_t dealer_card, bool can_double, bool can_split) {
    uint8_t total = hand_value(hand, count);
    uint8_t soft = 0, hard = 0;
    hand_values_soft_hard(hand, count, &soft, &hard);
    bool is_soft = (soft != hard && soft <= 21);
    bool is_pair = (count == 2 && CARD_RANK(hand[0]) == CARD_RANK(hand[1]));
    uint8_t r = dealer_card % 13;
    int d = (r >= 8 && r <= 11) ? 10 : (r == 12) ? 11 : (r + 2);
    if(is_pair && can_split) {
        if(CARD_RANK(hand[0]) == 12) return "Split";   /* A,A */
        if(CARD_RANK(hand[0]) == 9) return "Split";    /* 10,10 - no, stand. 9,9 vs 2-9,7: split */
        if(CARD_RANK(hand[0]) == 8) return "Split";   /* 8,8 */
        if(CARD_RANK(hand[0]) == 7 && d <= 7) return "Split";
        if(CARD_RANK(hand[0]) == 6 && d <= 6) return "Split";
        if(CARD_RANK(hand[0]) == 3 && d <= 7) return "Split";
        if(CARD_RANK(hand[0]) == 2 && d <= 7) return "Split";
    }
    if(is_soft) {
        if(soft >= 19) return "Stand";
        if(soft == 18 && d >= 9) return "Hit";
        if(soft == 18 && d <= 8) return "Stand";
        if(soft == 17 && d >= 3 && can_double) return "Double";
        if(soft >= 15 && soft <= 17 && d >= 4 && d <= 6 && can_double) return "Double";
        if(soft >= 13 && soft <= 17) return "Hit";
        return "Hit";
    }
    if(total >= 17) return "Stand";
    if(total >= 13 && total <= 16 && d <= 6) return "Stand";
    if(total == 12 && d >= 4 && d <= 6) return "Stand";
    if(total == 11 && can_double) return "Double";
    if(total == 10 && d <= 9 && can_double) return "Double";
    if(total == 9 && d >= 3 && d <= 6 && can_double) return "Double";
    if(total <= 11) return "Hit";
    if(total >= 12 && d >= 7) return "Hit";
    return "Stand";
}

static void shuffle_deck(uint8_t* deck) {
    /* Create 3 decks */
    for(int i = 0; i < DECK_SIZE; i++) {
        deck[i] = (uint8_t)(i % 52);
    }
    /* Fisher-Yates shuffle */
    for(int i = DECK_SIZE - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        uint8_t t = deck[i];
        deck[i] = deck[j];
        deck[j] = t;
    }
}

static uint8_t draw_card(BlackjackState* s) {
    /* Check if we've reached the burn zone (bottom 20 cards) */
    if(s->deck_top >= (DECK_SIZE - BURN_BOTTOM)) {
        /* Reshuffle when reaching burn zone */
        shuffle_deck(s->deck);
        s->deck_top = BURN_TOP; /* Burn top card */
        s->deck_bottom = DECK_SIZE - BURN_BOTTOM;
        s->reshuffle_announced = false; /* Need to announce */
    }
    if(s->deck_top >= DECK_SIZE) {
        shuffle_deck(s->deck);
        s->deck_top = BURN_TOP; /* Burn top card */
        s->deck_bottom = DECK_SIZE - BURN_BOTTOM;
        s->reshuffle_announced = false; /* Need to announce */
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
static void game_player_stand(BlackjackState* s);
static void game_player_split(BlackjackState* s);

#pragma pack(push, 1)
typedef struct {
    char name[PROFILE_NAME_LEN];
    uint16_t balance;
    uint16_t games_played;
    uint16_t games_won;
    uint16_t games_lost;
    uint16_t games_pushed;
} ProfileRecord;
#pragma pack(pop)

static void profile_ensure_dir(Storage* storage) {
    storage_simply_mkdir(storage, EXT_PATH("apps_data"));
    storage_simply_mkdir(storage, EXT_PATH("apps_data/blackjack"));
}

static void profile_load_list(Storage* storage, BlackjackState* s) {
    for(uint8_t i = 0; i < MAX_PROFILES; i++) {
        snprintf(s->profile_names[i], PROFILE_NAME_LEN, "Slot %u", (unsigned)(i + 1));
    }
    File* file = storage_file_alloc(storage);
    if(!storage_file_open(file, BLACKJACK_PROFILES_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        storage_file_free(file);
        return;
    }
    char magic[4];
    if(storage_file_read(file, magic, 4) != 4 || memcmp(magic, PROFILES_FILE_MAGIC, 4) != 0) {
        storage_file_close(file);
        storage_file_free(file);
        return;
    }
    ProfileRecord rec;
    for(uint8_t i = 0; i < MAX_PROFILES; i++) {
        if(storage_file_read(file, &rec, sizeof(rec)) != sizeof(rec)) break;
        rec.name[PROFILE_NAME_LEN - 1] = '\0';
        if(rec.name[0] != '\0') {
            strncpy(s->profile_names[i], rec.name, PROFILE_NAME_LEN - 1);
            s->profile_names[i][PROFILE_NAME_LEN - 1] = '\0';
        }
    }
    storage_file_close(file);
    storage_file_free(file);
}

static void profile_load_slot(Storage* storage, BlackjackState* s, uint8_t slot) {
    if(slot >= MAX_PROFILES) return;
    s->current_profile_slot = slot;
    File* file = storage_file_alloc(storage);
    if(!storage_file_open(file, BLACKJACK_PROFILES_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        storage_file_free(file);
        s->balance = STARTING_BALANCE;
        s->games_played = s->games_won = s->games_lost = s->games_pushed = 0;
        return;
    }
    storage_file_seek(file, 4 + (uint32_t)slot * sizeof(ProfileRecord), true);
    ProfileRecord rec;
    if(storage_file_read(file, &rec, sizeof(rec)) != sizeof(rec)) {
        storage_file_close(file);
        storage_file_free(file);
        s->balance = STARTING_BALANCE;
        s->games_played = s->games_won = s->games_lost = s->games_pushed = 0;
        return;
    }
    storage_file_close(file);
    storage_file_free(file);
    s->balance = rec.balance;
    s->games_played = rec.games_played;
    s->games_won = rec.games_won;
    s->games_lost = rec.games_lost;
    s->games_pushed = rec.games_pushed;
    if(s->balance == 0) s->balance = STARTING_BALANCE;
}

static uint8_t profile_load_last_used(Storage* storage) {
    File* file = storage_file_alloc(storage);
    uint8_t slot = 0;
    if(storage_file_open(file, BLACKJACK_LAST_USED_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        if(storage_file_read(file, &slot, 1) == 1 && slot < MAX_PROFILES) { /* valid */ }
        else slot = 0;
        storage_file_close(file);
    }
    storage_file_free(file);
    return slot;
}

static void profile_save_last_used(Storage* storage, uint8_t slot) {
    if(slot >= MAX_PROFILES) return;
    profile_ensure_dir(storage);
    File* file = storage_file_alloc(storage);
    if(storage_file_open(file, BLACKJACK_LAST_USED_PATH, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        storage_file_write(file, &slot, 1);
        storage_file_sync(file);
        storage_file_close(file);
    }
    storage_file_free(file);
}

static void profile_save_current(Storage* storage, BlackjackState* s) {
    profile_ensure_dir(storage);
    File* file = storage_file_alloc(storage);
    if(!storage_file_open(file, BLACKJACK_PROFILES_PATH, FSAM_READ_WRITE, FSOM_OPEN_ALWAYS)) {
        storage_file_free(file);
        return;
    }
    uint64_t size = storage_file_size(file);
    if(size < 4) {
        storage_file_seek(file, 0, true);
        storage_file_write(file, PROFILES_FILE_MAGIC, 4);
        for(uint8_t i = 0; i < MAX_PROFILES; i++) {
            ProfileRecord rec = { .name = {0}, .balance = 0, .games_played = 0, .games_won = 0, .games_lost = 0, .games_pushed = 0 };
            storage_file_write(file, &rec, sizeof(rec));
        }
        storage_file_sync(file);
    }
    ProfileRecord rec;
    memset(rec.name, 0, sizeof(rec.name));
    strncpy(rec.name, s->profile_names[s->current_profile_slot], PROFILE_NAME_LEN - 1);
    rec.balance = s->balance;
    rec.games_played = s->games_played;
    rec.games_won = s->games_won;
    rec.games_lost = s->games_lost;
    rec.games_pushed = s->games_pushed;
    storage_file_seek(file, 4 + (uint32_t)s->current_profile_slot * sizeof(ProfileRecord), true);
    storage_file_write(file, &rec, sizeof(rec));
    storage_file_sync(file);
    storage_file_close(file);
    storage_file_free(file);
    profile_save_last_used(storage, s->current_profile_slot);
}

static void profile_save_guest_to_slot(Storage* storage, BlackjackState* s, uint8_t slot) {
    if(slot >= MAX_PROFILES) return;
    profile_ensure_dir(storage);
    profile_load_list(storage, s);
    if(s->profile_names[slot][0] == '\0') {
        snprintf(s->profile_names[slot], PROFILE_NAME_LEN, "Player %u", (unsigned)(slot + 1));
    }
    s->current_profile_slot = slot;
    profile_save_current(storage, s);
}

static void profile_create_new(BlackjackState* s) {
    uint8_t slot = 0;
    if(s->profile_names[0][0] != '\0') {
        for(slot = 1; slot < MAX_PROFILES; slot++) {
            if(s->profile_names[slot][0] == '\0') break;
        }
        if(slot >= MAX_PROFILES) slot = 0;
    }
    s->current_profile_slot = slot;
    snprintf(s->profile_names[slot], PROFILE_NAME_LEN, "Player %u", (unsigned)(slot + 1));
    s->balance = STARTING_BALANCE;
    s->games_played = s->games_won = s->games_lost = s->games_pushed = 0;
}

static void game_start_betting(BlackjackState* s) {
    s->phase = PhaseBetting;
    s->current_bet = MIN_BET;
    if(s->current_bet > s->balance) {
        s->current_bet = (s->balance < MIN_BET) ? 0 : s->balance;
    }
    s->result_msg[0] = '\0';
    s->is_blackjack = false;
    s->can_double_down = false;
    s->can_split = false;
    s->is_split = false;
    s->active_hand = 0;
    s->bet_hand2 = 0;
    s->player_count2 = 0;
}

static void game_show_statistics(BlackjackState* s) {
    s->phase = PhaseStatistics;
    s->stat_scroll = 0;
}

static void game_show_help(BlackjackState* s) {
    s->prev_phase = s->phase;
    s->phase = PhaseHelp;
}

static void game_place_bet(BlackjackState* s) {
    if(s->current_bet == 0 || s->current_bet > s->balance) return;
    s->balance -= s->current_bet;
    game_deal_cards(s);
}

static void game_deal_cards(BlackjackState* s) {
    shuffle_deck(s->deck);
    s->deck_top = BURN_TOP; /* Burn top card */
    s->deck_bottom = DECK_SIZE - BURN_BOTTOM;
    s->reshuffle_announced = true; /* Already shuffled, no need to announce */
    s->player_count = 0;
    s->dealer_count = 0;
    s->dealer_hole = true;
    s->phase = PhaseDeal;

    s->player_hand[s->player_count++] = draw_card(s);
    s->dealer_hand[s->dealer_count++] = draw_card(s);
    s->player_hand[s->player_count++] = draw_card(s);
    s->dealer_hand[s->dealer_count++] = draw_card(s);
    
    /* Check if reshuffle happened during deal */
    if(!s->reshuffle_announced) {
        s->phase = PhaseReshuffle;
        return;
    }

    uint8_t pv = hand_value(s->player_hand, s->player_count);
    if(pv == 21) {
        /* Player blackjack - reveal dealer and resolve */
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
        /* Dealer peek: when up card is 10 or Ace, check for dealer blackjack (Wizard of Odds standard) */
        uint8_t dealer_up_rank = s->dealer_hand[1] % 13; /* Up card is second card: 9=10, 10=J, 11=Q, 12=K/A */
        bool dealer_has_10_or_ace = (dealer_up_rank >= 9);
        if(dealer_has_10_or_ace) {
            uint8_t dv = hand_value(s->dealer_hand, s->dealer_count);
            if(dv == 21) {
                /* Dealer blackjack - end hand immediately; player loses (no hit/double/split) */
                s->dealer_hole = false;
                s->games_played++;
                s->games_lost++;
                snprintf(s->result_msg, sizeof(s->result_msg), "Dealer blackjack. -$%u", s->current_bet);
                s->result_msg[sizeof(s->result_msg) - 1] = '\0';
                s->phase = PhaseResult;
                return;
            }
        }
        /* No dealer blackjack - check if player can split */
        bool is_pair = (s->player_count == 2 && CARD_RANK(s->player_hand[0]) == CARD_RANK(s->player_hand[1]));
        s->can_split = (is_pair && (s->balance >= s->current_bet));
        if(s->can_split) {
            s->phase = PhaseSplitPrompt;
        } else {
            s->phase = PhasePlayerTurn;
            s->can_double_down = (s->player_count == 2 && (s->balance >= s->current_bet));
        }
    }
}

static void game_player_hit(BlackjackState* s) {
    if(s->phase != PhasePlayerTurn) return;
    
    uint8_t* hand = (s->is_split && s->active_hand == 1) ? s->player_hand2 : s->player_hand;
    uint8_t* count = (s->is_split && s->active_hand == 1) ? &s->player_count2 : &s->player_count;
    
    if(*count >= MAX_HAND) return;
    
    hand[(*count)++] = draw_card(s);
    s->can_double_down = false; /* Can't double after hitting */
    s->can_split = false; /* Can't split after hitting */
    
    uint8_t pv = hand_value(hand, *count);
        if(pv > 21) {
            /* Hand busted - if split, move to next hand or show results */
            if(s->is_split && s->active_hand == 0) {
                /* Move to second hand */
                s->active_hand = 1;
                s->can_double_down = (s->player_count2 == 2 && (s->balance >= s->bet_hand2));
                s->can_split = false; /* Can't split second hand */
            } else {
            /* Both hands done or single hand busted */
            s->dealer_hole = false; /* Show dealer cards */
            s->phase = PhaseShowFinalCards;
        }
    } else if(*count == MAX_HAND) {
        /* Player wins with 6 cards without busting */
        if(s->is_split && s->active_hand == 0) {
            /* Move to second hand */
            s->active_hand = 1;
            s->can_double_down = (s->player_count2 == 2 && (s->balance >= s->bet_hand2));
            s->can_split = false;
        } else {
            s->dealer_hole = false; /* Show dealer cards */
            s->phase = PhaseShowFinalCards;
        }
    }
}

static void game_player_double_down(BlackjackState* s) {
    if(s->phase != PhasePlayerTurn || !s->can_double_down) return;
    
    uint8_t* hand = (s->is_split && s->active_hand == 1) ? s->player_hand2 : s->player_hand;
    uint8_t* count = (s->is_split && s->active_hand == 1) ? &s->player_count2 : &s->player_count;
    uint16_t* bet = (s->is_split && s->active_hand == 1) ? &s->bet_hand2 : &s->current_bet;
    
    if(*count != 2) return;
    if(s->balance >= *bet) {
        s->balance -= *bet;
        *bet *= 2;
    } else {
        /* Not enough balance to double */
        return;
    }
    /* Draw exactly one card */
    hand[(*count)++] = draw_card(s);
    s->can_double_down = false;
    s->can_split = false;
    /* Automatically stand after double down */
    game_player_stand(s);
}

static void game_player_split(BlackjackState* s) {
    if(s->phase != PhasePlayerTurn || !s->can_split) return;
    
    /* Check if we have enough balance for second bet */
    if(s->balance < s->current_bet) return;
    
    /* Place bet for second hand */
    s->balance -= s->current_bet;
    s->bet_hand2 = s->current_bet;
    
    /* Split the pair: second card goes to second hand */
    s->player_hand2[0] = s->player_hand[1];
    s->player_count2 = 1;
    s->player_count = 1; /* First hand now has one card */
    
    /* Deal one card to each hand */
    s->player_hand[s->player_count++] = draw_card(s);
    s->player_hand2[s->player_count2++] = draw_card(s);
    
    s->is_split = true;
    s->active_hand = 0; /* Start with first hand */
    s->can_split = false; /* Can't split again */
    s->can_double_down = (s->player_count == 2 && (s->balance >= s->current_bet));
}

static void game_player_stand(BlackjackState* s) {
    if(s->phase != PhasePlayerTurn) return;
    
    /* If split and on first hand, move to second hand */
    if(s->is_split && s->active_hand == 0) {
        s->active_hand = 1;
        s->can_double_down = (s->player_count2 == 2 && (s->balance >= s->bet_hand2));
        s->can_split = false;
        return;
    }
    
    /* Both hands done, dealer's turn */
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
    /* If blackjack was already handled in game_deal_cards, just track stats */
    if(s->is_blackjack && s->result_msg[0] != '\0') {
        s->games_played++;
        uint8_t dv = hand_value(s->dealer_hand, s->dealer_count);
        if(dv == 21) {
            s->games_pushed++;
        } else {
            s->games_won++;
        }
        s->phase = PhaseResult;
        return;
    }
    
    uint8_t dv = hand_value(s->dealer_hand, s->dealer_count);
    uint16_t total_winnings = 0;
    uint16_t total_losses = 0;
    char result_buf[64] = "";
    
    if(s->is_split) {
        /* Calculate results for both hands */
        uint8_t pv1 = hand_value(s->player_hand, s->player_count);
        uint8_t pv2 = hand_value(s->player_hand2, s->player_count2);
        uint16_t winnings1 = 0, winnings2 = 0;
        bool won1 = false, won2 = false, lost1 = false, lost2 = false;
        
        /* Hand 1 result */
        if(pv1 > 21) {
            lost1 = true;
            total_losses += s->current_bet;
        } else if(s->player_count == MAX_HAND) {
            won1 = true;
            winnings1 = s->current_bet * 2;
            total_winnings += winnings1;
        } else if(dv > 21) {
            won1 = true;
            winnings1 = s->current_bet * 2;
            total_winnings += winnings1;
        } else if(pv1 > dv) {
            won1 = true;
            winnings1 = s->current_bet * 2;
            total_winnings += winnings1;
        } else if(pv1 < dv) {
            lost1 = true;
            total_losses += s->current_bet;
        } else {
            /* Push - return bet */
            total_winnings += s->current_bet;
        }
        
        /* Hand 2 result */
        if(pv2 > 21) {
            lost2 = true;
            total_losses += s->bet_hand2;
        } else if(s->player_count2 == MAX_HAND) {
            won2 = true;
            winnings2 = s->bet_hand2 * 2;
            total_winnings += winnings2;
        } else if(dv > 21) {
            won2 = true;
            winnings2 = s->bet_hand2 * 2;
            total_winnings += winnings2;
        } else if(pv2 > dv) {
            won2 = true;
            winnings2 = s->bet_hand2 * 2;
            total_winnings += winnings2;
        } else if(pv2 < dv) {
            lost2 = true;
            total_losses += s->bet_hand2;
        } else {
            /* Push - return bet */
            total_winnings += s->bet_hand2;
        }
        
        s->balance += total_winnings;
        
        /* Format result message */
        if(total_winnings > total_losses) {
            uint16_t net = total_winnings - total_losses;
            snprintf(result_buf, sizeof(result_buf), "Split: +$%u", net);
        } else if(total_losses > total_winnings) {
            uint16_t net = total_losses - total_winnings;
            snprintf(result_buf, sizeof(result_buf), "Split: -$%u", net);
        } else {
            snprintf(result_buf, sizeof(result_buf), "Split: Push");
        }
        
        /* Track statistics */
        if(won1 && won2) {
            s->games_won++;
        } else if(lost1 && lost2) {
            s->games_lost++;
        } else {
            s->games_pushed++;
        }
    } else {
        /* Single hand result */
        uint8_t pv = hand_value(s->player_hand, s->player_count);
        
        if(pv > 21) {
            /* Player busted */
            s->games_lost++;
            snprintf(result_buf, sizeof(result_buf), "Bust! -$%u", s->current_bet);
        } else if(s->player_count == MAX_HAND) {
            /* Player wins with 6 cards */
            s->games_won++;
            uint16_t payout = s->current_bet * 2;
            s->balance += payout;
            snprintf(result_buf, sizeof(result_buf), "6 cards! +$%u", payout);
        } else if(dv > 21) {
            s->games_won++;
            uint16_t payout = s->current_bet * 2;
            s->balance += payout;
            if(s->dealer_count == MAX_HAND) {
                snprintf(result_buf, sizeof(result_buf), "Dealer 6 cards! +$%u", payout);
            } else {
                snprintf(result_buf, sizeof(result_buf), "Dealer bust! +$%u", payout);
            }
        } else if(pv > dv) {
            s->games_won++;
            uint16_t payout = s->current_bet * 2;
            s->balance += payout;
            snprintf(result_buf, sizeof(result_buf), "You win! +$%u", payout);
        } else if(pv < dv) {
            s->games_lost++;
            snprintf(result_buf, sizeof(result_buf), "Dealer wins. -$%u", s->current_bet);
        } else {
            s->games_pushed++;
            s->balance += s->current_bet; /* return bet on push */
            snprintf(result_buf, sizeof(result_buf), "Push. +$%u", s->current_bet);
        }
    }
    
    strncpy(s->result_msg, result_buf, sizeof(s->result_msg) - 1);
    s->result_msg[sizeof(s->result_msg) - 1] = '\0';
    s->games_played++;
    s->phase = PhaseResult;
}

static void draw_callback(Canvas* canvas, void* model) {
    BlackjackState* s = (BlackjackState*)model;
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);

    if(s->phase == PhaseSplash) {
        int box_x = 2;
        int box_y = 0;
        int box_w = 124;
        int box_h = 64;
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_box(canvas, box_x, box_y, box_w, box_h);
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_frame(canvas, box_x, box_y, box_w, box_h);
        canvas_set_font(canvas, FontPrimary);
        const char* title = "BLACKJACK";
        int title_w = canvas_string_width(canvas, title);
        canvas_draw_str(canvas, box_x + (box_w - title_w) / 2, box_y + 10, title);
        canvas_set_font(canvas, FontSecondary);
        static const char* const splash_opts[SPLASH_OPTIONS] = {
            "Continue (last profile)",
            "New profile",
            "Guest game",
            "Practice mode",
            "Help"
        };
        const int line_h = 9;
        for(uint8_t i = 0; i < SPLASH_OPTIONS; i++) {
            int y = box_y + 18 + (int)i * line_h;
            if(i == s->splash_selection) canvas_draw_str(canvas, box_x + 2, y, ">");
            canvas_draw_str(canvas, box_x + 10, y, splash_opts[i]);
        }
        canvas_draw_str(canvas, 0, 62, "Up/Down OK=Select Back=Exit");
        return;
    }
    if(s->phase == PhaseGuestSavePrompt) {
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_box(canvas, 10, 18, 108, 28);
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_frame(canvas, 10, 18, 108, 28);
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 18, 28, "Save to profile?");
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 22, 38, s->profile_menu_selection == 0 ? "> No" : "  No");
        canvas_draw_str(canvas, 70, 38, s->profile_menu_selection == 1 ? "> Yes" : "  Yes");
        canvas_draw_str(canvas, 0, 62, "Up/Down OK Back=Cancel");
        return;
    }
    if(s->phase == PhaseGuestPickProfile) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 0, 10, "Save to:");
        canvas_set_font(canvas, FontSecondary);
        const int line_h = 10;
        for(uint8_t i = 0; i <= MAX_PROFILES; i++) {
            int y = 20 + (int)i * line_h;
            if((uint8_t)i == s->profile_menu_selection) canvas_draw_str(canvas, 0, y, ">");
            if(i < MAX_PROFILES) canvas_draw_str(canvas, 8, y, s->profile_names[i]);
            else canvas_draw_str(canvas, 8, y, "New profile");
        }
        canvas_draw_str(canvas, 0, 62, "Up/Down OK=Save Back=Cancel");
        return;
    }

    if(s->phase == PhaseProfileMenu) {
        canvas_draw_str(canvas, 0, 10, "Select Profile");
        canvas_set_font(canvas, FontSecondary);
        const int line_h = 10;
        for(uint8_t i = 0; i <= MAX_PROFILES; i++) {
            int y = 20 + (int)i * line_h;
            if((uint8_t)i == s->profile_menu_selection) {
                canvas_draw_str(canvas, 0, y, ">");
            }
            if(i < MAX_PROFILES) {
                canvas_draw_str(canvas, 8, y, s->profile_names[i]);
            } else {
                canvas_draw_str(canvas, 8, y, "New profile");
            }
        }
        canvas_draw_str(canvas, 0, 62, "Up/Down=Select OK=Play Back=Exit");
        return;
    }

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
    } else if(s->phase == PhaseStatistics) {
        /* Statistics display - scrollable, loops top-bottom bottom-top */
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 0, 10, "Statistics");
        
        canvas_set_font(canvas, FontSecondary);
        char stat_buf[32];
        /* Build stat lines - scroll window, no modulo (show consecutive lines) */
        const int line_h = 10;
        int y = 20;
        for(uint8_t i = 0; i < STAT_VISIBLE; i++) {
            uint8_t idx = s->stat_scroll + i;
            if(idx >= STAT_LINES) idx = STAT_LINES - 1;
            switch(idx) {
            case 0:
                snprintf(stat_buf, sizeof(stat_buf), "Games: %u", s->games_played);
                break;
            case 1:
                snprintf(stat_buf, sizeof(stat_buf), "Wins: %u", s->games_won);
                break;
            case 2:
                snprintf(stat_buf, sizeof(stat_buf), "Losses: %u", s->games_lost);
                break;
            case 3:
                snprintf(stat_buf, sizeof(stat_buf), "Pushes: %u", s->games_pushed);
                break;
            case 4: {
                if(s->games_played > 0) {
                    uint16_t win_rate = (s->games_won * 100) / s->games_played;
                    snprintf(stat_buf, sizeof(stat_buf), "Win Rate: %u%%", win_rate);
                } else {
                    snprintf(stat_buf, sizeof(stat_buf), "Win Rate: --");
                }
                break;
            }
            default:
                stat_buf[0] = '\0';
                break;
            }
            canvas_draw_str(canvas, 0, y + (i * line_h), stat_buf);
        }
        
        /* Footer instructions */
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 0, 62, "Up/Down=Scroll Back=Return");
    } else if(s->phase == PhaseHelp) {
        /* Help screen - enlarged box, no footer */
        int box_x = 5;
        int box_y = 5;
        int box_w = 118;
        int box_h = 57;
        /* White box */
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_box(canvas, box_x, box_y, box_w, box_h);
        /* Black outline */
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_frame(canvas, box_x, box_y, box_w, box_h);
        /* Help text inside box */
        canvas_set_font(canvas, FontSecondary);
        int y_pos = box_y + 8;
        canvas_draw_str(canvas, box_x + 4, y_pos, "OK=Hit  Back=Stand");
        y_pos += 8;
        canvas_draw_str(canvas, box_x + 4, y_pos, "Up=Double Down=Split");
        y_pos += 8;
        canvas_draw_str(canvas, box_x + 4, y_pos, "Right=Help");
        y_pos += 8;
        canvas_draw_str(canvas, box_x + 4, y_pos, "Result Screen:");
        y_pos += 8;
        canvas_draw_str(canvas, box_x + 4, y_pos, "Left=Bet OK=Again");
        y_pos += 8;
        canvas_draw_str(canvas, box_x + 4, y_pos, "Right=Stats");
        y_pos += 8;
        canvas_draw_str(canvas, box_x + 4, y_pos, "Back=Return");
    } else if(s->phase == PhaseReshuffle) {
        /* Reshuffle announcement - result box style */
        int box_x = 10;
        int box_y = 20;
        int box_w = 108;
        int box_h = 28;
        /* White box */
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_box(canvas, box_x, box_y, box_w, box_h);
        /* Black outline */
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_frame(canvas, box_x, box_y, box_w, box_h);
        /* Reshuffle text */
        canvas_set_font(canvas, FontPrimary);
        const char* reshuffle_text = "Reshuffling...";
        int text_x = box_x + (box_w - canvas_string_width(canvas, reshuffle_text)) / 2;
        canvas_draw_str(canvas, text_x, box_y + 18, reshuffle_text);
        /* Footer */
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 0, 62, "OK=Continue");
    } else if(s->phase == PhaseSplitPrompt) {
        /* Split prompt - result box style */
        int box_x = 10;
        int box_y = 20;
        int box_w = 108;
        int box_h = 28;
        /* White box */
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_box(canvas, box_x, box_y, box_w, box_h);
        /* Black outline */
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_frame(canvas, box_x, box_y, box_w, box_h);
        /* Split prompt text */
        canvas_set_font(canvas, FontPrimary);
        const char* split_text = "Split Pair?";
        int text_x = box_x + (box_w - canvas_string_width(canvas, split_text)) / 2;
        canvas_draw_str(canvas, text_x, box_y + 12, split_text);
        canvas_set_font(canvas, FontSecondary);
        const char* split_controls = "Down=Yes  Back=No";
        int ctrl_x = box_x + (box_w - canvas_string_width(canvas, split_controls)) / 2;
        canvas_draw_str(canvas, ctrl_x, box_y + 22, split_controls);
        if(s->practice_mode) {
            const char* hint = wizard_strategy_hint(s->player_hand, 2, s->dealer_hand[1], false, true);
            char buf[24];
            snprintf(buf, sizeof(buf), "Strategy: %s", hint);
            canvas_draw_str(canvas, 0, 52, buf);
        }
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 0, 62, "Down=Split Back=No");
    } else if(s->phase == PhaseDeal || s->phase == PhasePlayerTurn || s->phase == PhaseDealerTurn || s->phase == PhaseShowFinalCards || s->phase == PhaseResult) {
        /* Show bet amount on right side */
        canvas_set_font(canvas, FontSecondary);
        char bet_buf[20];
        snprintf(bet_buf, sizeof(bet_buf), "Bet:$%u", s->current_bet);
        int bet_width = canvas_string_width(canvas, bet_buf);
        canvas_draw_str(canvas, 128 - bet_width, 10, bet_buf);

        /* Draw dealer cards - left side, overlay cards 3+ on bottom half (moved up to avoid footer) */
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 0, 24, "D:");
        int card_x = 18;
        int card_y = 16;
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

        /* Draw player cards - right side, overlay cards 3+ on bottom half (moved up to avoid footer) */
        canvas_set_font(canvas, FontPrimary);
        if(s->is_split) {
            /* Show both hands when split - use text instead of graphics */
            /* Hand 1 */
            canvas_draw_str(canvas, 54, 24, s->active_hand == 0 ? "P1:*" : "P1:"); /* * indicates active */
            canvas_set_font(canvas, FontSecondary);
            char hand1_buf[32] = "";
            int hand1_len = 0;
            for(uint8_t i = 0; i < s->player_count && hand1_len < (int)sizeof(hand1_buf) - 4; i++) {
                char card_buf[8];
                int len = snprintf(card_buf, sizeof(card_buf), "%s%c ", card_rank_str(s->player_hand[i]), card_suit_char(s->player_hand[i]));
                if(hand1_len + len < (int)sizeof(hand1_buf)) {
                    strncpy(hand1_buf + hand1_len, card_buf, sizeof(hand1_buf) - hand1_len);
                    hand1_len += len;
                }
            }
            canvas_draw_str(canvas, 70, 24, hand1_buf);
            uint8_t pv1_soft, pv1_hard;
            hand_values_soft_hard(s->player_hand, s->player_count, &pv1_soft, &pv1_hard);
            char vbuf[16];
            if(pv1_soft != pv1_hard && pv1_soft <= 21) {
                snprintf(vbuf, sizeof(vbuf), "%u or %u", pv1_soft, pv1_hard);
            } else {
                snprintf(vbuf, sizeof(vbuf), "%u", pv1_soft);
            }
            canvas_draw_str(canvas, 54, 32, vbuf);
            
            /* Hand 2 */
            canvas_set_font(canvas, FontPrimary);
            canvas_draw_str(canvas, 54, 40, s->active_hand == 1 ? "P2:*" : "P2:");
            canvas_set_font(canvas, FontSecondary);
            char hand2_buf[32] = "";
            int hand2_len = 0;
            for(uint8_t i = 0; i < s->player_count2 && hand2_len < (int)sizeof(hand2_buf) - 4; i++) {
                char card_buf[8];
                int len = snprintf(card_buf, sizeof(card_buf), "%s%c ", card_rank_str(s->player_hand2[i]), card_suit_char(s->player_hand2[i]));
                if(hand2_len + len < (int)sizeof(hand2_buf)) {
                    strncpy(hand2_buf + hand2_len, card_buf, sizeof(hand2_buf) - hand2_len);
                    hand2_len += len;
                }
            }
            canvas_draw_str(canvas, 70, 40, hand2_buf);
            uint8_t pv2_soft, pv2_hard;
            hand_values_soft_hard(s->player_hand2, s->player_count2, &pv2_soft, &pv2_hard);
            if(pv2_soft != pv2_hard && pv2_soft <= 21) {
                snprintf(vbuf, sizeof(vbuf), "%u or %u", pv2_soft, pv2_hard);
            } else {
                snprintf(vbuf, sizeof(vbuf), "%u", pv2_soft);
            }
            canvas_draw_str(canvas, 54, 48, vbuf);
        } else {
            /* Single hand */
            canvas_draw_str(canvas, 54, 24, "P:"); /* Moved left to avoid covering first card */
            card_x = 70; /* Start on right side */
            card_y = 16;
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
            /* Draw player hand value directly underneath "P:" label - show soft/hard for aces */
            uint8_t pv_soft, pv_hard;
            hand_values_soft_hard(s->player_hand, s->player_count, &pv_soft, &pv_hard);
            char vbuf[16];
            if(pv_soft != pv_hard && pv_soft <= 21) {
                snprintf(vbuf, sizeof(vbuf), "%u or %u", pv_soft, pv_hard);
            } else {
                snprintf(vbuf, sizeof(vbuf), "%u", pv_soft);
            }
            canvas_set_font(canvas, FontSecondary);
            canvas_draw_str(canvas, 54, 32, vbuf);
        }
        /* Draw dealer hand value with soft/hard for aces */
        if(!s->dealer_hole && s->dealer_count > 0) {
            uint8_t dv_soft, dv_hard;
            hand_values_soft_hard(s->dealer_hand, s->dealer_count, &dv_soft, &dv_hard);
            char dvbuf[16];
            if(dv_soft != dv_hard && dv_soft <= 21) {
                snprintf(dvbuf, sizeof(dvbuf), "%u or %u", dv_soft, dv_hard);
            } else {
                snprintf(dvbuf, sizeof(dvbuf), "%u", dv_soft);
            }
            canvas_set_font(canvas, FontSecondary);
            canvas_draw_str(canvas, 0, 32, dvbuf);
        }

        /* Help text in footer - one line */
        canvas_set_font(canvas, FontSecondary);
        if(s->phase == PhasePlayerTurn) {
            if(s->practice_mode) {
                const uint8_t* ph = (s->is_split && s->active_hand == 1) ? s->player_hand2 : s->player_hand;
                uint8_t pc = (s->is_split && s->active_hand == 1) ? s->player_count2 : s->player_count;
                const char* hint = wizard_strategy_hint(ph, pc, s->dealer_hand[1], s->can_double_down, s->can_split);
                char buf[24];
                snprintf(buf, sizeof(buf), "Strategy: %s", hint);
                canvas_draw_str(canvas, 0, 52, buf);
            }
            canvas_draw_str(canvas, 0, 62, "K=Hit BK=Stand R=Help");
        } else if(s->phase == PhaseShowFinalCards) {
            canvas_draw_str(canvas, 0, 62, "K=Result R=Help");
        } else if(s->phase == PhaseResult) {
            /* Draw result box overlay - white box with black outline (larger) */
            int box_x = 10;
            int box_y = 10;
            int box_w = 108;
            int box_h = 40;
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
            if(s->is_split) {
                uint8_t pv2_final = hand_value(s->player_hand2, s->player_count2);
                snprintf(result_text, sizeof(result_text), "P1:%u P2:%u D:%u", pv_final, pv2_final, dv_final);
            } else {
                snprintf(result_text, sizeof(result_text), "P:%u D:%u", pv_final, dv_final);
            }
            int text_x = box_x + (box_w - canvas_string_width(canvas, result_text)) / 2;
            canvas_draw_str(canvas, text_x, box_y + 12, result_text);
            /* Result message centered */
            int msg_x = box_x + (box_w - canvas_string_width(canvas, s->result_msg)) / 2;
            canvas_draw_str(canvas, msg_x, box_y + 28, s->result_msg);
            /* Footer instructions - one line */
            canvas_set_font(canvas, FontSecondary);
            canvas_draw_str(canvas, 0, 62, "Left=Bet OK=Again Right=Stats");
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
        if(s->phase == PhaseHelp) {
            /* Return from help to previous phase */
            s->phase = s->prev_phase;
            view_commit_model(app->view, true);
            return true;
        }
        if(s->phase == PhaseSplitPrompt) {
            /* Decline split - continue with normal play */
            s->phase = PhasePlayerTurn;
            s->can_double_down = (s->player_count == 2 && (s->balance >= s->current_bet));
            s->can_split = false;
            view_commit_model(app->view, true);
            return true;
        }
        if(s->phase == PhaseStatistics) {
            /* Return from statistics to result screen */
            s->phase = PhaseResult;
            view_commit_model(app->view, true);
            return true;
        }
        if(s->phase == PhaseSplash) {
            view_commit_model(app->view, false);
            view_dispatcher_stop(app->view_dispatcher);
            return true;
        }
        if(s->phase == PhaseProfileMenu) {
            view_commit_model(app->view, false);
            view_dispatcher_stop(app->view_dispatcher);
            return true;
        }
        if(s->phase == PhaseResult || s->phase == PhaseBetting) {
            if(s->is_guest) {
                s->phase = PhaseGuestSavePrompt;
                s->profile_menu_selection = 0;  /* No */
                view_commit_model(app->view, true);
                return true;
            }
            Storage* storage = furi_record_open(RECORD_STORAGE);
            profile_save_current(storage, s);
            furi_record_close(RECORD_STORAGE);
            s->phase = PhaseProfileMenu;
            s->profile_menu_selection = 0;
            view_commit_model(app->view, true);
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
        if(s->phase == PhaseSplash) {
            Storage* storage = furi_record_open(RECORD_STORAGE);
            profile_ensure_dir(storage);
            profile_load_list(storage, s);
            switch(s->splash_selection) {
            case 0: /* Continue */
                profile_load_slot(storage, s, profile_load_last_used(storage));
                s->is_guest = false;
                s->practice_mode = false;
                furi_record_close(RECORD_STORAGE);
                game_start_betting(s);
                break;
            case 1: /* New profile */
                profile_create_new(s);
                s->is_guest = false;
                s->practice_mode = false;
                profile_save_last_used(storage, s->current_profile_slot);
                furi_record_close(RECORD_STORAGE);
                game_start_betting(s);
                break;
            case 2: /* Guest */
                s->balance = STARTING_BALANCE;
                s->games_played = s->games_won = s->games_lost = s->games_pushed = 0;
                s->current_profile_slot = 0;
                s->is_guest = true;
                s->practice_mode = false;
                furi_record_close(RECORD_STORAGE);
                game_start_betting(s);
                break;
            case 3: /* Practice */
                s->balance = STARTING_BALANCE;
                s->games_played = s->games_won = s->games_lost = s->games_pushed = 0;
                s->current_profile_slot = 0;
                s->is_guest = true;
                s->practice_mode = true;
                furi_record_close(RECORD_STORAGE);
                game_start_betting(s);
                break;
            case 4: /* Help */
                s->prev_phase = PhaseSplash;
                s->phase = PhaseHelp;
                furi_record_close(RECORD_STORAGE);
                break;
            default:
                furi_record_close(RECORD_STORAGE);
                break;
            }
            view_commit_model(app->view, true);
            return true;
        }
        if(s->phase == PhaseGuestSavePrompt) {
            if(s->profile_menu_selection == 0) {
                s->phase = PhaseSplash;
                s->is_guest = false;
            } else {
                s->phase = PhaseGuestPickProfile;
                s->profile_menu_selection = 0;
                Storage* storage = furi_record_open(RECORD_STORAGE);
                profile_load_list(storage, s);
                furi_record_close(RECORD_STORAGE);
            }
            view_commit_model(app->view, true);
            return true;
        }
        if(s->phase == PhaseGuestPickProfile) {
            Storage* storage = furi_record_open(RECORD_STORAGE);
            if(s->profile_menu_selection < MAX_PROFILES) {
                profile_save_guest_to_slot(storage, s, s->profile_menu_selection);
            } else {
                uint8_t slot = 0;
                for(; slot < MAX_PROFILES && s->profile_names[slot][0] != '\0'; slot++) { }
                if(slot >= MAX_PROFILES) slot = 0;
                s->current_profile_slot = slot;
                snprintf(s->profile_names[slot], PROFILE_NAME_LEN, "Player %u", (unsigned)(slot + 1));
                profile_save_current(storage, s);
            }
            furi_record_close(RECORD_STORAGE);
            s->is_guest = false;
            s->phase = PhaseSplash;
            s->splash_selection = 0;
            view_commit_model(app->view, true);
            return true;
        }
        if(s->phase == PhaseProfileMenu) {
            Storage* storage = furi_record_open(RECORD_STORAGE);
            profile_ensure_dir(storage);
            profile_load_list(storage, s);
            if(s->profile_menu_selection < MAX_PROFILES) {
                profile_load_slot(storage, s, s->profile_menu_selection);
            } else {
                profile_create_new(s);
            }
            furi_record_close(RECORD_STORAGE);
            game_start_betting(s);
            view_commit_model(app->view, true);
            return true;
        }
        if(s->phase == PhaseReshuffle) {
            /* Continue after reshuffle */
            s->reshuffle_announced = true;
            /* If we were dealing, continue */
            if(s->player_count < 2 || s->dealer_count < 2) {
                s->phase = PhaseDeal;
                /* Continue dealing */
                while(s->player_count < 2) {
                    s->player_hand[s->player_count++] = draw_card(s);
                    if(!s->reshuffle_announced) {
                        s->phase = PhaseReshuffle;
                        view_commit_model(app->view, true);
                        return true;
                    }
                }
                while(s->dealer_count < 2) {
                    s->dealer_hand[s->dealer_count++] = draw_card(s);
                    if(!s->reshuffle_announced) {
                        s->phase = PhaseReshuffle;
                        view_commit_model(app->view, true);
                        return true;
                    }
                }
                /* Finish dealing */
                uint8_t pv = hand_value(s->player_hand, s->player_count);
                if(pv == 21) {
                    s->is_blackjack = true;
                    s->dealer_hole = false;
                    s->phase = PhaseShowFinalCards;
                    uint8_t dv = hand_value(s->dealer_hand, s->dealer_count);
                    if(dv == 21) {
                        s->balance += s->current_bet;
                        snprintf(s->result_msg, sizeof(s->result_msg), "Push. +$%u", s->current_bet);
                    } else {
                        uint16_t payout = s->current_bet + (s->current_bet * 3 / 2);
                        s->balance += payout;
                        snprintf(s->result_msg, sizeof(s->result_msg), "Blackjack! +$%u", payout);
                    }
                    s->result_msg[sizeof(s->result_msg) - 1] = '\0';
                } else {
                    /* Dealer peek: 10 or Ace up -> check dealer blackjack */
                    uint8_t dealer_up_rank = s->dealer_hand[1] % 13;
                    bool dealer_has_10_or_ace = (dealer_up_rank >= 9);
                    if(dealer_has_10_or_ace) {
                        uint8_t dv = hand_value(s->dealer_hand, s->dealer_count);
                        if(dv == 21) {
                            s->dealer_hole = false;
                            s->games_played++;
                            s->games_lost++;
                            snprintf(s->result_msg, sizeof(s->result_msg), "Dealer blackjack. -$%u", s->current_bet);
                            s->result_msg[sizeof(s->result_msg) - 1] = '\0';
                            s->phase = PhaseResult;
                            view_commit_model(app->view, true);
                            return true;
                        }
                    }
                    bool is_pair = (s->player_count == 2 && CARD_RANK(s->player_hand[0]) == CARD_RANK(s->player_hand[1]));
                    s->can_split = (is_pair && (s->balance >= s->current_bet));
                    if(s->can_split) {
                        s->phase = PhaseSplitPrompt;
                    } else {
                        s->phase = PhasePlayerTurn;
                        s->can_double_down = (s->player_count == 2 && (s->balance >= s->current_bet));
                    }
                }
            } else {
                /* Return to player turn */
                s->phase = PhasePlayerTurn;
            }
            view_commit_model(app->view, true);
            return true;
        }
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
        if(s->phase == PhaseResult) {
            /* Open statistics window */
            game_show_statistics(s);
            view_commit_model(app->view, true);
            return true;
        }
        if(s->phase == PhasePlayerTurn || s->phase == PhaseShowFinalCards) {
            /* Open help screen */
            game_show_help(s);
            view_commit_model(app->view, true);
            return true;
        }
    }
    if(event->key == InputKeyUp) {
        if(s->phase == PhaseSplash) {
            if(s->splash_selection == 0) s->splash_selection = SPLASH_OPTIONS - 1;
            else s->splash_selection--;
            view_commit_model(app->view, true);
            return true;
        }
        if(s->phase == PhaseGuestSavePrompt) {
            s->profile_menu_selection = 0;
            view_commit_model(app->view, true);
            return true;
        }
        if(s->phase == PhaseGuestPickProfile) {
            if(s->profile_menu_selection == 0) s->profile_menu_selection = MAX_PROFILES;
            else s->profile_menu_selection--;
            view_commit_model(app->view, true);
            return true;
        }
        if(s->phase == PhaseProfileMenu) {
            if(s->profile_menu_selection == 0) {
                s->profile_menu_selection = MAX_PROFILES;
            } else {
                s->profile_menu_selection--;
            }
            view_commit_model(app->view, true);
            return true;
        }
        if(s->phase == PhaseStatistics) {
            /* Scroll up - at top wrap to bottom */
            if(s->stat_scroll == 0) {
                s->stat_scroll = STAT_MAX_SCROLL;
            } else {
                s->stat_scroll--;
            }
            view_commit_model(app->view, true);
            return true;
        }
        if(s->phase == PhasePlayerTurn) {
            game_player_double_down(s);
            view_commit_model(app->view, true);
            return true;
        }
    }
    if(event->key == InputKeyDown) {
        if(s->phase == PhaseSplash) {
            if(s->splash_selection >= SPLASH_OPTIONS - 1) s->splash_selection = 0;
            else s->splash_selection++;
            view_commit_model(app->view, true);
            return true;
        }
        if(s->phase == PhaseGuestSavePrompt) {
            s->profile_menu_selection = 1;
            view_commit_model(app->view, true);
            return true;
        }
        if(s->phase == PhaseGuestPickProfile) {
            if(s->profile_menu_selection >= MAX_PROFILES) s->profile_menu_selection = 0;
            else s->profile_menu_selection++;
            view_commit_model(app->view, true);
            return true;
        }
        if(s->phase == PhaseProfileMenu) {
            if(s->profile_menu_selection >= MAX_PROFILES) {
                s->profile_menu_selection = 0;
            } else {
                s->profile_menu_selection++;
            }
            view_commit_model(app->view, true);
            return true;
        }
        if(s->phase == PhaseStatistics) {
            /* Scroll down - at bottom wrap to top */
            if(s->stat_scroll >= STAT_MAX_SCROLL) {
                s->stat_scroll = 0;
            } else {
                s->stat_scroll++;
            }
            view_commit_model(app->view, true);
            return true;
        }
        if(s->phase == PhaseSplitPrompt) {
            /* Accept split */
            game_player_split(s);
            view_commit_model(app->view, true);
            return true;
        }
        if(s->phase == PhasePlayerTurn) {
            /* Allow split from player turn if still available */
            if(s->can_split) {
                s->phase = PhaseSplitPrompt;
                view_commit_model(app->view, true);
                return true;
            }
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
    state->bet_hand2 = 0;
    state->games_played = 0;
    state->games_won = 0;
    state->games_lost = 0;
    state->games_pushed = 0;
    state->player_count2 = 0;
    state->phase = PhaseSplash;
    state->splash_selection = 0;
    state->profile_menu_selection = 0;
    state->current_profile_slot = 0;
    state->is_guest = false;
    state->practice_mode = false;
    Storage* storage = furi_record_open(RECORD_STORAGE);
    profile_load_list(storage, state);
    furi_record_close(RECORD_STORAGE);
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
