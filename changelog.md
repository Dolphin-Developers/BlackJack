# Changelog

## v0.4

- **Splash menu**: On start, choose Continue (last profile), New profile, Guest game, Practice mode, or Help (Up/Down, OK, Back=exit).
- **Player profiles**: Up to 4 saved profiles; bank and game stats stored on SD under `apps_data/blackjack/`.
- **Last-used profile**: "Continue" loads the profile you last played; saved automatically when leaving to menu.
- **Guest game**: Play without saving; Back from Bet or Result shows "Save to profile?" (Yes = pick slot or New profile, No = return to splash).
- **Practice mode**: Wizard of Odds basic strategy hints shown during player turn and split prompt (Hit/Stand/Double/Split).
- **Dealer peek**: When dealer up card is 10 or Ace, dealer blackjack is checked immediately; hand ends with no hit/double/split.
- **Double down**: Only allowed with exactly 2 cards (guard added).

## v0.3

- Added statistics tracking: Win/loss/push counts and win rate percentage
- Statistics window accessible from result screen (Right button); scrollable (Up/Down, loops)
- Double down feature: Double bet on first 2 cards, draw one card, then stand (Up button)
- Split pairs: Split pairs into two hands, play sequentially; prompt "Split Pair?" (Down=Yes, Back=No)
- Split display: Player cards shown as text during split to avoid overlap; dealer cards remain graphical
- 3-deck shoe: Game uses 156 cards; burn top card and bottom 20; reshuffle announced in result-style box
- Soft/hard totals: Hand totals show "X or Y" for hands with aces when both values apply
- Help screen: Accessible via Right button during play; enlarged box; no footer (Back to return)
- UI improvements: Larger result box (40px height) to accommodate split hand results
- Footer simplified to one line with abbreviated controls (K=Hit BK=Stand R=Help)
- Card positions adjusted to avoid footer collision
- Split hand results display both hands in result box (P1: P2: D:)

## v0.2

- Added betting system: Start with $3,125, bet $5-$500 per hand
- Visual card graphics: Black cards with white text, 2-column grid layout
- Chip stack indicator: Grows with bet amount (1 chip per $25)
- Result box overlay: Centered white box with final scores and outcome
- Pause on final cards: Review final hands before seeing result
- 6-card win rule: Player wins with 6 cards without busting
- 6-card dealer bust: Dealer busts if they draw 6 cards without winning
- Updated controls: Change bet, bet again, or exit from result screen
- Improved UI spacing and layout

## v0.1

- Initial release.
- Classic Blackjack vs dealer: Hit / Stand.
- Dealer draws to 16, stands on 17+.
- OK = Hit / New game, Back = Stand / Exit.
