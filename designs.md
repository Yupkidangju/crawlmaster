# designs.md (UI/UX 및 화면 디자인 명세)

## 1. 개요 및 핵심 경험
* 본 문서는 **Crawlmaster**의 비주얼 토큰, 화면 흐름, 레이아웃 및 1인칭 와이어프레임 UI 레이아웃의 규칙을 동결한다.
* 복고풍 벡터 그래픽스 감성을 극대화하기 위해, 오직 선(Line)과 단순 텍스트, 네온 스타일의 단색 단색 팔레트로 UI를 구성한다.

## 2. 컬러 및 타이포 토큰

### 2.1 컬러 팔레트 (Color Palette)
전체 UI는 모노크롬 네온 그린 테마를 기본으로 하며, 특정 상호작용 및 경고 시에 제한적인 레트로 단색 계열을 채용한다.
* `COLOR_BG`: `#050B05` (거의 검은색에 가까운 어두운 녹색 배경)
* `COLOR_NEON_GREEN`: `#33FF33` (주요 벽면 선, HUD 라인, 일반 텍스트)
* `COLOR_BRIGHT_GREEN`: `#66FF66` (포커스된 메뉴 항목, 직접 밟은 미니맵 바닥)
* `COLOR_PLAYER_CYAN`: `#66FFFF` (미니맵 현재 파티 위치 방향 삼각형)
* `COLOR_AMBER`: `#FFB000` (상점 골드 정보, 경고, 퀘스트 완료 표시, 버프 상태)
* `COLOR_RED`: `#FF3333` (대미지 수치, 심각한 상태이상(독, 마비), 사망 캐릭터 표시)
* `COLOR_MUTED`: `#114411` (비활성 UI 테두리, 보이지 않는 미니맵 탐색 경계)

### 2.2 타이포그래피 (Typography)
* **기본 폰트:**
  * 한글/영어 기본: `neodgm.ttf` (한글/영문 레트로 감성 폰트)
  * 일어/중국어 다국어: `NotoSansCJK-Regular.ttc` (혼합 CJK/ASCII raster 검증 및 OFL-1.1 provenance 대상)
  * 영문 폴백: `UbuntuMono[wght].ttf` (내부 family와 일치하는 이름, UFL-1.0)
* **폰트 크기 정책:**
  * `FONT_SIZE_TITLE`: 32pt (메인 타이틀)
  * `FONT_SIZE_HEADER`: 20pt (마을 건물 이름, 전투 상태창 헤더)
  * `FONT_SIZE_BODY`: 14pt (일반 대화 로그, 스탯 수치, 메뉴 텍스트)
  * `FONT_SIZE_SMALL`: 11pt (상세 수치 묘사, 미니맵 인덱스)

## 3. ASCII UI 레이아웃 구조도

### 3.0 캐릭터 생성 화면 (CharacterCreationState)

길드의 생성 메뉴는 독립 상태 화면을 열며 `신원 입력 -> 능력치 배분 -> 최종 확인`의 세 단계를 사용한다.

```text
+-----------------------------------------------------------------------------+
|                         === CHARACTER CREATION ===                          |
|                                                                             |
|  Name   : > Aria_          Age: 20    Gender: Female    Class: Rogue        |
|  Trait  : Quick Reflexes (+2 Initiative)                                    |
|                                                                             |
|  STR 11 -> 11   DEX 14 -> 16   CON 12 -> 13   Remaining Points: 4          |
|  INT 10 -> 10   WIS  9 ->  9   CHA 13 -> 13   Next Cost: 3                 |
|                                                                             |
|  [Up/Down] Select | [Left/Right] Change | [R] Reroll | [Enter] Continue     |
|  [Esc] Back/Cancel                                                            |
+-----------------------------------------------------------------------------+
```

* 이름은 `TextEntered`와 Backspace로 편집하고 선택 행과 오류 이유를 텍스트로 표시한다.
* 200% 텍스트에서는 단계별 핵심 정보만 표시하여 겹침을 피한다.
* 최종 확인 전에는 파티와 저장 파일을 변경하지 않으며 저장 실패 시 입력 내용을 유지한다.

### 3.1 3D 던전 탐험 화면 (DungeonState)
화면 크기: 1024x768 고정

