# BSD Custom Build Overlay

Big Sky Dynamics changes to QGroundControl live here, not in `src/`. QGC's
root `CMakeLists.txt` picks this folder up automatically because it is named
`custom` (see `QGC_CUSTOM_DIR` in `cmake/CustomOptions.cmake`), so a normal
`just configure && just build` produces the BSD build. Delete `build/` the
first time after adding or removing this folder.

This is intentionally much smaller than `custom-example/`. That example turns
off the ArduPilot plugin factory and forces PX4 defaults; the first BSD
aircraft is an ArduPilot helicopter, so none of that is copied.

## Contents

| Path | Purpose |
|---|---|
| `cmake/CustomOverrides.cmake` | Required by the build; keeps stock app name so settings are shared with a plain QGC build |
| `src/CustomPlugin.cc/.h` | Minimal `QGCCorePlugin` subclass that installs the resource override URL interceptor |
| `src/PlanView/PlanToolBarIndicators.qml` | Override: adds a **Map** button (provider and type picker) to the Plan toolbar |

## Adding an override

1. Copy the upstream file from `src/...` into `custom/src/...` and edit it.
2. In `CMakeLists.txt`, add it to `CUSTOM_QML_OVERRIDES` with a
   `QT_RESOURCE_ALIAS` equal to its upstream path under `/qml`
   (for example `QGroundControl/PlanView/PlanToolBarIndicators.qml`).
3. After each upstream rebase, diff the override against its upstream source
   and carry across any upstream changes. Overrides are full-file copies, so
   this is the maintenance cost of keeping `src/` untouched.

Upstream documentation: `docs/en/qgc-dev-guide/custom_build/`.
