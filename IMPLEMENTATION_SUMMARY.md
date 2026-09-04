# IMPLEMENTATION_SUMMARY.md (현재 구현 및 파일 책임)

작성 기준: 2026-09-04
제품 상태: 3층 영속 던전 pre-release demo candidate. 상용 PASS 아님.

## 1. 런타임 흐름

```text
main
  -> Game (window, font, event/update/draw loop)
  -> GameStateManager
       TitleState
         -> TownState
              -> CharacterCreationState / CharacterInfoState / SettingsState
              -> QuestJournalState
              -> DungeonState (floors 1..3)
                   -> CombatState(normal tiers)
                   -> CombatState(final boss)
                        -> VictoryState
                   -> GameOverState on TPK
       ShutdownState (save 실패 또는 recovery-pending 복구 재시도/명시 종료)
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

- `Character`: 이름/나이/성별, stats/status/skills/equipment와 class/STR/two-handed invariant
- `Party`: 최대 4인, inventory/key item/gold/quest/campaign/영속 월드, schema-v4 save와 v1~v3 migration
- `CombatRules`: 일반 공격과 Skill이 공유하는 natural roll, Bless, class trait, weapon/extra dice, resistance, tier gold
- `CombatActionRules`: 소모품이 실제 효과를 낼 수 있는지 판정
- `RecruitmentDraft`: 신원 검증, seed 기반 4d6 리롤, 가중 10포인트 배분, one-time confirm
- `DungeonMap`/`DungeonWorld`: 층별 seeded DFS/loops, 계단·목표 배치, fog/object snapshot
- `ItemFactory`, `MonsterFactory`, `Quest`: canonical IDs, acquisition/drop/reward registry
- `Skill`: 성공 여부를 반환해 invalid/no-effect action의 자원 소비를 막음

### View/Controller

- `DungeonRenderer`: 1인칭 wireframe, 청록색 방향 마커 automap, actual Party HUD
- `PartyHudSnapshot`: renderer에 전달하는 immutable 0~4인 상태
- Town/Combat/CharacterInfo/Settings는 현재 각 State가 직접 SFML TUI를 그린다. 존재하지 않는 TownRenderer/CombatRenderer/UIRenderer를 현재 파일로 주장하지 않는다.
- `TitleState`: New/Continue/Settings/Exit와 New confirmation
- `TownState`: 전용 캐릭터 생성 진입, 판매/해고 확인, 상점/교회/목적형 quest board
- `QuestJournalState`: 활성/보고 가능 퀘스트, 층 단서와 중요품 조회
- `CharacterCreationState`: 이름·나이·성별·직업, 능력치 리롤·배분, 최종 저장 확인
- `DungeonState`: movement, FOW, tier encounter, landmark/boss transition
- `CombatState`: 명시적 item/ally target, combat/reward/drop, TPK/victory transition
- `ShutdownState`: 일반 저장 실패 재시도와 recovery-pending load 재시도, 명시적 무저장 종료
- `GameOverState`, `VictoryState`: 지속 결과 화면

## 3. 저장 및 실패 경계

- save schema v4는 파티, 중요품, 퀘스트 상태와 3층 전체 월드 snapshot을 canonical로 쓴다.
- v1~v3 save는 파티 진행을 보존하고 저장 seed에서 월드를 결정론적으로 생성한다. 없는 신원은 미상으로 보존한다.
- save/config는 same-directory temp -> flush/fsync -> backup rotate -> atomic replace 순서다.
- 손상 파일은 quarantine하고 backup을 읽는다. 자동 New Game/reset은 하지 않는다.
- 활성 dungeon 좌표는 저장하지 않지만 지형·fog·목표 상태와 최신 full-session checkpoint를 저장한다.
- checkpoint는 `lastSessionSeed`와 `sessionRngDrawCount`를 함께 저장하며 Continue가 실제 global stream을 복원한다.
- TPK는 정상 save를 삭제하지 않는다.
- TPK/Title 복구 실패는 메모리 상태를 보존하되 `recoveryPending`으로 전환해 일반 저장과 종료 자동 저장을 차단한다. 성공 load 또는 확인된 New Game만 이 차단을 해제한다.

## 4. 검증 표면

- Release-safe `CHECK` 기반 TestHarness
- `CrawlmasterRuntime` production library를 앱과 controller transcript test가 함께 링크
- Linux CTest 16개: Resource/HUD/Combat/Content/Agency/Localization/FontRaster/독립 process RNG·seedless migration/Controller/UI state raster
- 2026-09-04 로컬 GCC 15.2.0 검증: `build/debug` 16/16, `build/release` 16/16 통과
- 3층 생성/이관/round-trip, 세 목적형 퀘스트, 보고 rollback과 전투 저장 실패 후 월드 참조 수명을 production-linked 계약 테스트로 확인했다.
- 퀘스트 보드/일지와 발견 표식을 포함한 5 locale × 75/100/200% production raster를 생성하고 대표 100/200% 화면을 판독했다.
- Linux `Crawlmaster-0.10.0-Linux-x86_64.tar.gz` 패키지의 임의 CWD 리소스 탐색과 SHA-256 검증을 통과했다.
- Turn 3 FIN-F001~F035 remediation은 strict v4, in-memory Party/World/RNG transaction, character/combat/shutdown lifecycle과 production E2E로 검증했다. 세부 원 finding verdict는 `docs/audit/audit_report_14.md`에 기록한다.
- RA15 recovery remediation은 TPK/Title 실패의 write fence, 단독 손상 backup quarantine과 독립 process `IoError` 불변성을 검증했다. 후속 판정은 `docs/audit/audit_report_16.md`에 기록한다.
- Linux Release install/CPack/arbitrary-CWD Xvfb smoke
- 로컬 MinGW 전체 target compile과 Windows ZIP/checksum/PE import 정적 검사
- artifact SPDX generator, OSV/Grype known-vulnerability gate와 GitHub SBOM/provenance attestation workflow

## 5. 미검증/잔여 gate

- Windows Server 2022 hosted MSVC와 GitHub SLSA/SPDX run `33786241695`는 0.9.4 historical evidence다. current v0.10 tree는 `UNVERIFIED`다.
- Clean Windows 10/11 VC++ runtime 및 high-DPI/장시간 실기: `UNVERIFIED`
- 30~60분 완주 시간과 장기 밸런스: `UNVERIFIED`
- 5 locale × 3 scale production state/substate raster는 Linux에서 생성·판독. 실제 IME 조합 입력과 OS high-DPI는 `UNVERIFIED`
- 폰트 hash/upstream/license는 `FONT_PROVENANCE.md`로 검증. 제품 legal/support identity는 `Human Review Required`