* HUD는 현재 층 `B1/B2/B3`과 현재 칸에서 가능한 `E` 상호작용을 텍스트로 표시한다.
* 수락한 목표는 시야로 발견하기 전에는 미니맵에 표시하지 않는다. 발견 후 중요품은 마름모, NPC는 사각 윤곽, 퀘스트 보스는 적색 십자 표식으로 구분하여 색상만으로 의미를 전달하지 않는다.
* `E`는 계단 이동·중요품 회수·NPC 대화에 사용하고 `Q`는 퀘스트 일지를 push한다. 자동 이동은 아이템·NPC·계단을 자동 활성화하지 않지만 quest boss/BossGate 칸에 진입하면 수동 이동과 동일하게 전투를 시작한다.

### 3.1.1 퀘스트 일지 (QuestJournalState)

```text
+-----------------------------------------------------------------------------+
| === QUEST JOURNAL ===                                      Key Items: 1     |
| > Moon Seal Recovery       B1  READY TO REPORT                               |
|   Clue: beyond the old door                                                 |
|   Crypt Warden             B2  ACTIVE                                        |
|   Missing Scout            B3  ACTIVE                                        |
|                                                                             |
| Up/Down: Scroll                                              Esc: Back       |
+-----------------------------------------------------------------------------+
```

* 일지는 읽기 전용이며 수주·보상 지급은 성에서만 수행한다.
* 200% 배율에서는 선택된 퀘스트 한 건의 이름·상태·층·단서와 스크롤 위치만 표시한다.

```text
+-----------------------------------------------------+-----------------------+
|                                                     |  [MINI MAP]           |
|                                                     |  Level 1  (X:12 Y:08) |
|                                                     |                       |
|                                                     |  . . . # # . . . . .  |
|                                                     |  . . . # P # . . . .  |
|              [ 1인칭 3D 와이어프레임 뷰 ]           |  . . . # . # . . . .  |
|                                                     |  . . . # # # . . . .  |
|              (700 x 500)                            |                       |
|                                                     |  P: Player            |
|                                                     |  #: Wall  .: Floor    |
|                                                     |                       |
|                                                     +-----------------------+
|                                                     |  [PARTY STATUS]       |
|                                                     |  1. WARRIOR  HP:18/18 |
|                                                     |  2. CLERIC   HP:14/14 |
|                                                     |  3. ROGUE    HP:12/12 |
|                                                     |  4. MAGE     HP:08/08 |
+-----------------------------------------------------+-----------------------+
| [LOG WINDOW]                                        | [COMMANDS]            |
| > Moved North.                                      | W - Forward           |
| > You hear bones rattling nearby...                 | A/D - Turn Left/Right |
| > Active Quest: Kobold Hunt (3/5 Killed)            | S - Move Backward     |
| >                                                   | I - Inventory         |
+-----------------------------------------------------+-----------------------+
```

### 3.2 턴제 전투 화면 (CombatState)

```text
+-----------------------------------------------------+-----------------------+
|                                                     |  [FOES]               |
|                                                     |  A: mon_skeleton (9HP)|
|                                                     |  B: mon_goblin   (7HP)|
|                                                     |  C: mon_goblin   (7HP)|
|                 [ MONSTER SPRITES ]                 |                       |
|                                                     |                       |
|              (와이어프레임 아웃라인 몬스터)         |                       |
|                                                     |                       |
|                                                     |                       |
|                                                     +-----------------------+
|                                                     |  [PARTY ACTION]       |
|                                                     |  * Turn: WARRIOR      |
|                                                     |  [1] Attack           |
|                                                     |  [2] Skill / Spell    |
|                                                     |  [3] Item             |
|                                                     |  [4] Run              |
+-----------------------------------------------------+-----------------------+
+-----------------------------------------------------+-----------------------+
| [BATTLE LOG]                                       | [SELECTED TARGET]     |
| > WARRIOR rolled 15 (Hit!).                         | > Target: A (Skeleton)|
| > SKELETON took 7 Slash damage.                     |                       |
| > SKELETON is still standing.                       |                       |
+-----------------------------------------------------+-----------------------+
```

### 3.3 전투 중 스킬/주문 선택 팝업창 (Skill/Spell Selection Menu Overlay)

전투 화면에서 2번 `Skill / Spell` 입력 시, 좌측 몬스터 그래픽 하단 또는 로그 윈도우 위쪽에 팝업 TUI 오버레이가 드로잉된다.

