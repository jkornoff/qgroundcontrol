# BSD / SAM Notes -- QGroundControl Customization

Working notes on customizing QGC for SAM survey/BVLOS ops -- custom UI, DRONETAG
Firelight traffic overlay, mission-planner dispatch tie-in. Not for upstream --
BSD-internal only. Updated 2026-08-20.

## Repo setup

- Forked from `mavlink/qgroundcontrol` to `jkornoff/qgroundcontrol`.
- `origin` = fork, `upstream` = `mavlink/qgroundcontrol`. Pull `upstream/master`
  and merge to keep current; customizations should stay additive (new files,
  isolated overrides) to keep merges painless.
- No git submodules in current QGC -- deps are CMake FetchContent/vcpkg.
- `just configure` / `just build` / `just check` -- see `AGENTS.md` for full
  build workflow and coding rules before touching source.

## First build on a clean macOS machine -- gotchas hit

`just setup` (deps + submodules + configure + build) is not one clean shot on
a fresh machine. Hit all of these in sequence today; fixes below so next time
(new machine, clean checkout) is a five-minute read instead of a repeat.

1. **`just deps` GStreamer step needs a real terminal for sudo.** The
   installer script shells out to `sudo installer -pkg ... -target /`. Run
   `just setup`/`just deps` from an actual interactive terminal (not a
   backgrounded/non-tty shell) so sudo can prompt normally. A GUI
   `osascript ... with administrator privileges` password prompt does NOT
   pre-authorize a later plain `sudo` call in a different shell -- it's a
   separate one-off authorization, not the standard sudo timestamp cache.
   Don't bother trying to pre-auth sudo for a background run; just run it
   interactively.
2. **GStreamer download lives in a `tempfile.TemporaryDirectory()`** in
   `tools/setup/install_dependencies/_macos.py` -- wiped on ANY exit,
   success or failure. If that step fails (network blip, missing Python
   module), the downloaded `.pkg`s are gone and it redownloads from scratch
   next attempt. Nothing to fix on our end, just know it's not resumable.
3. **`tools/setup/install_python.py dev`** needed to run once before `just
   deps` would fully succeed -- the GStreamer downloader imports `httpx`,
   which isn't a stdlib/preinstalled module.
4. **Qt is NOT installed by `just deps`.** It only does Homebrew formulae +
   GStreamer. Qt SDK is a separate manual step via `tools/setup/install_qt.py`
   (aqtinstall wrapper), using the exact version/modules from
   `.github/build-config.json` (currently Qt 6.11.1):
   ```
   python3 tools/setup/install_qt.py install --version 6.11.1 --host mac \
     --arch clang_64 --modules "qtgraphs qtlocation qtpositioning qtspeech \
     qtmultimedia qtserialport qtimageformats qtshadertools qtconnectivity \
     qtquick3d qtsensors qtscxml qtwebsockets qthttpserver" --outdir .qt
   ```
   Then `export QT_ROOT_DIR="$(pwd)/.qt/6.11.1/macos"` before `just configure`
   (arch `clang_64` resolves to an on-disk dir literally named `macos`).
   Writes to a persistent `--outdir`, not a tempdir -- a failed/retried run
   does NOT redownload modules already present, unlike the GStreamer step above.
5. **`install_qt.py` installs `aqtinstall` into the project's `.venv`**, not
   system-wide. If you run `python3 tools/setup/install_qt.py` directly
   (not via `just`), `.venv/bin` needs to be on PATH first (`source
   .venv/bin/activate`) or `shutil.which("aqt")` fails with "aqtinstall not
   found after pip install" even though it installed fine.
