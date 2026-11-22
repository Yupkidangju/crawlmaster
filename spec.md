# spec.md (Crawlmaster v1.0.0 — AAA 마스터 스펙 & 설계 기준)

## 1. 문서 운영 규칙

* 본 문서는 **Crawlmaster v1.0.0**의 단일 진실원(Source of Truth)이다.
* 모든 소스 코드, 에셋 파이프라인, 테스트, `designs.md`, `DESIGN_DECISIONS.md`, `IMPLEMENTATION_SUMMARY.md`, `BUILD_GUIDE.md`는 본 스펙과 정합성을 유지한다.
* 설계·계약 변경이 발생하면 코드를 수정하기 전에 본 문서를 먼저 갱신한다.
* 내부 문서는 한국어로 작성하며, `README.md`는 5개국어(한국어·영어·일본어·중국어 번체·중국어 간체)를 유지한다.

## 2. 프로젝트 정체성

* **프로젝트명:** Crawlmaster (크롤마스터)
* **버전:** `1.0.0` (AAA 전면 리팩토링 릴리스)
* **장르:** 다크 판타지 회화풍 리얼리즘 1인칭 3D 그리드 던전 크롤러 전술 RPG
* **기술 기반:** C++20, SFML 2.6.1 (Window/Graphics/System), OpenGL 3.3+ / GLSL 3D 텍스처 던전 렌더링(+ 자동 GL 1.1 소프트웨어 텍스처 3D 폴백), `miniaudio` 0.11.26 헤더 온리 오디오 엔진, `nlohmann/json` 3.11.3
* **아트 디렉션:** 다크 판타지 회화풍 리얼리즘 (*Legend of Grimrock* / *Wizardry 리메이크* / *Darkest Dungeon* 계열 — 습기 찬 고대 석조·노멀맵·흔들리는 동적 횃불 광원·바이옴별 안개·유화/회화풍 몬스터 및 초상화·금속/석재 9-슬라이스 UI)
* **지원 플랫폼:** Ubuntu 24.04 x86_64 Linux 및 Windows x64 (MSVC 2022 + MinGW 정적 검사). GitHub Actions CI/CD로 양 플랫폼 빌드·테스트·패키지·SPDX SBOM·SLSA 증명·GitHub Release 자동 생성. macOS는 범위 밖.

## 3. 목표와 객관적 AAA 성공 기준 (Objective AAA Quality Gates)

| 게이트 영역 | 객관적 합격 기준 | 검증 수단 |
| --- | --- | --- |
| **G1. 3D 그래픽 & 아트** | 5개 고유 3D 바이옴(알베도+노멀맵+러프니스, 동적 횃불 광원, 안개, 블룸/비네트, 60fps 카메라 보간·헤드밥·파티클) + 전 몬스터/초상화/UI/마을 고해상도 회화풍 에셋. 와이어프레임 폴백 0건. | `UiStateRasterTests` 5개 바이옴/전투/마을 실제 PNG 캡처 시각 판독 + `ResourceVerification` |
| **G2. 인터페이스 & UX** | 16:9 반응형 와이드스크린(기본 1280×720, 리사이즈·1920×1080·전체화면 지원), 완전한 마우스+키보드 하이브리드(호버·클릭·툴팁·비교), Controller-View MVC 완전 분리, 5개국어×배율×고대비 겹침/잘림 0건. | `UiStateRasterTests` + `FontRasterTests` + `HudContractTests` + `ControllerContractTests` |
| **G3. 오디오** | BGM 5종 + 바이옴 환경음 5종 + 전투/탐험/UI SFX 22종 실음원(무음 아님, 클리핑 없음), 헤드리스 안전 `miniaudio` 믹서, Config v3 볼륨/음소거 영속화. | `AudioContractTests` + `ResourceVerification` |
| **G4. 게임 시스템** | 5층 영속 월드(5개 바이옴, 함정·상자·비밀벽·레버·잠긴 문·성소·로어), 전열/후열 진형·무기 사거리·적 행동 예고·속성 약점·스태거/브레이크·8종 전직/특성 트리·장비 희귀도/접사/대장간 강화·8종 퀘스트 체인·2페이즈 최종 보스·몬스터 도감. | `CombatContractTests` + `ContentContractTests` + `AgencyContractTests` + `ControllerContractTests` E2E |
| **G5. 저장·안정성·릴리스** | Save Schema v5 & Config Schema v3, 원자적 저장/백업/격리, 구 v1~v4 세이브 안전 격리, 결정론적 `SessionRng` 프로세스 리플레이, Debug/Release `-Wall -Wextra -Werror -pedantic` & MSVC `/W4 /WX` 무경고, CI Linux/Windows 릴리스 그린. | `TestHarness` + `RngProcessReplayTests` + CPack + CI/CD Matrix |

