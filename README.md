# Blackjack

Classic **Blackjack** (21) for Flipper Zero. Play against the dealer: get closer to 21 than the dealer without going over.

## How to play

- **Splash menu** (on start): Continue (last profile), New profile, Guest game, Practice mode, Help, or Settings. Up/Down to select, OK to choose, Back to exit.
- **Profiles**: Up to 4 saved profiles (bank + game stats). Last-used profile is remembered for "Continue."
- **Guest game**: Play without saving; from Bet or Result, Back asks "Save to profile?" (Yes = pick slot to save, No = return to splash).
- **Practice mode**: Same as guest but shows **Wizard of Odds** basic strategy hints (Hit/Stand/Double/Split) during your turn.
- Start with **$3,125** per profile (or guest). Place a bet before each round ($5 minimum, $500 maximum).
- **Dealer (D)** and **Player (P)** each start with two cards. One dealer card is hidden until you stand.
- **Hit** = take another card. **Stand** = keep your hand and let the dealer play.
- Hand totals shown under "D:" and "P:" labels. Face cards = 10, Ace = 1 or 11 (best value).
- Dealer must draw to 16 and stand on 17 or higher (optional in Settings: dealer can hit soft 17).
- **Payouts**: Win = 1:1 (double your bet), Blackjack = 3:2, Push = bet returned, Loss = bet lost.
- **Special rules**: Player wins with 6 cards without busting. Dealer busts if they draw 6 cards without winning.
- **3-deck shoe**: 156 cards; top card and bottom 20 are burned; reshuffle is announced on screen.
- **Split**: When you have a pair, you are asked "Split Pair?" — Down=Yes, Back=No.
- **Statistics**: Scroll with Up/Down (loops); Back to return.

## Controls

### Splash Menu
| Button | Action |
|--------|--------|
| **Up/Down** | Change selection |
| **OK** | Select (Continue / New profile / Guest / Practice / Help / Settings) |
| **Back** | Exit app |

### Betting Phase
| Button | Action |
|--------|--------|
| **Left** | Decrease bet (by $5) |
| **Right** | Increase bet (by $5) |
| **OK** | Place bet and deal cards |
| **Back** | Save & return to profile menu (or, if guest, "Save to profile?" prompt) |

### During Play
| Button | Action |
|--------|--------|
| **OK** | **Hit** (take another card) |
| **Up** | **Double Down** (double bet, draw one card, then stand - only on first 2 cards) |
| **Down** | **Split** (split pairs into two hands - only on first 2 cards if they're a pair) |
| **Back** | **Stand** (end your turn, or move to second hand if split) |
| **Right** | **Help** (view all controls and instructions) |

### Final Cards Screen
| Button | Action |
|--------|--------|
| **OK** | Show result |

### Settings (from splash)
| Button | Action |
|--------|--------|
| **Up/Down** | Move selection (Sound, Vibro, Dealer S17, Erase all profiles) |
| **OK** | Toggle option (Sound/Vibro/Dealer S17) or run Erase (then confirm) |
| **Back** | Return to splash |

### Result Screen
| Button | Action |
|--------|--------|
| **Left** | Change bet (return to betting phase) |
| **OK** | Bet again (deal new hand with same bet) |
| **Right** | View statistics (wins, losses, win rate) |
| **Back** | Save & return to profile menu (or guest: "Save to profile?" prompt) |

## Features

- **Splash menu**: Continue (last profile), New profile, Guest game, Practice mode, Help, Settings
- **Player profiles**: Up to 4 saved profiles; bank and game stats stored on SD (`apps_data/blackjack/`)
- **Last-used profile**: "Continue" loads the last profile you played
- **Guest game**: Play without a profile; optionally save to a profile when leaving (Back from Bet/Result)
- **Practice mode**: Wizard of Odds basic strategy hints (Hit/Stand/Double/Split) during your turn and on split prompt
- **Betting system**: Start with $3,125, bet $5-$500 per hand
- **Double down**: Double your bet on first 2 cards (if balance allows), draw one card, then automatically stand
- **Split pairs**: Split pairs into two separate hands (if first 2 cards are same rank and balance allows), play each hand sequentially
- **Visual cards**: Black cards with white text, arranged in 2-column grid
- **Chip stack**: Visual indicator grows with bet amount (1 chip per $25)
- **Result overlay**: Centered white box shows final scores and outcome
- **Statistics**: Track wins, losses, pushes, and win rate (Right on result screen); scrollable list (Up/Down, loops)
- **6-card rule**: Win with 6 cards without busting (rare but rewarding!)
- **Settings**: Sound on/off, vibration on/off, dealer hits soft 17 on/off; erase all profiles (reset banks to $3,125). Stored on SD.
- **Feedback**: Short vibration on blackjack (player or dealer); short tones on Hit and Stand (when sound is on).

## Installation

- **From Apps Catalog**: Install via the Flipper mobile app or [Flipper Lab](https://lab.flipper.net/apps) (Games → Blackjack).
- **Manual**: Copy `blackjack.fap` from the `dist` folder to `SD:/apps/Games/` on your Flipper's microSD card.

## Building

Requires [uFBT](https://github.com/flipperdevices/flipperzero-ufbt) (Applications SDK):

```bash
python3 -m pip install --upgrade ufbt
ufbt
```

Output: `dist/blackjack.fap`. Run on device over USB with `ufbt launch`.

## Catalog submission (App Store)

To submit to the [Flipper Apps Catalog](https://github.com/flipperdevices/flipper-application-catalog) (Flipper Lab / mobile app):

1. **Screenshot** (required): Take at least one screenshot with **qFlipper**, save as `screenshots/ss0.png`. Do not change resolution or format.
2. **Tag and push**: Commit all changes, then e.g. `git tag v0.4 && git push origin main --tags`. Note the commit SHA you want the catalog to build from.
3. **Fork the catalog**: Fork [flipper-application-catalog](https://github.com/flipperdevices/flipper-application-catalog), clone your fork, create a branch (e.g. `blackjack-0.4`).
4. **Add manifest**: In the fork, create `applications/Games/blackjack/manifest.yml`. Use the template in this repo (`manifest.yml`): set `sourcecode.location.origin` to your repo URL, set `sourcecode.location.commit_sha` to your release commit SHA. If the app is in a subdirectory, set `sourcecode.location.subdir` (e.g. `BlackJack`).
5. **Screenshot in repo**: Ensure `screenshots/ss0.png` exists in your app repo (same commit as `commit_sha`).
6. **Validate**: In the catalog repo clone, run  
   `python3 tools/bundle.py --nolint applications/Games/blackjack/manifest.yml bundle.zip`  
   (see catalog docs for venv/setup). Fix any errors.
7. **Open a PR** to `flipper-application-catalog` with your branch; fill the PR template. Respond to review within 14 days if requested.

See `ROADMAP.md` for full checklist and links.

## Hardware

No extra hardware. Works on Flipper Zero with official or compatible firmware (release/RC channel).

## License

MIT. See [LICENSE](LICENSE).
