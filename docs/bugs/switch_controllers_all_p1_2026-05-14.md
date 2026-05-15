# switch_controllers_all_p1_2026-05-14

**RESOLVED (Switch multiplayer input routing)** — In Switch builds, all connected controllers drove only player 1 by default.

## Symptoms

- Multiple Joy-Con / Pro Controllers connected.
- Gameplay input from every physical pad affected only P1.
- P2-P4 could not be controlled out of the box.

## Root Cause

Two default behaviors combined into a Switch-specific usability bug:

1. `ConnectedPhysicalDeviceManager::RefreshConnectedSDLGamepads` defaulted every newly discovered SDL gamepad to port 0 by inserting each instance ID into ignore-sets for ports 1-3.
2. `ControlDeck::Init` only seeded default SDL mappings for port 0 on fresh config.

On desktop, users can manually toggle per-port device routing in the input editor. On Switch, that workflow is not practical, so defaults effectively locked all pads to P1.

## Fix

- `ConnectedPhysicalDeviceManager.cpp`:
  - Under `__SWITCH__`, assign each connected gamepad to exactly one port by default.
  - Use SDL player index when available; otherwise fall back to connection order modulo 4.
  - Clear and rebuild per-port ignore maps on refresh.
- `ControlDeck.cpp`:
  - Under `__SWITCH__`, seed default SDL mappings for every unconfigured controller port (while keeping keyboard/mouse defaults only on port 0).

## Follow-up (Detached Joy-Cons)

Detached Joy-Cons still showed a partial failure mode after the routing fix: buttons worked and per-player assignment was correct, but analog movement could fail specifically in character select.

Root cause was mapping coverage on Switch: the loaded controller DB entries for Joy-Cons were commonly tagged for desktop platforms, so SDL did not always expose left-stick axes for detached Joy-Con GUID variants on Switch.

Additional fix:

- `os.cpp` (`osContInit`): inject Switch-scoped fallback mappings at startup via `SDL_GameControllerAddMapping` for combined/split Joy-Con GUID variants, including hat-based split variants (`030000...` and `050000...`).

This keeps desktop mapping behavior unchanged and only patches Switch startup defaults when DB platform tags are missing or mismatched.

## Validation

- Ran `./scripts/build-switch.sh --skip-extract`.
- Build completed successfully and produced `build-switch/BattleShip.nro`.