## 4. 비목표 (Out of Scope)

* 외부 게임 엔진(Godot, Unreal, Unity) 전환 및 3D 스켈레탈 메시 몬스터 모델 파이프라인
* 게임패드(컨트롤러) 3중 입력 UI 및 동적 버튼 글리프 전환
* 구버전 Save Schema v1~v4 자동 데이터 마이그레이션 (안전 격리 후 새 게임 안내만 수행)
* 네트워크 멀티플레이어, macOS 타겟, 유료 외부 상용 에셋 구매

## 5. 동결된 핵심 결정

1. **화면:** 기본 `1280x720` (16:9), 리사이즈 가능(`sf::Style::Default`), `F11` 및 설정 화면에서 전체화면 전환 및 해상도 프리셋(`1280x720`, `1600x900`, `1920x1080`) 지원. 가상 1280×720 논리 좌표계와 고해상도 3D 백버퍼 매핑으로 모든 비율에서 왜곡 없는 반응형 렌더링 제공.
2. **렌더링 이중 경로:**
   * **Primary (Hardware OpenGL / GLSL):** SFML OpenGL 컨텍스트 위에서 GLSL 120/130/330 셰이더로 노멀맵 조명, 다중 흔들리는 동적 횃불 점광원, 바이옴 지수 안개, 수면/용암 발광, 포스트프로세싱 블룸/비네트/색보정, 파티클을 렌더링한다.
   * **Fallback (Software-Textured 3D for GL 1.1 / Headless CI):** `sf::Shader::isAvailable() == false` 환경(예: Windows CI GDI Generic OpenGL 1.1)에서도 동일한 텍스처 3D 원근 폴리곤·거리 안개·동적 횃불 정점 조명·몬스터/UI 스프라이트로 렌더링하며 크래시나 와이어프레임 퇴행이 절대 발생하지 않는다.
3. **오디오:** `miniaudio` 0.11.26 (`877ea4a7c2601c7b0f2a44e83dfac42e6470fea6`)을 단일 헤더 CMake FetchContent로 고정. 오디오 장치 부재 또는 `CRAWLMASTER_HEADLESS=1` 시 자동으로 Null 백엔드로 폴백한다.
4. **세이브/설정 정책:** **Save Schema v5** & **Config Schema v3** 클린 브레이크. 구버전 세이브(`schemaVersion < 5`) 감지 시 원본을 보존한 채 `quarantine` 이동 후 `PersistenceStatus::UnsupportedVersion` 반환. v5 자체의 same-dir temp -> fsync -> backup rotate -> atomic replace, `recoveryPending` write fence, TPK non-destructive checkpoint 복원, `SessionRng` seed + raw draw count 리플레이 계약은 100% 유지.
5. **파티:** 최대 4인, 최대 레벨 **10**, 레벨 3에서 직업당 2종 전직(총 8종) 해금, 레벨업마다 특성 포인트(Talent Point) 1 획득.
6. **입력:** 모든 화면에서 마우스(호버·좌클릭·우클릭 취소·스크롤·툴팁)와 키보드(WASD/QE/방향키·Enter/Space·Esc·단축키) 100% 하이브리드 동작.

## 6. 기술 스택과 아키텍처 (MVC 완전 분리)

