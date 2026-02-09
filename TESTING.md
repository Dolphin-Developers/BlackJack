# Blackjack — On-Device Testing Checklist

Use this checklist when testing the app on a Flipper Zero (firmware release or RC). Test with **latest build** (`ufbt` → `dist/blackjack.fap` or `ufbt launch`).

---

## 1. App launch and splash

| # | Test | Pass |
|---|------|------|
| 1.1 | App appears under **Games** as "Blackjack" with correct icon (Ace + Queen). | ☐ |
| 1.2 | Splash shows 6 options: Continue, New profile, Guest game, Practice mode, Help, Settings. | ☐ |
| 1.3 | **Up/Down** moves selection (cursor/highlight); wrap from bottom to top and top to bottom. | ☐ |
| 1.4 | **Back** from splash exits the app. | ☐ |

---

## 2. Help

| # | Test | Pass |
|---|------|------|
| 2.1 | Splash → **Help** → Help screen shows (controls, rules, etc.). | ☐ |
| 2.2 | **Back** from Help returns to splash. | ☐ |
| 2.3 | Help content is readable (no cut-off, scroll if present). | ☐ |

---

## 3. Settings

| # | Test | Pass |
|---|------|------|
| 3.1 | Splash → **Settings** → Settings screen shows: Sound, Vibro, Dealer S17, Erase all profiles. | ☐ |
| 3.2 | **Up/Down** moves selection; wrap correctly. | ☐ |
| 3.3 | **OK** on Sound toggles On/Off; label updates (Sound: On / Sound: Off). | ☐ |
| 3.4 | **OK** on Vibro toggles On/Off. | ☐ |
| 3.5 | **OK** on Dealer S17 toggles On/Off (Dealer S17: Hit / Stand). | ☐ |
| 3.6 | **Back** from Settings returns to splash. | ☐ |
| 3.7 | Quit app, relaunch → Settings choices are still as set (persisted to SD). | ☐ |
| 3.8 | **Erase all profiles**: Select "Erase all profiles" → **OK** → Confirmation screen ("Erase all profiles? Banks reset to $3125. Back=Cancel OK=Yes"). | ☐ |
| 3.9 | Confirmation **Back** → returns to Settings (no erase). | ☐ |
| 3.10 | Confirmation **OK** → all profiles reset (banks $3,125, names cleared/reset); return to splash. | ☐ |

---

## 4. Profiles and continue

| # | Test | Pass |
|---|------|------|
| 4.1 | **Continue** (with no prior save): loads last-used or default profile; balance $3,125 or saved value. | ☐ |
| 4.2 | **New profile**: Creates/overwrites profile; goes to betting with $3,125. | ☐ |
| 4.3 | Play a hand, then **Back** from Result/Bet → profile menu or "Save to profile?" (guest). | ☐ |
| 4.4 | Relaunch → **Continue** → same profile and balance as when left. | ☐ |

---

## 5. Guest and practice

| # | Test | Pass |
|---|------|------|
| 5.1 | **Guest game** → Betting with $3,125; no profile name shown (or "Guest"). | ☐ |
| 5.2 | From Result or Bet, **Back** → "Save to profile?" (No / Yes). | ☐ |
| 5.3 | **No** → return to splash. **Yes** → pick slot or New profile; save then splash. | ☐ |
| 5.4 | **Practice mode** → Same as guest but strategy hints (e.g. Hit/Stand/Double/Split) appear during player turn. | ☐ |

---

## 6. Betting

| # | Test | Pass |
|---|------|------|
| 6.1 | Betting screen shows balance and current bet (e.g. Bet: $5). | ☐ |
| 6.2 | **Left** decreases bet by $5 (min $5). **Right** increases bet by $5 (max $500). | ☐ |
| 6.3 | Chip stack graphic updates with bet amount. | ☐ |
| 6.4 | **OK** places bet and deals (balance decreases by bet). | ☐ |
| 6.5 | **Back** from betting → profile menu (or guest save prompt). | ☐ |

---

## 7. Gameplay — basic

