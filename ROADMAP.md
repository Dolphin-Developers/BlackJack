# Blackjack for Flipper Zero — Roadmap

A Blackjack game for Flipper Zero, built for listing in the **official Flipper Apps Catalog** (mobile app and [Flipper Lab](https://lab.flipper.net/apps)).

---

## 1. Official compliance (Apps Catalog)

These requirements come from the [Apps Catalog: Contribution Guide](https://github.com/flipperdevices/flipper-application-catalog/blob/main/documentation/Contributing.md). We design and build with them from the start.

### 1.1 General terms (we will comply)

| Requirement | Our approach |
|-------------|--------------|
| **Open Source License** | ✅ Project uses MIT (or another OSI-approved license). Permits building and distribution by the catalog. |
| **No infringement / trademarks** | Game and assets are original or properly licensed; no misuse of third-party IP. |
| **No malicious code** | No code that harms the device or user data. |
| **No illegal activity** | No bypass of Flipper limits; no links/instructions for illegal firmware or use. |
| **UGC policies** | Content complies with [Play Store](https://support.google.com/googleplay/android-developer/answer/9876937) and [App Store](https://developer.apple.com/app-store/review/guidelines/#user-generated-content) UGC policies. |
| **Metadata/source updates** | Respond within 14 days if catalog maintainers request changes; keep contact info in manifest. |
| **Emergency unpublish** | Accept that critical issues (security, copyright, etc.) may lead to unpublish before contact. |

### 1.2 Repository and build (source repo)

| Item | Requirement | Status |
|------|-------------|--------|
| **Build** | Buildable with [uFBT](https://pypi.org/project/ufbt/), compatible with latest **Release** or **Release Candidate** firmware | ✅ Done |
| **Hosting** | Public Git repo; catalog accepts **GitHub** only | ✅ GitHub repo |
| **App icon** | 10×10 px, 1-bit `.png`, in repo | ✅ Done |
| **Screenshots** | At least one; taken with **qFlipper**; do not change resolution or format | ⚠️ To do |
| **README.md** | Markdown: usage, hardware add-ons, etc.; shown as app description in catalog | ✅ Done |
| **changelog.md** | Markdown; versioned changes (see format below) | ✅ Done |

Changelog format (from Contributing.md):

```text
v0.2:
added images

v0.1:
added changelog
```

### 1.3 App manifest (`application.fam`)

Defined in [Flipper App Manifests (FAM)](https://github.com/flipperdevices/flipperzero-firmware/blob/dev/documentation/AppManifests.md). For a **FAP** (external app):

| Field | Purpose |
|-------|---------|
| `appid` | Unique, lowercase, no spaces (e.g. `blackjack`) |
| `apptype` | `FlipperAppType.EXTERNAL` (builds as `.fap`) |
| `name` | Display name in menus (e.g. "Blackjack") |
| `fap_category` | Must be **"Games"** for catalog placement under Games |
| `fap_version` | e.g. `"0.1"`; increment for each catalog update |
| `fap_icon` | 10×10 1-bit `.png` filename |
| `fap_description` | Short description |
| `fap_author` | Author name (and/or contact) |
| `fap_weburl` | Optional; repo or homepage |
| `entry_point` | C entry (e.g. `extern "C"` for C++) |
| `sources` | Optional; default `["*.c*"]` |

### 1.4 Catalog manifest (`manifest.yml`)

Submitted via **pull request** to [flipper-application-catalog](https://github.com/flipperdevices/flipper-application-catalog) in:

`applications/Games/blackjack/manifest.yml`

Required/content fields (see [Manifest.md](https://github.com/flipperdevices/flipper-application-catalog/blob/main/documentation/Manifest.md)):

- `sourcecode.type`: `git`
- `sourcecode.location.origin`: repo URL
- `sourcecode.location.commit_sha`: commit to build (required for each submit/update)
- `screenshots`: list of paths in repo (e.g. `screenshots/ss0.png`)
- `changelog`: e.g. `"@changelog.md"`
- `short_description`: plain text
- `description`: e.g. `"@README.md"`
- `name`, `id` (= appid), `category`: **Games**, `version`

Validate before submitting:

```bash
python3 tools/bundle.py --nolint applications/Games/blackjack/manifest.yml bundle.zip
```

---

## 2. SDK and tooling

### 2.1 Applications SDK (uFBT)

- **Docs**: [uFBT README](https://github.com/flipperdevices/flipperzero-ufbt), [Flipper Downloads](https://flipper.net/pages/downloads)  
- **Install** (macOS/Linux): `python3 -m pip install --upgrade ufbt`  
- **Install** (Windows): `py -m pip install --upgrade ufbt`  
- First run downloads SDK/toolchain from official firmware **release** channel.

### 2.2 Useful commands

| Command | Purpose |
|---------|---------|
| `ufbt create APPID=blackjack` | Scaffold app with `application.fam` and main source (in current dir) |
| `ufbt` | Build app → output in `dist/` |
| `ufbt launch` | Build, upload, and run on Flipper over USB |
| `ufbt -h` | All options |
| `ufbt vscode_dist` | Generate VSCode build/debug config |
| `ufbt update` | Update SDK (use `--channel=release` or `rc` as needed) |

### 2.3 References

- [Flipper Documentation](https://docs.flipper.net/)
- [Flipper Zero Apps (user docs)](https://docs.flipper.net/zero/apps)
- [Publishing to the Apps Catalog](https://developer.flipper.net/flipperzero/doxygen/app_publishing.html)
- [App Manifests (FAM)](https://github.com/flipperdevices/flipperzero-firmware/blob/dev/documentation/AppManifests.md)

---

## 3. Blackjack feature roadmap

### Phase 0 — Project setup and catalog readiness

- [x] Install uFBT and create app scaffold (`ufbt create APPID=blackjack`).
- [x] Add `application.fam` with correct `appid`, `name`, `fap_category="Games"`, `fap_version`, `fap_icon`, `fap_description`, `fap_author`, `entry_point`.
- [x] Add 10×10 1-bit `.png` icon.
- [x] Add `README.md` (how to play, controls, any hardware notes).
- [x] Add `changelog.md` (versioned).
- [x] Ensure build with `ufbt` and compatibility with latest release/RC firmware.
- [ ] Take at least one screenshot with qFlipper; add path to future `manifest.yml`.

### Phase 1 — Core game (MVP)

- [x] **Deck**: 3-deck shoe (156 cards); Fisher–Yates shuffle; burn top + bottom 20.
- [x] **Dealer vs player**: One human player, one dealer (house).
- [x] **Basic flow**: Deal 2 cards each; player hand visible, one dealer card hidden.
- [x] **Player actions**: Hit (one card), Stand (end turn).
- [x] **Dealer logic**: Dealer draws to 16, stands on 17+ (standard house rules).
- [x] **Hand value**: Aces 1 or 11; face cards 10; soft/hard totals shown.
- [x] **Win/loss**: Compare totals; bust loses; blackjack (21 in two cards) wins 3:2; push returns bet.
- [x] **UI**: Simple text/graphics on Flipper (canvas + fonts): cards, totals, “Hit” / “Stand”, result message.
- [x] **Controls**: OK = Hit, Back = Stand; documented in README and in-app Help.

### Phase 2 — Polish and game feel

- [x] **New round**: After win/loss, “Play again?” (e.g. OK = yes, Back = exit to menu).
- [x] **Score or stats**: Win/loss/push count, win rate; scrollable stats (Right on result).
- [x] **Vibration**: On blackjack only (player or dealer); short burst via notification API.
- [x] **Audio**: Hit and Stand only; short tones (Hit ≈400 Hz, Stand ≈600 Hz).
- [ ] **Visual/audio** (optional, future): Card reveal animation or beep on deal; optional short “win/lose” feedback (vibration/sound if API available).
- [x] **Edge cases**: Soft/hard aces; dealer peek; insurance on Ace; clear bust/blackjack messaging.

### Best places for better images (visual assets)

Screen: **128×64 px**, monochrome (1-bit or limited grayscale). Layout is canvas + fonts; images can replace or augment text.

| Place | Phase / screen | Notes |
|-------|----------------|--------|
| **Splash** | PhaseSplash | Full-screen or centered logo; "Blackjack" title + optional card motif. High impact for first impression. |
| **Betting** | PhaseBetting | Chips stack or bet cursor; could show chip denominations ($5–$500) as small sprites. |
| **Card graphics** | PhaseDeal, PlayerTurn, DealerTurn, ShowFinalCards | Replace or augment text cards (e.g. "AS", "QH") with tiny card faces or backs; ~small rect per card, multiple cards in a row—size constraint makes full card art hard; consider minimal suit/rank symbols. |
| **Result overlay** | PhaseResult | Win/lose/push badge or short message graphic; optional confetti or simple border. |
| **Profile menu** | PhaseProfileMenu | Icon per menu item (Continue, New profile, Guest, Practice, Help) or single header image. |
| **Help** | PhaseHelp | Optional illustration for controls (OK=Hit, Back=Stand) or strategy reminder. |

Constraints: 128×64 limits how much can be shown at once; prefer a few strong images over cluttered art. Use `images/` in repo for assets; reference in code where each phase is drawn.

### Phase 3 — Optional extras

- [x] **Double down**: Allow double on initial two cards (one card only). (Up button.)
- [x] **Split**: Split pairs; prompt "Split Pair?"; play two hands; player cards as text during split.
- [x] **Chips/betting**: Virtual chips and bet-before-deal ($3,125 start, $5–$500).
- [x] **Splash menu**: Continue (last profile), New profile, Guest game, Practice mode, Help.
- [x] **Player profiles**: Up to 4 saved profiles (bank + stats) on SD; last-used remembered.
- [x] **Guest game**: Play without profile; "Save to profile?" when leaving (Back from Bet/Result).
- [x] **Practice mode**: Wizard of Odds basic strategy hints during player turn and split prompt.
- [x] **Settings**: Sound on/off, vibration on/off, dealer hits soft 17 on/off; erase all profiles (reset banks to $3,125). Persisted to SD.

### Phase 4 — Catalog submission

- [x] Finalize `application.fam` and repo (icon, README, changelog; version 0.4).
- [x] Add **manifest template** in repo: `manifest.yml` (copy to catalog fork at `applications/Games/blackjack/manifest.yml`; set `origin`, `commit_sha`, and optional `subdir`).
- [x] Document submission steps in README ("Catalog submission" section).
- [ ] **Screenshot** (human): Take at least one with qFlipper → `screenshots/ss0.png` (required).
- [ ] Fork [flipper-application-catalog](https://github.com/flipperdevices/flipper-application-catalog), create branch (e.g. `username_blackjack_0.4`).
- [ ] Copy `manifest.yml` to fork at `applications/Games/blackjack/manifest.yml`; set `commit_sha` and `origin` (and `subdir` if needed).
- [ ] Run catalog validation: `python3 tools/bundle.py --nolint applications/Games/blackjack/manifest.yml bundle.zip`.
- [ ] Open PR, fill template, respond to review within 14 days if asked for changes.

---

## 4. Summary

- **Compliance**: Open source, no infringement/malicious/illegal content, UGC-compliant; repo has icon, screenshots, README, changelog; app buildable with uFBT on release/RC.
- **SDK**: uFBT from [Flipper downloads](https://flipper.net/pages/downloads); docs at [docs.flipper.net](https://docs.flipper.net/) and [uFBT repo](https://github.com/flipperdevices/flipperzero-ufbt).
- **Catalog**: Publish via PR to `flipper-application-catalog` with `manifest.yml` under `applications/Games/blackjack/`; validate before submit.

Next practical steps (human tasks last): Fork catalog repo → copy this repo's `manifest.yml` to `applications/Games/blackjack/manifest.yml` in the fork → set `origin` and `commit_sha` (and `subdir` if app is in a subdirectory) → add `screenshots/ss0.png` (qFlipper screenshot) to the app repo and push → validate with `bundle.py` → open PR. See README "Catalog submission" for the full sequence.