```text
+-----------------------------------------------------+-----------------------+
|                                                     |  [FOES]               |
|                                                     |  A: mon_skeleton (9HP)|
|                                                     |                       |
|                 [ MONSTER SPRITES ]                 |                       |
|                                                     |                       |
|  +-----------------------------------------------+  |                       |
|  | [SELECT SKILL/SPELL]                          |  +-----------------------+
|  | > 1. Slash        (Cost: None)                |  |  [PARTY ACTION]       |
|  |   2. Shield Bash  (Cost: None)                |  |  * Turn: WARRIOR      |
|  |   3. Cleave       (Cost: None)                |  |  [ESC] Cancel         |
|  |                                               |  |                       |
|  +-----------------------------------------------+  |                       |
+-----------------------------------------------------+-----------------------+
| [BATTLE LOG]                                       | [SELECTED TARGET]     |
| > Select a skill or spell to perform.               | > Target: A (Skeleton)|
+-----------------------------------------------------+-----------------------+
```

### 3.4 캐릭터 정보 및 인벤토리 화면 (CharacterInfoState)

```text
+-----------------------------------------------------------------------------+
|                     === CHARACTER SHEET & INVENTORY ===                     |
|            1.HeroA [*]   2.HeroB       3.HeroC       4.HeroD                |
|                                                                             |
|  +-----------------------------------+   +-------------------------------+  |
|  | [ MEMBER DETAILS ]                |   | [ PARTY INVENTORY ]           |  |
|  | Name:  HeroA                      |   | Gold: 100 G                   |  |
|  | Class: Warrior (Lv.1)             |   |                               |  |
|  | HP:    18 / 18                    |   |   pot_heal                    |  |
|  | XP:    0 / 300   AC: 14           |   |   pot_greater_heal            |  |
|  | Status: [OK]                      |   | > pot_strength  (물약)        |  |
|  | - Abilities -                     |   |   scr_cure                    |  |
|  | STR: 15 (+2)      INT: 10 (0)     |   |                               |  |
|  | DEX: 12 (+1)      WIS: 9  (-1)    |   |                               |  |
|  | CON: 14 (+2)      CHA: 11 (0)     |   | +---------------------------+ |  |
|  |                                   |   | | [ pot_strength ]          | |  |
|  | - Equipment Slots -               |   | | 사용 시 힘 +3 버프 제공   | |  |
|  |   Weapon: wpn_longsword           |   | | 가치: 25 G                | |  |
|  |   Armor : arm_scale               |   | +---------------------------+ |  |
|  |   Shield: 없음                    |   |                               |  |
|  +-----------------------------------+   +-------------------------------+  |
|  > Info log line here...                                                    |
|  +-----------------------------------------------------------------------+  |
|  | [1~4] 선택 | [Tab/좌우] 영역 전환 | [상하] 탐색 | [Enter] 결정 | [ESC] 닫기 |  |
|  +-----------------------------------------------------------------------+  |
+-----------------------------------------------------------------------------+

### 3.5 설정 화면 (SettingsState)

```text
+-----------------------------------------------------------------------------+
|                                                                             |
|                               === SETTINGS ===                              |
|                                                                             |
|                                                                             |
|                  > Language (언어)      :  < 한국어 (KO) >                  |
|                    Text Scale           :  < 100% >                         |
|                    High Contrast        :  < ON >                           |
|                                                                             |
|                                                                             |
|                           --- Controls Guide ---                            |
|                                                                             |
|                      Move         : W/A/S/D or Arrow Keys                   |
|                      Menu Select  : Up/Down/Enter                           |
|                      Inventory    : I / Character Sheet: C / Close: ESC     |
|                      Automove     : Minimap Empty Floor Left Click          |
|                      Settings Menu: O Key (Town/Dungeon/Combat)             |
|                                                                             |
|                                                                             |
|                      [ Save & Go Back to Game (ESC) ]                       |
|                                                                             |
+-----------------------------------------------------------------------------+
```
```

### 3.6 상점 화면 TUI 레이아웃 (SHOP, SHOP_BUY, SHOP_SELL)

마을 Plaza에서 2번 `SHOP` 선택 시, 구매/판매 메인 및 서브상태로 분기되는 TUI 레이아웃 구조가 적용된다.