* **Core (`include/core/`, `src/core/`):**
  * `Game`: SFML 윈도우, 16:9 뷰포트 매핑, 60fps 델타타임, 화면 페이드 전환, 토스트 알림, 상태 루프
  * `GameStateManager`: 스택 기반 상태 전이(`push/pop/change/replaceAll`)
  * `AssetManager`: 텍스처·노멀맵·초상화·몬스터 스프라이트·아이콘·셰이더·폰트 캐시 및 검증
  * `AudioManager`: `miniaudio` 래퍼(BGM 크로스페이드, 바이옴 환경음, 폴리포닉 SFX, 헤드리스 Null 안전)
  * `LocalizationManager`: 5개국어 카탈로그, 플레이스홀더 포맷터, Config v3(언어·배율·고대비·해상도·전체화면·볼륨·음소거·그래픽 품질)
  * `Persistence`, `ResourceLocator`, `SessionRng`
* **Model (`include/model/`, `src/model/`):**
  * `Character`: 신원, 6대 능력치, 레벨 1~10, `CombatRow`, `Specialization`, 특성 트리(`TalentId`), 4슬롯 장비(Weapon/Armor/Shield/Accessory), 상태이상 9종, 버프/가드/도발
  * `Equipment` / `Item` / `ConcreteItems` / `ItemFactory`: 36+ 기본 아이템, `ItemRarity`(5등급), `upgradeLevel`(+0~+5), `ItemAffix`, `WeaponReach`(Melee/Reach/Ranged), `DamageType`(9속성)
  * `Skill` / `ConcreteSkills` / `SkillFactory`: 4직업 × 6스킬(기본 4 + 전직 특화 2 = 24+ 스킬/주문)
  * `Monster` / `MonsterFactory`: 20종 몬스터(5층 일반/정예 14종 + 층별 중보스 5종 + 2페이즈 최종 보스 1종), `stagger/poise`, 약점/저항, `EnemyIntent` 텔레그래프 AI
  * `CombatRules` / `CombatActionRules`: 진형 보정, 무기 사거리, 속성 배율(약점 1.5x / 저항 0.5x), 스태거/브레이크(+50% 피해·행동 취소), d20 명중·자연 1/20 치명타, 대장간 강화/재연마 공식
  * `DungeonMap` / `DungeonWorld`: `FLOOR_COUNT = 5`, 5개 바이옴, 20×20 방+복도+루프 시드 생성, 함정·상자·비밀벽·레버/잠긴 문·성소·로어·보스 게이트·도달성 검증
  * `Quest`: 8종 정식 퀘스트 체인(회수·NPC 탐색·층별 보스 토벌·성소 유물·최종 용 토벌)
  * `Bestiary`: 몬스터 조우·처치 횟수 및 약점/드롭 해금 기록
  * `Party`: 4인 파티, 인벤토리, 중요품, 골드, 퀘스트, 도감, 5층 영속 월드, Save v5 직렬화
* **View (`include/view/`, `src/view/`):**
  * `UiTheme`: 다크 판타지 컬러/타이포/희귀도/속성 토큰
  * `UiSystem`: 9-슬라이스 패널, 인터랙티브 버튼(마우스 히트박스+호버+SFX), 애니메이션 게이지 바, 자동 경계 클램핑 툴팁, 플로팅 텍스트, 파티클 VFX 시스템, 모달 다이얼로그
  * `DungeonRenderer`: 1인칭 3D 텍스처/셰이더 던전 렌더러(GLSL 3D + 자동 GL 1.1 텍스처 3D 폴백, 카메라 보간/헤드밥/흔들림, 3D 바이옴 파티클, 미니맵/전체지도, 하단 파티 초상화 바, 로그)
  * `GameViews`: `TitleView`, `TownView`(길드/상점/대장간/교회/게시판/도감), `CharacterCreationView`, `CombatView`(3D 배경+몬스터 스프라이트 애니메이션+인텐트+스태거+턴 타임라인+VFX), `CharacterInfoView`(장비 비교+특성 트리+진형 전환), `QuestJournalView`, `BestiaryView`, `SettingsView`, `EndgameView`
  * `PartyHudSnapshot`: 불변 HUD 스냅샷 계약