6. **The project `.venv`'s Python (uv-managed, 3.10.x) has no `pip` module**
   by default -- uv doesn't need pip itself, but the vendored MAVLink CPM
   subbuild's CMakeLists shells out to `python -m pip` during `just
   configure`. Fix once: `uv pip install --python .venv/bin/python pip`.
7. **Homebrew package downloads are cached** (`~/Library/Caches/Homebrew/downloads`)
   and ccache is installed as part of `deps` specifically so a failed/retried
   `just build` doesn't recompile from scratch -- only GStreamer's tempdir
   and (partially) Qt's fresh-module downloads are the redo-prone steps.
8. **The real built app isn't at `build/QGroundControl.app`** (that's a
   partial qml-resources staging path from the install step) -- it's at
   `build/Debug/QGroundControl.app` (Debug preset). `open
   build/Debug/QGroundControl.app` or `just run` to launch it.

Bottom line: this is all one-time toolchain bootstrap cost. Once `.venv`,
`.qt/`, and `build/` exist, a QML-only edit is `just build` (ccache-backed,
QML doesn't need C++ recompilation) -- a couple minutes, not an afternoon.

## Custom Build mechanism

QGC's sanctioned way to re-skin/extend without forking core logic:
`custom-example/` at repo root -- a self-contained sibling project (own
`CustomOverrides.cmake`, `CustomPlugin.cc/.h`, QML overrides like
`FlyViewCustomLayer.qml`, `CustomGuidedActionsController.qml`). Copy/adapt this
rather than editing `src/` directly. This is how companies white-label QGC
(ModalAI, Auterion, ag/survey vendors) without maintaining a hard fork.

## ADS-B / traffic display -- two native ingestion paths

1. **MAVLink `ADSB_VEHICLE` messages** -- via `ADSBVehicleManager`
   (`src/Vehicle/Vehicle.cc`). Fed by an onboard receiver wired to the flight
   controller; traffic near *that* aircraft, from *its* sensor.
2. **Network ADS-B server (SBS/BaseStation format)** -- Settings > General >
   ADSB Server. Independent of any vehicle connection. QGC opens a TCP socket
   to `host:port` and expects SBS-protocol text lines (same format
   dump1090/readsb output). This is the path we prototyped.

RemoteID does NOT fit the SBS schema (no ICAO24/squawk/altitude in that sense
-- it's operator ID, control station location, serial number). SBS path is
ADS-B only; RemoteID needs a custom QML overlay pulling from a separate API/
schema if we want it rendered on QGC's map.

## Prototype: adsb.lol -> Pi5 -> SBS -> QGC

Working end-to-end as of today. Script: `adsb_sbs_bridge.py` (stdlib-only
Python, polls `https://api.adsb.lol/v2/point/{lat}/{lon}/{radius_nm}`,
reformats to SBS, serves on TCP :30003). Deployed on `pi5-b` (Tailscale
100.69.214.58, SSH creds in Doppler as `PI5_B_SSH_USER`/`PI5_B_SSH_PASSWORD`/
`PI5_B_HOST`).