* **상점 메인 메뉴 (TownSubState::SHOP):**
```text
+-----------------------------------------------------+-----------------------+
|                                                     |  [PARTY STATUS]       |
|  === 무기 및 갑옷 상점 (SHOP) ===                   |  Gold: 100 G          |
|                                                     |  Members: 0 / 4       |
|  Shop Menu:                                         |                       |
|                                                     |  - Bag -              |
|  1. 장비 및 물약 구매 (Buy)                         |  - Empty bag -        |
|  2. 인벤토리 아이템 판매 (Sell)                     |                       |
|                                                     |                       |
|  ESC. 마을로 돌아가기 (Back)                         |                       |
|                                                     |                       |
|                                                     |                       |
+-----------------------------------------------------+-----------------------+
```

* **상점 구매 목록 (TownSubState::SHOP_BUY):**
```text
+-----------------------------------------------------+-----------------------+
|                                                     |  [PARTY STATUS]       |
|  === 무기 및 갑옷 상점 (SHOP) ===                   |  Gold: 100 G          |
|                                                     |  Members: 0 / 4       |
|  Shop Catalog (Buy):                                |                       |
|                                                     |  - Bag -              |
|  1. Dagger          - 10 G                          |  - Empty bag -        |
|  2. Longsword       - 30 G                          |                       |
|  3. Mace            - 20 G                          |                       |
|  ...                                                |                       |
|  8. Healing Potion  - 15 G                          |                       |
|                                                     |                       |
|  ESC. 뒤로가기 (Back)                               |                       |
+-----------------------------------------------------+-----------------------+
```

* **상점 판매 목록 (TownSubState::SHOP_SELL):**
```text
+-----------------------------------------------------+-----------------------+
|                                                     |  [PARTY STATUS]       |
|  === 무기 및 갑옷 상점 (SHOP) ===                   |  Gold: 100 G          |
|                                                     |  Members: 1 / 4       |
|  판매할 아이템 선택 (Sell):                          |                       |
|                                                     |  - Bag -              |
|  1. 롱소드 (+15 G)                                  |  - 롱소드 x1 -        |
|  2. 치유 물약 (+7 G)                                |                       |
|                                                     |                       |
|                                                     |                       |
|  ESC. 뒤로가기 (Back)                               |                       |
+-----------------------------------------------------+-----------------------+
```

## 4. 각 영역별 기능 상세 설명

### 4.1 1인칭 3D 와이어프레임 뷰 (Viewport)
* **좌표 투영 계산:** 플레이어가 가리키는 방향에 벽이 존재할 때 원근에 맞게 수직선과 수평선을 조합해 드로잉한다.
* **이동 시 애니메이션 (Deferred visual polish):** 0.1초 스크린 셰이크 또는 오프셋 시프트는 현재 구현·테스트되지 않았다. 별도 UI 변경과 raster 검증 전까지 출시 기능으로 주장하지 않는다.

### 4.2 미니맵 (MiniMap)
* **안개 효과 및 오토맵 고도화:**
  * 플레이어가 직접 밟은 바닥: 밝은 네온 그린 (`COLOR_BRIGHT_GREEN`, `#66FF66`).
  * 직접 밟지 않고 FOW만 해제된 바닥: 어두운 녹색 (`COLOR_MUTED`, `#114411`).
  * 탐험 과정에서 드러난 벽: 회색 (`#808080`) 사각형 아웃라인.
  * 탐험하지 않은 미답지: 검정색 (`COLOR_BG`, `#050B05`).
* **현재 위치와 방향:** 지나간 녹색 바닥 위에 청록색(`#66FFFF`) 방향 삼각형과 어두운 외곽선을 최상단으로 그린다. 색상뿐 아니라 삼각형 꼭짓점 방향으로 북·동·남·서를 구분한다.

### 4.3 로그 윈도우 (Log Window)
* **텍스트 스크롤:** 최대 5줄의 최근 로그 보관.
* **색상 강조:** 몬스터 조우(주황색 `COLOR_AMBER`), 대미지 피격/독 대미지(적색 `COLOR_RED`), 아이템 획득/회복(밝은 녹색 `COLOR_BRIGHT_GREEN`).

### 4.4 캐릭터 정보 관리 및 인벤토리 화면 (CharacterInfoState)
* **좌우 분할 TUI:** 좌측에 캐릭터 상세 능력치, 장착 장비, **현재 상태이상(독, 마비 등) 또는 버프 상태**를 출력한다.
* **상태 표시 정보:**
  * 건강한 상태: `Status: [OK]` (네온 그린)
  * 독 감염 상태: `Status: [POISONED]` (적색)
  * 마비 상태: `Status: [PARALYZED]` (적색)
  * 버프 상태: `STR: 15(+3) (+2)` (원래 능력치 스코어 뒤 괄호 안에 버프 보너스 표출, Amber색)