* **Controller (`include/controller/`, `src/controller/`):**
  * `TitleState`, `TownState`, `CharacterCreationState`, `DungeonState`, `CombatState`(+`CombatStateActions`), `CharacterInfoState`, `QuestJournalState`, `BestiaryState`, `SettingsState`, `GameOverState`, `VictoryState`, `ShutdownState`
  * 컨트롤러는 상태/입력(마우스+키보드)/규칙 오케스트레이션만 담당하고, 모든 렌더링은 `View` 계층에 위임한다.

## 7. 시스템 상세 명세 및 실데이터 기준표

### 7.1 5개 층 바이옴 & 캠페인 구조 (`DungeonWorld::FLOOR_COUNT = 5`)

| 층 | Biome ID | 테마 | 조명/안개 톤 | 일반/정예 몬스터 풀 | 층 보스 / 핵심 목표 |
| --- | --- | --- | --- | --- | --- |
| 1F | `CATACOMBS` | 수몰된 납골당 | 따뜻한 호박 횃불 + 짙은 이끼 녹회색 안개 | `mon_kobold`, `mon_goblin`, `mon_skeleton`, `mon_crypt_rat` | `mon_crypt_warden` (묘지 감시자) + `qst_01_relic`, `qst_02_warden` |
| 2F | `CISTERN` | 익사자의 수로 | 청록 포자광 + 축축한 감청색 안개 | `mon_giant_spider`, `mon_goblin_shaman`, `mon_drowned_knight`, `mon_mire_slime` | `mon_bog_hydra` (수렁 히드라) + `qst_03_scout`, `qst_04_hydra` |
| 3F | `OSSUARY` | 저주받은 납골회랑 | 자줏빛 영혼 화로 + 창백한 골회색 안개 | `mon_ghoul`, `mon_orc`, `mon_bone_archer`, `mon_wraith` | `mon_ossuary_lich` (납골 리치) + `qst_05_phylactery`, `qst_06_lich` |
| 4F | `FOUNDRY` | 흑요석 대장간 | 주황 용암광 + 그을음 암적색 안개 | `mon_orc_berserker`, `mon_ember_golem`, `mon_ash_cultist`, `mon_hellhound` | `mon_iron_colossus` (강철 거신) + `qst_07_forge_core` |
| 5F | `SANCTUM` | 용의 성소 | 심홍 용염 + 황금빛 흑요석 안개 | `mon_drake_guard`, `mon_dragon_whelp`, `mon_sanctum_wyrm` | **2페이즈 최종 보스** `mon_elder_wyrm` (태고의 비룡) + `qst_08_elder_wyrm` |

### 7.2 던전 탐험 기믹 (`DungeonMap` / `WorldObject`)

* **TileType:** `WALL`, `EMPTY`, `DOOR`, `LOCKED_GATE`, `SECRET_WALL`, `UPSTAIRS`, `DOWNSTAIRS`, `BOSS_GATE`
* **WorldObjectKind:** `QUEST_ITEM`, `NPC`, `QUEST_BOSS`, `CHEST`, `TRAP`, `LEVER`, `SANCTUARY`, `LORE_PLAQUE`
* **규칙:**
  * `CHEST`: 층별 스케일링된 골드 + Uncommon/Rare/Epic 접사 장비 또는 소모품 획득 (1회성).
  * `TRAP`: 파티에 도적(`ROGUE`)이 있으면 인접 시 자동 탐지(노란 경고 아이콘 + 3D 바닥 룬 표시). `E`로 해제 시도(`d20 + DEX mod + (Rogue ? +4 : 0) >= 12 + floor`). 실패 또는 밟을 시 함정 피해/독 발동 후 해제.
  * `LEVER`: `E` 작동 시 같은 층의 `LOCKED_GATE` 숏컷을 영구 개방(계단과 보스실 사이 귀환 동선 단축).
  * `SECRET_WALL`: 인접 시 미세한 금/광채 표시, `E` 조사 시 통로로 개방되어 비밀 보물방 연결.
  * `SANCTUARY`: 층당 1회 화톳불 성소 휴식 — 생존 파티원 HP 60% 회복 + 주문 슬롯 50%(최소 1) 회복 + 독/화상 해제.
  * `LORE_PLAQUE`: 층별 다크 판타지 세계관 비문 열람 + 소량 경험치(25 × floor) 최초 1회 획득.