QGC config: Settings > General > ADSB Server > Enable, host = Pi's Tailscale
IP or `pi5-b.local`, port 30003 (fixed, no separate port setting needed
beyond what's shown -- QGC's field matches dump1090's standard port).

### Gotchas hit today

- **QGC doesn't auto-reconnect on setting change.** If you edit the host
  field while "Enable" is already on, or the far end drops/restarts, QGC
  surfaces a dialog ("ADSB Server Error: remote host closed connection")
  rather than silently retrying. Toggle Enable off/on to force a fresh
  connect.
- **No vehicle connection required** for the ADSB Server traffic overlay --
  it's independent of the flight link. But with no vehicle connected, QGC
  has nothing to auto-center the map on, so you have to manually pan/zoom to
  where the traffic actually is.
- **Silent parse failures are easy to hit.** QGC's SBS parser
  (`src/ADSB/ADSBTCPLink.cc`, `_parseAndEmitLocation`) requires SPECIFIC
  fields to parse as valid numbers or it drops the whole position message
  with no error surfaced anywhere:
  - `values.at(11)` altitude, `values.at(14)` lat, `values.at(15)` lon --
    obviously required.
  - `values.at(19)` -- the "alert" field -- ALSO required to parse as an int.
    An empty string here (which is valid/common in real SBS feeds) silently
    kills the entire position update. Burned an hour on this: connection was
    live, data was flowing (confirmed via `nc` and `ss -tn` showing
    ESTABLISHED), but zero icons on the map because this one field was `""`
    instead of `"0"`. **Always send `"0"` not `""` for alert/emergency/spi
    fields if you don't have real values.**
  - Velocity messages (`MSG,4`) only need speed (`values.at(12)`) and
    heading (`values.at(13)`) to parse -- much more forgiving, which is why
    those showed up fine while position never did. Good diagnostic signal if
    this happens again: velocity-derived behavior working + position never
    appearing = check the alert field first.

### Display quality

Stock traffic icon (`src/FlightMap/MapItems/ADSBVehicleMapItem.qml`) is
minimal -- single rotating icon (`AwarenessAircraft.svg` /
`AlertAircraft.svg` on alert flag) + altitude/callsign text label. No
color-coding, no vertical-rate indicator, no trail, no clustering, no
ADS-B-vs-RemoteID visual distinction. It's a leaf QML file bound to a plain
data model (`ADSBVehicleManager` exposes location/heading/velocity/
verticalVel/alert/callsign/icao as `Q_PROPERTY`s) -- straightforward to
override via Custom Build if we ever want to, low merge-conflict risk since
it's small and isolated. Not worth it right now.

## Decision: two-screen airspace awareness, not a QGC rebuild

DRONETAG Firelight already does this better and isn't worth rebuilding inside
QGC. Plan: run Firelight on its own screen for full situational awareness,
keep QGC untouched as the flight-critical GCS. Common pattern for BVLOS ops
setups generally (GCS + separate awareness display, not fused into one pane).

**The ADSB Server hook remains useful even with two screens**, as a one-way
alert signal FROM Firelight INTO QGC:

- Firelight stays the fusion/risk authority (proximity, geofence, RemoteID
  correlation -- whatever its model already does).
- When Firelight flags a real threat, set that track's `alert` field to `1`
  in the SBS line sent to QGC.
- `alert` is a pure pass-through in QGC (`src/ADSB/ADSBVehicle.h:26`,
  `Q_PROPERTY(bool alert)`) -- QGC does zero proximity computation of its
  own, it just trusts what the feed says.
- **Current limit:** stock QGC only swaps the map icon to
  `AlertAircraft.svg` on `alert=1` -- confirmed no toolbar indicator, no
  sound, no banner tied to it (checked `ADSBVehicleManager.cc`,
  `Toolbar/`, `FlightMap/`). Quiet signal as shipped. Making it loud
  (audible warning, banner) would be a small, scoped Custom Build QML
  addition reacting to a property that already exists -- not touching core
  logic.

So: Firelight screen for the real picture, QGC's own map also lights up red
on flagged threats as a belt-and-suspenders layer for whoever's only looking
at the GCS.

## Architecture (target, not yet built)

```
DroneTag Scout --(Ethernet)--> Pi5 (field)
                                  |-- parse/reformat locally
                                  |-- SBS feed on LAN --------> QGC (ADS-B, native, incl. alert flag)
                                  |-- RemoteID feed on LAN ---> Firelight display (custom, richer UI)
                                  '-- spool/queue --(Starlink, async, store-and-forward)--> server (recording only)
```

Starlink kept OUT of the local-awareness critical path -- only used for
non-time-critical recording upload. Local LAN (Scout + Pi5 + QGC laptop +
Firelight display) is self-contained and doesn't depend on the home network
or a satellite link.

## Open questions / next steps

- Does the physical DroneTag Scout expose a decoded API (REST/WebSocket/MQTT)
  over its Ethernet link, or raw RF frames needing our own decode? Determines
  how much work the Pi5 bridge actually is for the real hardware (vs. this
  adsb.lol prototype which had decoded JSON handed to it).
- DRONETAG production is currently UM's -- BSD has the code but no deploy
  target. Confirm whether this SAM-side pipeline reuses anything from that
  instance or is a clean separate build.
- mission-planner dispatch tie-in (pull mission, push status) not started --
  separate from the traffic-display work above.
- If/when the display quality or dispatch panel work starts: scaffold off
  `custom-example/`, keep it additive so `upstream` merges stay clean.
