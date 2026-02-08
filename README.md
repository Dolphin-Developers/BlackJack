# Blackjack

Classic **Blackjack** (21) for Flipper Zero. Play against the dealer: get closer to 21 than the dealer without going over.

## How to play

- **Dealer (D)** and **Player (P)** each start with two cards. One dealer card is hidden until you stand.
- **Hit** = take another card. **Stand** = keep your hand and let the dealer play.
- Number in parentheses is the hand total. Face cards = 10, Ace = 1 or 11 (best value).
- Dealer must draw to 16 and stand on 17 or higher.
- You **win** if your total is higher than the dealer’s (without going over 21), or if the dealer busts. **Push** = tie.

## Controls

| Button | Action |
|--------|--------|
| **OK** | **Hit** (during your turn) or **New game** (on result screen) |
| **Back** | **Stand** (during your turn) or **Exit** (on result screen) |

## Installation

- **From Apps Catalog**: Install via the Flipper mobile app or [Flipper Lab](https://lab.flipper.net/apps) (Games → Blackjack).
- **Manual**: Copy `blackjack.fap` from the `dist` folder to `SD:/apps/Games/` on your Flipper’s microSD card.

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