### 7.3 전술 전투 시스템 (`CombatRules`)

* **전열/후열 진형 (`CombatRow::FRONT`, `CombatRow::BACK`):**
  * 전열: 근접 무기(`MELEE`) 100% 피해, 적 근접 공격 우선 타겟(가중치 75%).
  * 후열: 근접 무기 공격 시 피해 -35%(최소 1), 원거리/장병기(`REACH`/`RANGED`) 및 마법 100% 피해, 근접 피격 시 AC +2 보너스.
  * 전투 중 행동으로 `Guard`(방어 태세: 다음 턴까지 받는 피해 50% 감소 + AC +3 + 스태거 반격) 및 `Swap Row`(진형 교대, 턴 소비 없이 턴당 1회 가능 또는 전술 버튼) 제공.
* **9대 속성 (`DamageType`):** `SLASHING`, `PIERCING`, `BLUDGEONING`, `FIRE`, `ICE`, `LIGHTNING`, `HOLY`, `POISON`, `ARCANE`.
  * 약점 적중: 피해 **1.5배** + 스태거 게이지 추가 -2 (기본 -1).
  * 저항 적중: 피해 **0.5배**(최소 1), 스태거 감소 0.
* **스태거/브레이크 (Stagger & Break):**
  * 모든 몬스터는 `maxStagger` (일반 2~3, 정예 4, 중보스 5, 최종 보스 6) 보유.
  * `currentStagger == 0` 도달 시 **BROKEN** 상태(1턴 행동 취소 + 받는 피해 **1.5배** + 예약된 차징/광역기 강제 중단). 브레이크 종료 후 `currentStagger = maxStagger` 회복.
* **적 행동 예고 (`EnemyIntent`):**
  * 매 라운드/턴 시작 시 모든 생존 몬스터가 다음 행동(`ATTACK`, `HEAVY_STRIKE`, `AOE_BREATH`, `SPELL`, `DEBUFF`, `BUFF`, `DEFEND`, `CHARGING`)을 확정하고 UI 아이콘·툴팁으로 공개.
  * 최종 보스 `mon_elder_wyrm`은 HP ≤ 50% 도달 시 **Phase 2: 태고의 각성** 돌입(스태거 초기화, 화염 보호막, 격노 광역 브레스 패턴 해금).

### 7.4 캐릭터 성장·전직·특성 트리 (레벨 1~10)

* **레벨업 공식:** `nextLevelXp(L) = 100 * L * L` (Lv 1→2: 100, Lv 2→3: 400, ..., Lv 9→10: 8100). 최대 레벨 10.
* **전직 (레벨 3 이상, 마을 길드 또는 캐릭터 정보창에서 1회 선택):**
  * `WARRIOR` → `VANGUARD` (방벽의 기사: AC+2, 도발/아군 엄호) | `BERSERKER` (광전사: 공격력+2, 피의 격노/출혈)
  * `MAGE` → `PYROMANCER` (화염술사: 화염 피해+25%, 메테오 스파크) | `CHRONOMANCER` (시공간술사: 빙결/전격·가속·적 지연)
  * `ROGUE` → `ASSASSIN` (암살자: 치명타 범위 19~20, 급소 처형) | `TRAPSMITH` (기공사: 함정 해제+6, 연막탄·산성폭탄)
  * `CLERIC` → `PALADIN` (성기사: 전열 신성 강타·오라) | `ORACLE` (신탁사: 주문 슬롯+2, 광역 정화·부활의 기도)
* **특성 포인트 (Talent Tree):** 레벨업마다 1포인트 획득, 직업별 4개 패시브 노드(각 최대 3랭크: 체력/공격/전술/속성 특화)에 투자.

### 7.5 장비 희귀도, 접사, 대장간 강화 & 36+ 아이템