* **아이템 사용:**
  * 소모품 물약/스크롤을 가방에서 골라 `Enter`를 누르면, 캐릭터에 따라 효과가 연산 적용된다.
  * 독에 걸린 상태에서 `scr_cure` 사용 시 독 치유.
  * 마법사의 주문 슬롯이 깎인 상태에서 `pot_mana` 사용 시 주문 슬롯 +1 회복.
  * 전투 돌입 상태가 아니더라도 가방 내 버프 물약 복용 시 경고 로그("전투 중에만 효과가 지속됩니다")와 함께 복용 자체는 가능하지만 즉시 만료 처리 또는 복용 차단 처리.

### 4.5 전투 중 스킬/스펠 선택 오버레이 (TUI Overlay)
* 전투 턴 수행 캐릭터가 2번 `Skill / Spell` 입력 시, 하단에 팝업창 형태로 캐릭터가 가지고 있는 `m_skills` 목록(이름, 필요 레벨, 소모 주문 슬롯)이 세로로 나열된다.
* 숫자 키(1~9)를 입력해 사용할 스킬/스펠을 최종 결정할 수 있다.
* `ESC` 키 입력 시 상위 전투 선택 메뉴(`Attack / Skill / Item / Run`)로 취소 및 이전 복원된다.

### 4.6 설정 및 다국어 실시간 전환 (SettingsState)
* **실시간 언어 변경:** 언어 옵션 변경 시 `ko`, `en`, `ja`, `zh_tw`, `zh_cn` 리소스를 실시간 재로딩하여 하위 뷰에 즉시 투영한다.
* **접근성 설정:** text scale은 75~200% 범위에서 25% 단위로 조절하고 high contrast를 켜고 끌 수 있다. 설정은 OS별 per-user `config.json`에 원자 저장된다.
  * 150~200%에서는 Town/Combat이 짧은 조작·상태 label을 사용하고 CharacterInfo는 상세/가방을 한 화면씩 표시해 고정 패널 겹침을 피한다.
  * Linux raster gate는 5 locale × 75/100/200%에서 Title, Town 7 substate, Settings, CharacterInfo 상세/가방, Dungeon, Combat 및 선택 overlay를 실제 production draw 경로로 캡처한다.
  * OS 배율 기반 high-DPI와 장시간 동적 상태의 모든 조합은 실제 플랫폼 실행 전까지 `UNVERIFIED`다.
* **진입 키 통일:** 이동 조작과의 중복 충돌을 원천 차단하기 위해 설정 단축키는 `O` 키(Options)로 강제 지정된다.

### 4.7 상점 매매 시스템 및 서브 장소 FSM (SHOP)
* **상점 서브 FSM 구성:** 상점 진입 시 구매(Buy)와 판매(Sell)를 고르는 메인 서브메뉴(SHOP)를 제공하며, 키 입력에 따라 구매 카탈로그(SHOP_BUY) 또는 판매 인벤토리 목록(SHOP_SELL)으로 각각 진입 및 ESC를 통해 계층 구조로 되돌아간다.
* **아이템 매매 룰:**
  * **구매 (SHOP_BUY):** 고정된 8종의 물품 리스트와 골드 가격을 표시하며, 소지 골드 차감 후 공용 인벤토리에 즉시 추가된다.
  * **판매 (SHOP_SELL):** 숫자 단축키 `1`~`9`로 대상을 고른 뒤 가격 preview를 표시하고 Enter로 재확인해야 한다. Esc 취소는 파티와 save bytes를 바꾸지 않는다. 판매가는 원가의 **50%**다.
* **상태 변경 영속성:** 아이템 구매 또는 판매 완료 시 즉시 세이브 파일(`save.json`)을 갱신해 영속 데이터를 보존한다.

## 5. 구현 시 주의사항 및 요청사항

### 5.1 Turn 1 감사 remediation UI 계약

다음 계약은 기존 화면 설명과 충돌할 경우 우선한다.

