# Mobile / Companion Strategy — platform independence is a feature

The desktop app is the product core. "Mobile" here means *get the data in front of the
driver*, not *join another platform ecosystem*. Decision record for the repo, per the
license's spirit: no dependence on any corporate dev platform, no app-store gatekeeper,
no telemetry, no account.

## Options, honestly weighed

### Option 1 — MDU headless + local web dashboard (**recommended**)

MDU gains a headless mode that serves the same session model over **HTTP + WebSocket JSON**
on a LAN-only endpoint (mDNS: `mdu.local`), with a static HTML/JS dashboard served by the
app itself. The phone's *existing* browser is the client — Android, de-Googled (Graphene),
iOS, postmarketOS, a laptop, anything with a browser.

- Zero new platform commitment: no Kotlin/Jetpack/Compose, no Play Store, no Firebase, no
  iOS build for the same reason
- One codebase stays true: `MdFrameDecoder` / `SessionLog` / `PlotDataModel` are already
  platform-agnostic; only the transport layer is new
- The ESP-NOW link is serial-over-radio anyway: an ESP32-class receiver with WiFi in AP
  mode can carry the same JSON stream — the phone never needs to be "Android", it needs
  a browser
- Distribution = a URL on the vehicle's own LAN. Nothing to install on the phone. Ever.

### Option 2 — Qt for Android (possible, not chosen)

Qt supports Android natively: the *entire* app stays C++/Qt (Qt LGPL-v3 is a license, not
a platform — that distinction matters), and there is zero Kotlin/Jetpack in app code —
but Gradle/SDK packaging (Google's build chain) and APK distribution (Play or APK sideload)
remain unavoidable underneath. Viable if someone insists on a store-installable APK;
the `android/` placeholder remains for that day.

### Option 3 — Linux-phone native (postmarketOS/Debian mobile)

The true escape hatch: a Qt app running natively on a Linux phone. Honestly niche —
today's supported phones are scarce and the modem/USB-C-OTG stack is rough. Listed for
completeness; the web dashboard makes this moot for most pilots.

## Decision

**Primary path = Option 1 (LAN JSON API + bundled web UI).** It is the only option that
adds zero platform dependency, works on every phone already in the wild (which is the
actual reason Android was ever on the table: ubiquity, not ideology), and shares 100% of
the tested decode/record/replay core. Qt-for-Android stays parked as Option 2 behind the
existing `android/` placeholder.

## Implementation sketch (when scheduled)

- `MduServer` (Qt): QHttpServer-style tiny HTTP + WebSocket; reuses `UnifiedDataStore`
  signals → JSON frames `{id,name,unit,value,ts}`; read-only in v1 (dashboard = telemetry)
- static web assets in `resources/web/` — plain HTML/JS, no framework
- bind to LAN interface only; no cloud relay exists or can exist; `--serve <port>` flag
- unit tests: encode/decode of the JSON frame schema; authn deliberately out of scope for
  a point-to-point vehicle LAN (document this)