* **희귀도 (`ItemRarity`):** `COMMON`(#d6cfc2), `UNCOMMON`(#52c46b), `RARE`(#4a9eff), `EPIC`(#b86bff), `LEGENDARY`(#ffb833).
* **장비 슬롯 (4종):** `WEAPON`, `ARMOR`, `SHIELD`, `ACCESSORY` (반지·부적).
* **대장간 (`TownLocation::BLACKSMITH`):**
  * **강화 (Enhance +0 → +5):** 비용 `40 * (upgradeLevel + 1) * rarityMultiplier` 골드. 무기 피해 보너스 +1/레벨, 방어구/방패 AC +1/2레벨(홀수 레벨 HP+4), 장신구 주능력치 +1/2레벨.
  * **재연마 (Reforge Affix):** 비용 `60 * rarityMultiplier` 골드. `SessionRng`로 장비 접사(STR/DEX/CON/INT/WIS/AC/CRIT/LIFESTEAL/ELEMENTAL) 재부여.

### 7.6 8종 정식 퀘스트 체인

1. `qst_01_relic` (1F 회수): 납골당의 잃어버린 성배 회수 (`key_sun_chalice`)
2. `qst_02_warden` (1F 보스): 묘지 감시자 토벌 (`mon_crypt_warden`)
3. `qst_03_scout` (2F NPC): 실종된 길드 정찰병 엘라 구조 (`npc_missing_scout`)
4. `qst_04_hydra` (2F 보스): 익사자의 수로 지배자 수렁 히드라 토벌 (`mon_bog_hydra`)
5. `qst_05_phylactery` (3F 회수): 저주받은 성물함 회수 (`key_bone_phylactery`)
6. `qst_06_lich` (3F 보스): 납골 리치 토벌 (`mon_ossuary_lich`)
7. `qst_07_forge_core` (4F 보스/회수): 강철 거신 토벌 및 흑요석 핵 확보 (`mon_iron_colossus`)
8. `qst_08_elder_wyrm` (5F 최종 보스): 태고의 비룡 토벌 (`mon_elder_wyrm`) → 캠페인 승리(`VictoryState`)

## 8. 저장 및 설정 스키마 (Save v5 / Config v3)

* **Save Schema v5 (`save.json`):**
  * `schemaVersion: 5` (v1~v4 파일 발견 시 자동 `quarantine` 후 `PersistenceStatus::UnsupportedVersion`)
  * `lastSessionSeed`, `sessionRngDrawCount`, `gold`, `campaignCompleted`, `totalTurns`, `deepestFloorReached`
  * `members[]`: 신원, 클래스, 전직, 레벨, XP, HP/MaxHP, SpellSlots, `row`, 6대 능력치, 특성 랭크, 장착 장비(ID + rarity + upgradeLevel + affix), 상태이상
  * `inventory[]`: 일반 아이템/접사·강화 장비
  * `keyItems[]`, `quests[]`, `bestiary[]`
  * `dungeonWorld`: `seed`, `floors[5]`(20×20 타일·방문·밟음·탐지 함정), `objects[]`(상태 포함)
* **Config Schema v3 (`config.json`):**
  * `schemaVersion: 3`
  * `language`, `textScale`, `highContrast`, `windowWidth`, `windowHeight`, `fullscreen`, `masterVolume`, `bgmVolume`, `sfxVolume`, `muted`, `graphicsQuality`

## 9. 빌드, 검증 및 CI/CD 릴리스 파이프라인

* **로컬 빌드/검증:**
  * `cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug && cmake --build build/debug --parallel`
  * `ctest --test-dir build/debug --output-on-failure --no-tests=error`
  * `cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release && cmake --build build/release --parallel`
  * `ctest --test-dir build/release --output-on-failure --no-tests=error`
  * `cpack --config build/release/CPackConfig.cmake`
* **원격 릴리스 (GitHub Actions `.github/workflows/ci.yml`):**
  * `v*` 태그 푸시 시 `ubuntu-24.04` 및 `windows-2022` 매트릭스 빌드 → CTest → CPack(`.tar.gz` / `.zip` + `.sha256`) → 임의 CWD 리소스/기동 스모크 → SPDX SBOM & Grype 스캔 → SLSA/SBOM 증명 → GitHub Release 자동 생성 및 양 플랫폼 릴리스 아티팩트 첨부.
