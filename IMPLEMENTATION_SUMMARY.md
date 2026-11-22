# IMPLEMENTATION_SUMMARY.md (v1.0.0 AAA 구현 및 파일 책임)

작성 기준: 2026-11-22  
제품 버전: **Crawlmaster v1.0.0** (AAA Dark Fantasy 5-Floor 3D Tactical Dungeon Crawler RPG)

## 1. 런타임 흐름

```text
main (--verify-resources | GUI loop)
  -> Game (16:9 1280×720 virtual letterbox canvas, F11 fullscreen, mouse mapping, AssetManager, AudioManager, fade/toast)
  -> GameStateManager
       TitleState
         -> TownState (Hub / Dungeon Gate / Guild / Shop Buy & Sell / Blacksmith Forge & Reforge / Church / Quest Board)
              -> CharacterCreationState (4d6 reroll + 10pt weighted allocation)
              -> CharacterInfoState (Inventory / Lv3+ Specializations & 4-Node Talents / Skills / Row toggle)
              -> BestiaryState (24-monster Codex, weakness/drop unlock)
              -> QuestJournalState (8 canonical quests)
              -> SettingsState (Config v3: Language / Text Scale / High Contrast / Resolution / Fullscreen / 3D Quality / Audio Vol & Mute)
              -> DungeonState (5 persistent biomes, 3D GLSL + GL 1.1 fallback, chests/traps/levers/secret walls/sanctuaries/lore)
                   -> CombatState (Front/Back rows, 9 elements, Stagger/Break, Enemy Intent, Guard, VFX, 2-phase Elder Wyrm)
                        -> VictoryState (final boss defeat)
                   -> GameOverState (non-destructive TPK checkpoint restore)
       ShutdownState (save failure retry / recoveryPending write-fence recovery / explicit exit)
```

`replaceAll()`은 TPK·승리·새 게임 등 세션 루트가 바뀌는 전이에서 잔존 상태를 완전히 제거한다.

## 2. 모듈별 파일 책임

### Core (`include/core/`, `src/core/`)
- `Game`: 16:9 가상 1280×720 캔버스, 창 리사이즈 레터박스 뷰포트, `F11` 전체화면, 마우스 논리 좌표 변환, 화면 페이드·토스트, 상태 루프
- `GameStateManager`: 상태 스택(`push`/`pop`/`change`/`replaceAll`)
- `AssetManager`: 텍스처(5바이옴+노멀맵·배경·초상화·24몬스터·9프롭·UI 9-슬라이스·파티클·비네트) 및 GLSL 셰이더 캐싱, `shadersAvailable()` 감지
- `AudioManager` + `MiniAudioImpl`: `miniaudio` 0.11.26 래퍼, 5 BGM·5 바이옴 환경음 크로스페이드·24 SFX, 볼륨/음소거, CI/Xvfb 헤드리스 `ma_backend_null` 자동 폴백
- `LocalizationManager`: 464키 × 5개국어 카탈로그, 플레이스홀더 포맷, Config Schema v3 영속화
- `Persistence`: OS별 사용자 데이터 경로, `fsync`/원자 교체, 백업 회전, 심볼릭 링크 거부, 구버전 `v1~v4` 및 손상 파일 안전 격리(`quarantine`)
- `ResourceLocator`: 실행 파일 기준 `assets`, `../assets`, `../share/crawlmaster/assets` 탐색
- `SessionRng`: 세션 시드 + 원시 드로우 카운트 기반 단일 `std::mt19937` 결정론 스트림

### Model (`include/model/`, `src/model/`)
- `Character` + `CharacterIdentityRules`: Lv 1~10 성장, 8종 Lv3+ 전직(`Specialization`), 4노드 특성 트리(최대 랭크 3), 전열/후열 진형, 상태이상(독/화상/출혈/마비/빙결/축복/보호막/도발/방어)
- `Item` / `Equipment` / `ConcreteItems` / `ItemFactory`: 43종 아이템, 5등급 희귀도, 9종 속성, 무기 사거리, 접사(Affix), `+0..+5` 대장간 강화 및 접사 재연마
- `Skill` / `ConcreteSkills` / `SkillFactory`: 24종 직업/전직 스킬, 스태거/약점/상태이상/아군 치유·부활·버프
- `Monster` / `MonsterFactory` / `Bestiary`: 24종 몬스터(5층 일반+중간/최종 보스), 2페이즈 최종 보스 `mon_elder_wyrm`, 인텐트 예고, 스태거/브레이크, 도감 약점/드롭 해금
- `CombatRules` / `CombatActionRules`: 전열/후열 보정, 9속성 상성, 스태거/브레이크 배율, 대장간 비용·재연마 규칙, 무효 소모품 비소비
- `DungeonMap` / `DungeonWorld`: 5층 × 20×20 5바이옴 영속 던전, 상자·함정·레버·잠긴 문·비밀 벽·성소·로어·보스 게이트·계단 생성 및 직렬화
- `Quest`: 8종 정식 퀘스트(수집·처치·NPC 구출·보스 토벌)
- `Party`: 최대 4인 파티, 공유 인벤토리, 도감, 8종 퀘스트, 5층 영속 월드, Save Schema v5 직렬화 및 `v1~v4` 안전 격리
- `RecruitmentDraft`: 신원 검증, 4d6 리롤, 가중 10포인트 배분, 일회성 확정

### View (`include/view/`, `src/view/`)
- `DungeonRenderer`: 1인칭 텍스처 3D 메시 투영(근접 셀 8×8 세분화, 근평면 클리핑, 엄격한 약순서 페인터 정렬), GLSL 노멀맵/동적 횃불/안개 + 블룸/비네트 포스트프로세싱, GL 1.1 CPU-lit 텍스처 3D 폴백, 미니맵/전체지도, 3D 월드 프롭 빌보드, 부유 입자
- `UiTheme`: 다크 판타지 옵시디언/골드 팔레트, 고대비 모드 분기, 5바이옴 조명/안개 프로필, 9속성·5희귀도·인텐트 색상
- `UiSystem`: 9-슬라이스 패널, 자동 크기 단축키 배지 버튼, 게이지 바(HP 트레일), 배지, 4인 유화 초상화 카드, 툴팁, 전투 파티클 VFX·플로팅 데미지
- `PartyHudSnapshot`: 컨트롤러/뷰 분리용 불변 파티 HUD 스냅샷

### Controller (`include/controller/`, `src/controller/`)
- `TitleState`, `TownState`, `CharacterCreationState`, `DungeonState`, `CombatState`(+`CombatStateActions`), `CharacterInfoState`, `BestiaryState`, `QuestJournalState`, `SettingsState`, `GameOverState`, `VictoryState`, `ShutdownState` — 전 상태 마우스 + 키보드 하이브리드 조작 통합

## 3. 저장·복구 및 검증 경계

- Save Schema v5는 파티·전직/특성·강화/접사 장비·도감·8종 퀘스트·5층 영속 월드·RNG 시드/드로우 카운트를 원자적으로 저장한다.
- 구버전 `v1~v4` 세이브는 `Persistence::quarantine`으로 원본을 보존 격리하고 `UnsupportedVersion` 상태를 반환한다.
- 15개 CTest 타겟(`ResourceVerification`, `TestHarness`, `Hud/Combat/Content/Agency/Localization/Audio/Controller ContractTests`, `FontRasterTests`, `UiStateRasterTests`, `RngReplay*`, `LegacySaveQuarantineTest`)과 1280×720 시각 검수, CPack, SPDX 2.3 SBOM, MinGW 교차 컴파일 및 GitHub Actions Linux+Windows 릴리스 파이프라인으로 검증한다.