| # | Test | Pass |
|---|------|------|
| 7.1 | After deal: 2 player cards and 2 dealer cards; one dealer card hidden (hole). | ☐ |
| 7.2 | **OK** = Hit: one card added; total(s) update. | ☐ |
| 7.3 | **Back** = Stand: dealer reveals hole, dealer draws to 16/17 per rules. | ☐ |
| 7.4 | Hand totals (P: / D:) correct; bust (>21) detected. | ☐ |
| 7.5 | Final cards screen shows all cards; **OK** → result. | ☐ |
| 7.6 | Result screen: correct message (Win/Loss/Push/Blackjack/Dealer bust etc.) and balance update. | ☐ |

---

## 8. Audio and vibration (respect Settings)

| # | Test | Pass |
|---|------|------|
| 8.1 | Settings **Sound: On** → **Hit** and **Stand** play short tones. | ☐ |
| 8.2 | Settings **Sound: Off** → No sound on Hit or Stand. | ☐ |
| 8.3 | Settings **Vibro: On** → Short vibration on **blackjack** (player 21 in 2 cards or dealer blackjack). | ☐ |
| 8.4 | Settings **Vibro: Off** → No vibration on blackjack. | ☐ |
| 8.5 | No vibration on normal win/loss/hit/stand (only blackjack). | ☐ |

---

## 9. Dealer rules — soft 17

| # | Test | Pass |
|---|------|------|
| 9.1 | Settings **Dealer S17: Stand** (default): Dealer with soft 17 (A+6) stands. | ☐ |
| 9.2 | Settings **Dealer S17: Hit**: Dealer with soft 17 draws one more card. | ☐ |
| 9.3 | Dealer still draws to 16 and stands on hard 17+. | ☐ |

*(If hard to trigger soft 17, at least verify setting toggles and persists.)*

---

## 10. Double down and split

| # | Test | Pass |
|---|------|------|
| 10.1 | With 2 cards, **Up** = Double down: bet doubles, one card dealt, then auto-stand. | ☐ |
| 10.2 | Double down only offered when balance ≥ current bet. | ☐ |
| 10.3 | Pair (same rank) → "Split Pair?" prompt. **Down** = Yes: two hands, second bet placed. | ☐ |
| 10.4 | **Back** on split prompt = No: play first hand only. | ☐ |
| 10.5 | Split: play first hand (Hit/Stand), then second hand; then dealer; result shows both hands. | ☐ |

---

## 11. Insurance and dealer blackjack

| # | Test | Pass |
|---|------|------|
| 11.1 | Dealer up card = Ace → "Insurance?" (No/Yes). **OK** toggles; confirm choice. | ☐ |
| 11.2 | Insurance Yes: half bet taken; if dealer blackjack, insurance pays 2:1; main bet push. | ☐ |
| 11.3 | Dealer up card 10/J/Q/K → if dealer has blackjack, hand ends immediately (loss; vibro if on). | ☐ |
| 11.4 | Player blackjack (A+10) when dealer doesn’t: 3:2 payout, vibro if on. | ☐ |

---

## 12. Statistics and result screen

| # | Test | Pass |
|---|------|------|
| 12.1 | Result screen: **Right** → Statistics (games, wins, losses, pushes, win rate). | ☐ |
| 12.2 | Stats scroll (Up/Down) and wrap. **Back** → result screen. | ☐ |
| 12.3 | Result **Left** → back to betting (same bet). **OK** → bet again (new hand). | ☐ |
| 12.4 | Result **Back** → profile menu (or guest save). | ☐ |

---

## 13. Edge cases and stability

| # | Test | Pass |
|---|------|------|
| 13.1 | Play until reshuffle message appears (near bottom of shoe); game continues. | ☐ |
| 13.2 | 6 cards without bust → player wins (if implemented). | ☐ |
| 13.3 | No crash on rapid button presses (Hit, Stand, menu navigation). | ☐ |
| 13.4 | Balance never goes negative; min bet $5 enforced. | ☐ |
| 13.5 | With no SD card (if possible): app runs; settings/profiles may use defaults or in-memory only. | ☐ |

---

## 14. Build and version

| # | Test | Pass |
|---|------|------|
| 14.1 | Build: `ufbt` completes; `dist/blackjack.fap` (or equivalent) present. | ☐ |
| 14.2 | App version in manifest/fam matches expected (e.g. 0.5). | ☐ |

---

## Sign-off

- **Tester:** ________________  
- **Device / FW:** ________________  
- **Date:** ________________  
- **Build (commit or tag):** ________________  

**Notes:**

_________________________________________________________________
_________________________________________________________________
_________________________________________________________________
