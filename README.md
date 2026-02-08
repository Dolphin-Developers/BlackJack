# Blackjack

Classic **Blackjack** (21) for Flipper Zero. Play against the dealer: get closer to 21 than the dealer without going over.

## How to play

- Start with **$3,125**. Place a bet before each round ($5 minimum, $500 maximum).
- **Dealer (D)** and **Player (P)** each start with two cards. One dealer card is hidden until you stand.
- **Hit** = take another card. **Stand** = keep your hand and let the dealer play.
- Hand totals shown under "D:" and "P:" labels. Face cards = 10, Ace = 1 or 11 (best value).
- Dealer must draw to 16 and stand on 17 or higher.
- **Payouts**: Win = 1:1 (double your bet), Blackjack = 3:2, Push = bet returned, Loss = bet lost.
- **Special rules**: Player wins with 6 cards without busting. Dealer busts if they draw 6 cards without winning.

## Controls

### Betting Phase
| Button | Action |
|--------|--------|
| **Left** | Decrease bet (by $5) |
| **Right** | Increase bet (by $5) |
| **OK** | Place bet and deal cards |
| **Back** | Exit game |

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

### Result Screen
| Button | Action |
|--------|--------|
| **Left** | Change bet (return to betting phase) |
| **OK** | Bet again (deal new hand with same bet) |
| **Right** | View statistics (wins, losses, win rate) |
| **Back** | Exit game |

## Features

- **Betting system**: Start with $3,125, bet $5-$500 per hand
- **Double down**: Double your bet on first 2 cards (if balance allows), draw one card, then automatically stand
- **Split pairs**: Split pairs into two separate hands (if first 2 cards are same rank and balance allows), play each hand sequentially
- **Visual cards**: Black cards with white text, arranged in 2-column grid
- **Chip stack**: Visual indicator grows with bet amount (1 chip per $25)
- **Result overlay**: Centered white box shows final scores and outcome
- **Statistics**: Track wins, losses, pushes, and win rate (Right button on result screen)
- **6-card rule**: Win with 6 cards without busting (rare but rewarding!)

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

## Hardware

No extra hardware. Works on Flipper Zero with official or compatible firmware (release/RC channel).

## License

MIT. See [LICENSE](LICENSE).