```text
TitleState
  ├─ New Game ──> Confirm(New/Cancel) ──> TownState
  ├─ Continue ──> valid save ──────────> TownState
  │                corrupt/no save ────> persistent error banner
  ├─ Settings
  └─ Exit

Town/Guild
  └─ Recruit ──> Candidate Preview ──> Enter: Confirm / R: Reroll / Esc: Cancel

DungeonState ──> Boss Gate ──> CombatState(boss) ──> VictoryState ──> TitleState
CombatState  ──> TPK ─────────────────────────────> GameOverState ──> TitleState
```

* **파괴적 행동:** save 유무와 관계없이 New Game, 파티원 해고, 아이템 판매는 대상·결과를 먼저 보여주며 `Enter` 재확인 전에는 domain state와 save bytes를 바꾸지 않는다. `Esc`는 항상 취소다.
* **전투 아이템/단일 아군 주문:** `item/skill list -> target -> preview -> confirm/cancel` 순서다. 효과가 없는 대상은 confirm할 수 없고 자원과 턴을 소비하지 않는다.
* **Dungeon HUD:** renderer는 실제 Party snapshot과 현재 층/발견 목표를 입력받아 최대 4개 slot의 이름, 직업, HP/MaxHP, Dead, Poison, Paralysis, Bless를 그린다. 빈 slot은 고정 캐릭터명으로 채우지 않는다.
* **지속 피드백:** save failure, corrupt save, TPK, campaign completion은 로그 큐가 아니라 banner 또는 전용 State에 유지한다.
* **입력:** 모든 화면은 `Esc=취소/뒤로`, `O=설정 열기 또는 설정 닫기`, 방향키=선택 이동, `Enter=확정`을 공통 의미로 사용한다. 숫자 단축키는 표시된 항목과 정확히 일치해야 한다.
* **타이포그래피:** 고정 1024x768 TUI의 일반 본문과 보조문은 14px, 핵심 상태·선택은 16px 이상을 사용한다. 넘치는 목록은 scroll하고 문장은 wrap한다.

### 4.8 종료 저장 확인

* 활성 세션에서 창 닫기 저장이 `Saved`면 종료한다. `CommittedDurabilityUnknown` 또는 실패면 창을 유지하고 원인과 `Enter=재시도`, `Esc=저장 없이 종료`를 지속 표시한다.
* TPK/Continue 복구 실패의 `recoveryPending` 화면에서는 자동 저장을 시도하지 않는다. `Enter`는 checkpoint load 재시도, `Esc`는 저장 없이 종료이며, New Game은 Title의 기존 확인 흐름에서만 실행한다.
* CharacterInfo 상세는 HP/주문 슬롯뿐 아니라 Dead, Poison, Paralysis, STR/DEX buff와 Bless의 남은 턴을 텍스트로 표시한다.
* **색상:** 핵심 본문/선택/오류 텍스트는 배경 `#050B05` 대비 4.5:1 이상을 사용한다. 저대비 `#114411`은 장식/비활성 프레임에만 사용하며 의미 전달을 단독으로 맡기지 않는다.
* **다국어:** 화면에 표시되는 문장과 item/monster/skill/quest 이름은 localization key와 placeholder로 구성한다. 5개 locale에서 key 누락과 text bounds를 검증한다.
* 모든 UI 컨트롤은 키보드 입력을 기본으로 한다. 미니맵 클릭을 통한 자동 이동만 예외이다.
* 스킬 팝업 조작 모드 시에는 일반 전투 선택 모드 키(1~4)가 무효화되고 스킬 리스트 번호 키(1~N)와 취소 키(ESC)만 정상 동작하도록 입력 처리 분기(`CombatState` 내부 상태 전환)를 정밀히 작성해야 한다.
* 해상도 강제: `window.setSize(sf::Vector2u(1024, 768))` 호출 및 리사이즈 이벤트 시 비율 고정 처리를 준수한다.
* **설정 상태 복귀 시 실시간 언어 업데이트:** `SettingsState`에서 옵션 변경 후 복귀 시, 이미 그려진 이전 언어의 프레임이 렌더링에 남지 않도록 모든 상태(State)들의 `draw()` 함수 시작부에서 `updateTuiContent()`를 강제 기동하여 렌더링 텍스트를 실시간으로 재바인딩해 묘사해야 한다.
* **조작 키 중복 배제:** 던전 내 W/A/S/D 방향키(특히 S키 후진)와의 겹침을 방지하기 위해, 설정창 진입 단축키는 무조건 `O` 키를 사용해야 한다.
