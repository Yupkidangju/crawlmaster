# IMPLEMENTATION_SUMMARY.md (현재 구현 및 파일 책임)

작성 기준: 2026-09-03  
제품 상태: 한 층짜리 pre-release demo candidate. 상용 PASS 아님.

## 1. 런타임 흐름

```text
main
  -> Game (window, font, event/update/draw loop)
  -> GameStateManager
       TitleState
         -> TownState
              -> CharacterInfoState / SettingsState
              -> DungeonState
                   -> CombatState(normal tiers)
                   -> CombatState(final boss)
                        -> VictoryState
                   -> GameOverState on TPK
```

`replaceAll()`은 TPK/종결처럼 session root를 바꾸는 전이에서 stale state를 모두 제거한다.

## 2. 실제 파일 책임

### Core

- `Game`: SFML window와 state loop, 실행 파일 상대 resource 기동
- `GameStateManager`: push/pop/top replace/root replace
- `LocalizationManager`: locale catalog/placeholder, localized content/log, text scale/high contrast, atomic config
- `Persistence`: per-user path, fsync, backup, atomic replace, corrupt quarantine
- `ResourceLocator`: build/install asset 위치
- `SessionRng`: 한 세션 seed, raw engine draw count와 단일 `std::mt19937` stream

### Model

- `Character`: stats/status/skills/equipment와 class/STR/two-handed invariant
- `Party`: 최대 4인, inventory/gold/quest/campaign, schema-v2 save와 v1 migration
- `CombatRules`: 일반 공격과 공격형 Skill이 공유하는 natural roll, Bless, weapon/extra dice, resistance, tier gold
- `CombatActionRules`: 소모품이 실제 효과를 낼 수 있는지 판정
- `RecruitmentDraft`: preview/reroll/one-time confirm
- `DungeonMap`: seeded DFS/loops, Door landmark, farthest BossGate, progress
- `ItemFactory`, `MonsterFactory`, `Quest`: canonical IDs, acquisition/drop/reward registry
- `Skill`: 성공 여부를 반환해 invalid/no-effect action의 자원 소비를 막음

### View/Controller

- `DungeonRenderer`: 1인칭 wireframe, automap, actual Party HUD
- `PartyHudSnapshot`: renderer에 전달하는 immutable 0~4인 상태
- Town/Combat/CharacterInfo/Settings는 현재 각 State가 직접 SFML TUI를 그린다. 존재하지 않는 TownRenderer/CombatRenderer/UIRenderer를 현재 파일로 주장하지 않는다.
- `TitleState`: New/Continue/Settings/Exit와 New confirmation
- `TownState`: 모집 preview, 판매/해고 확인, 상점/교회/3 quest
- `DungeonState`: movement, FOW, tier encounter, landmark/boss transition
- `CombatState`: 명시적 item/ally target, combat/reward/drop, TPK/victory transition
- `GameOverState`, `VictoryState`: 지속 결과 화면

## 3. 저장 및 실패 경계

- schema v2는 camelCase/nested equipment를 canonical로 쓴다.
- v1 snake_case는 읽어서 다음 저장 때 v2로 쓴다.
- save/config는 same-directory temp -> flush/fsync -> backup rotate -> atomic replace 순서다.
- 손상 파일은 quarantine하고 backup을 읽는다. 자동 New Game/reset은 하지 않는다.
- 활성 dungeon 좌표는 저장하지 않으며 town checkpoint와 campaign completion을 저장한다.
- checkpoint는 `lastSessionSeed`와 `sessionRngDrawCount`를 함께 저장하며 Continue가 실제 global stream을 복원한다.
- TPK는 정상 save를 삭제하지 않는다.

## 4. 검증 표면

- Release-safe `CHECK` 기반 TestHarness
- `CrawlmasterRuntime` production library를 앱과 controller transcript test가 함께 링크
- Linux CTest 13개: Resource/HUD/Combat/Content/Agency/Localization/FontRaster/독립 process RNG/Controller/UI state raster
- Linux Release install/CPack/arbitrary-CWD Xvfb smoke
- 로컬 MinGW 전체 target compile과 Windows ZIP/checksum/PE import 정적 검사
- artifact SPDX generator, OSV/Grype known-vulnerability gate와 GitHub SBOM/provenance attestation workflow

## 5. 미검증/잔여 gate

- Windows Server 2022 hosted MSVC build/test/package/5초 startup: run `33786241695`에서 Verified
- GitHub native build/SBOM attestation, clean Windows 10/11 VC++ runtime 및 high-DPI/장시간 실기: `UNVERIFIED`
- 30~60분 완주 시간과 장기 밸런스: `UNVERIFIED`
- 5 locale × 3 scale production state/substate raster는 Linux에서 생성·판독. OS high-DPI는 `UNVERIFIED`
- 폰트 hash/upstream/license는 `FONT_PROVENANCE.md`로 검증. 제품 legal/support identity는 `Human Review Required`
