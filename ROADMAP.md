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

- [ ] **Deck**: Standard 52 cards; shuffle (e.g. Fisher–Yates).
- [ ] **Dealer vs player**: One human player, one dealer (house).
- [ ] **Basic flow**: Deal 2 cards each; player hand visible, one dealer card hidden.
- [ ] **Player actions**: Hit (one card), Stand (end turn).
- [ ] **Dealer logic**: Dealer draws to 16, stands on 17+ (standard house rules).
- [ ] **Hand value**: Aces 1 or 11; face cards 10; no splitting/doubling in MVP.
- [ ] **Win/loss**: Compare totals; bust (over 21) loses; blackjack (21 in two cards) wins (optional: 3:2 payout in later phase).
- [ ] **UI**: Simple text/graphics on Flipper (canvas + fonts): cards, totals, “Hit” / “Stand”, result message.
- [ ] **Controls**: Map Flipper buttons (e.g. OK = Hit, Back = Stand; or Left/Right + OK) and document in README.

### Phase 2 — Polish and game feel

- [ ] **New round**: After win/loss, “Play again?” (e.g. OK = yes, Back = exit to menu).
- [ ] **Score or stats**: Optional on-screen win/loss count (or sessions played) for the session.
- [ ] **Visual/audio**: Card reveal animation or beep on deal/hit; optional short “win/lose” feedback (vibration/sound if API available).
- [ ] **Edge cases**: Double aces (soft 22) treated as 12; clear bust/blackjack messaging.

### Phase 3 — Optional extras

- [x] **Double down**: Allow double on initial two cards (one card only). (Up button.)
- [x] **Split**: Split pairs; prompt "Split Pair?"; play two hands; player cards as text during split.
- [x] **Chips/betting**: Virtual chips and bet-before-deal ($3,125 start, $5–$500).
- [x] **Splash menu**: Continue (last profile), New profile, Guest game, Practice mode, Help.
- [x] **Player profiles**: Up to 4 saved profiles (bank + stats) on SD; last-used remembered.
- [x] **Guest game**: Play without profile; "Save to profile?" when leaving (Back from Bet/Result).
- [x] **Practice mode**: Wizard of Odds basic strategy hints during player turn and split prompt.
- [ ] **Settings**: Optional sound on/off or dealer rules (future).

### Phase 4 — Catalog submission

- [ ] **Screenshot**: Take at least one with qFlipper → `screenshots/ss0.png` (required).
- [x] Finalize `application.fam` and repo (icon, README, changelog; version 0.4).
- [ ] Fork [flipper-application-catalog](https://github.com/flipperdevices/flipper-application-catalog), create branch (e.g. `username_blackjack_0.4`).
- [ ] Add `applications/Games/blackjack/manifest.yml` with `sourcecode`, `screenshots`, `changelog`, `description`, `short_description`, `name`, `id`, `category`, `version` (0.4).
- [ ] Run catalog validation: `python3 tools/bundle.py --nolint applications/Games/blackjack/manifest.yml bundle.zip`.
- [ ] Open PR, fill template, respond to review within 14 days if asked for changes.

---

## 4. Summary

- **Compliance**: Open source, no infringement/malicious/illegal content, UGC-compliant; repo has icon, screenshots, README, changelog; app buildable with uFBT on release/RC.
- **SDK**: uFBT from [Flipper downloads](https://flipper.net/pages/downloads); docs at [docs.flipper.net](https://docs.flipper.net/) and [uFBT repo](https://github.com/flipperdevices/flipperzero-ufbt).
- **Catalog**: Publish via PR to `flipper-application-catalog` with `manifest.yml` under `applications/Games/blackjack/`; validate before submit.

Next practical step: **Phase 4** — take qFlipper screenshot → `screenshots/ss0.png`, then fork catalog repo, add `applications/Games/blackjack/manifest.yml`, validate, and open PR.
