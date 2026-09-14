# Fable session consolidation and issue replies

Reviewed the three local session transcripts covering the five-issue batch,
stage icons, and typed asset extraction. The bug batch was already merged
as PR #268. The stage-icon changes were also already in main. PR #270
contains the remaining deblob changes and already includes those merged
fixes. Reapplying the earlier branches would duplicate work.

The following issues were still open without a maintainer response to the
fixes when reviewed. No replies have been posted by this review.

| Issue | Reply content / remaining check |
| --- | --- |
| [#215](https://github.com/JRickey/BattleShip/issues/215) | Explain background HD texture preloading and cache-budget controls; request a check with the actual SSB Reloaded pack. Synthetic-pack checks passed, but perceived stutter with the real pack remains unverified. |
| [#240](https://github.com/JRickey/BattleShip/issues/240) | Report restored demo-motion call/jump pointers and replay handicap clamping. The session recorded 60 clean stress runs plus 11 ASan runs including results transitions. Ask for a retest on the resulting build if the original crash persists. |
| [#259](https://github.com/JRickey/BattleShip/issues/259) | Explain improved Android ROM/extraction errors and archive-write checks; ask for the new diagnostic on-device if extraction still fails. All ROM byte orders are supported; do not blame byte order without evidence. |
| [#260](https://github.com/JRickey/BattleShip/issues/260) | Confirm the N64 logo is automatically skipped on cold boot and attract-loop restarts. No setting is required. |
| [#264](https://github.com/JRickey/BattleShip/issues/264) | Explain the bonus-stage training wallpaper bounds fix and request a manual test of the page-two training selection on the new build. |
| [#267](https://github.com/JRickey/BattleShip/issues/267) | Confirm archive-generated stage icons, restored arrows/nameplates and the Bonus Stages toggle. The prior session includes user visual approval. |

Avoid presenting outstanding on-device or real-pack checks as completed.
The contributor's display-list and build reports are tracked separately in
the asset-build and replacement-loader investigation notes.
