# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

DayZ Community Offline Mode (COM) — an Enforce Script mission mod that loads DayZ in singleplayer with editor/admin tooling. It is **not** a traditional codebase with a build step: the DayZ engine compiles `.c` files at runtime when launched with `-filePatching`. There is no compiler, linter, package manager, or test framework in this repo.

## Running / "deploying"

The three `deploy<Map>.bat` scripts at the repo root are the primary dev loop:

- `deployChernarusPlus.bat` / `deployEnoch.bat` / `deployNamalsk.bat`
- Each reads the DayZ install dir from `HKLM\SOFTWARE\Wow6432Node\bohemia interactive\Dayz\main`, deletes any prior copy of the matching mission under `<DayZ>\Missions\`, `xcopy`'s the corresponding `Missions/DayZCommunityOfflineMode.<Map>` folder into the game's Missions dir, then invokes that mission's inner `DayZCommunityOfflineMode.bat`.
- The inner `.bat` kills `DayZ_x64.exe`, wipes the `storage_-1` save folder (full reset every launch — persistency is off by default), then launches `DayZ_x64.exe -mission=.\Missions\DayZCommunityOfflineMode.<Map> -nosplash -noPause -noBenchmark -filePatching -doLogs -scriptDebug=true`.

Iteration: edit files in this repo → run the matching `.bat` → game launches with the new code. `-filePatching` is required; without it the loose `.c`/`.xml` files are ignored. Logs land in `%localappdata%/DayZ` (`script.log`, `crash.log`, etc.).

## Map missions are independent copies

`Missions/` contains three sibling missions:

- `DayZCommunityOfflineMode.ChernarusPlus`
- `DayZCommunityOfflineMode.Enoch` (Livonia)
- `DayZCommunityOfflineMode.Namalsk` (requires the Namalsk mod)

**Each mission carries its own full copy of `core/` and its own economy XMLs (`db/`, `env/`, `cfg*.xml`, `mapgroup*.xml`).** The Enforce `#include` paths are absolute under `$CurrentDir:missions\\DayZCommunityOfflineMode.<Map>\\...` and are hardcoded per map. Consequence: a fix to a module file (e.g. `core/modules/ComEditor/...`) must be mirrored across all three mission folders. There is no shared/symlinked core. When changing scripts, search/replace across all three trees and verify the include paths inside any new file match its mission folder.

## Script architecture

Entry point per mission: `init.c` defines `DISABLE_PERSISTENCY`, `#include`s `core/BaseModuleInclude.c`, and overrides `CreateCustomMission()` to return `CommunityOfflineClient` (or `CommunityOfflineServer` in MP — unused in offline).

`core/BaseModuleInclude.c` is the include manifest — pulls in `ModuleManager.c`, `StaticFunctions.c`, `CommunityOfflineClient.c`, `CommunityOfflineServer.c`. `ModuleManager.c` defines `COM_MODULES_OLDLOADING`, then conditionally `#include`s every module's `module.c` (legacy "old loading" path; the alternative single-file-per-module registration path is dead code today).

`CommunityOfflineClient extends MissionGameplay` is the runtime root:
- `OnInit()` — calls `InitHive()` (toggle via `HIVE_ENABLED` field — controls loot/zombie spawn), `SetupWeather()` (clamps overcast/rain/fog to 0), `SpawnPlayer()` via `COM_CreateCustomDefaultCharacter()`, sets `$saves:CommunityOfflineMode\\` for camera tools.
- `OnMissionStart()` / `OnUpdate()` / `OnMissionFinish()` — delegate to `COM_GetModuleManager()`.
- `CreateScriptedMenu()` — routes `EditorMenu.MENU_ID` to the object editor.

`ModuleManager` (singleton via `g_com_ModuleManager` + `COM_GetModuleManager()` / `NewModuleManager()`):
- Registers modules guarded by `#ifdef MODULE_*` defines (each module's `module.c` defines its flag, e.g. `#define MODULE_COM_EDITOR`).
- Each frame in `OnUpdate()`: walks every module's `KeyMouseBinding` set, queries `GetUApi()` for press/release/hold/doubleclick/value, and dispatches via `GameScript.CallFunction()` to the bound method. Bindings are skipped when an `EditBoxWidget` has focus or when `CanBeUsedInMenu()` is false and a menu is open.

Module pattern (`core/Module.c`): subclass `Module`, override `RegisterKeyMouseBindings()` to call `RegisterKeyMouseBinding(...)`, override the `onMission*` / `onUpdate` / `onKey*` / `onMouse*` hooks as needed. A binding is `{ UAInputName, ActionType, callback method, target Module type, key combos, can-be-used-in-menu }`.

Active modules (registered in `RegisterModules()`):
- `ComEditor` — object editor / spawner / scene save (the largest module: `gui/`, `scene/`, `ObjectEditor.c`).
- `CameraTool` — free camera (INSERT key) + camera tool menus.
- `ComKeyBinds` — key rebinding UI.
- `ComMenu` — top toolbar (Y / Z key).
- `DebugMonitor` — on-screen debug overlay (B key).
- `BarrelCrosshair` — weapon barrel-direction crosshair.
- `AdminTool` — position/teleport menus (its `gui/` is also pulled in by `ComEditor/module.c` directly — there is cross-module include coupling, watch for it when moving files).
- `Persistency` — save/load characters. **Currently disabled**: the `module.c` include is commented out in `ModuleManager.c` and `init.c` defines `DISABLE_PERSISTENCY`. Re-enabling means uncommenting the include, removing `DISABLE_PERSISTENCY`, and verifying `MODULE_PERSISTENCY` registration.

`StaticFunctions.c` — global `COM_*` helpers (vector/float formatting, config-tree walks for spawn pickers, `COM_CreateCustomDefaultCharacter`, `COM_Message`, etc.).

## Economy / world data

The `db/`, `env/`, `cfg*.xml`, `mapgroup*.xml`, `cfg*.json` files are stock DayZ central-economy configs copied per map. They control loot tables, animal/zombie territories, weather, spawn points, event groups, etc. Editing them is a content change, not a script change — no rebuild needed, but the game must be restarted (the deploy `.bat` does this).

## When making changes

- Cross-mission parity: a script edit in one mission almost always needs to be replicated in the other two, including any new `#include` lines (paths are mission-name-prefixed).
- Don't forget to update `ModuleManager.c`'s `#include` block when adding/removing a module's `module.c`.
- Persistency is off by default — the `storage_-1` wipe in the inner `.bat` makes any in-game state non-durable across launches.
- The wiki ([Add custom objects to your server or mission](https://github.com/Arkensor/DayZCommunityOfflineMode/wiki/Add-custom-objects-to-your-server-or-mission), [Toggle loot and infected spawn](https://github.com/Arkensor/DayZCommunityOfflineMode/wiki/Toggle-loot-and-infected-spawn)) is the user-facing reference; the README lists the in-game keybinds (Y, X, END, O, R, P, B, INSERT).
