# recovery_dna.md (AI Learning and Recovery DNA - Crawlmaster)

본 문서는 **D3D Protocol v1.0**에 의거하여, Crawlmaster 프로젝트의 유전 정보를 영구 보존하기 위해 작성된 복구 문서(Recovery DNA)다. 소스 코드가 유실되더라도 본 문서만으로 시스템 아키텍처를 95% 이상 복구할 수 있도록 상세히 기록한다.

---

## 1. Master Plan (마스터플랜 요약)
* **목표:** C++20과 SFML 기반의 1인칭 와이어프레임 3D 던전 탐험 RPG 게임 구축.
* **코어 루프:** 타이틀 로고 -> TUI 마을(길드 영웅 영입, 상점 거래, 교회 치료) -> 1인칭 20x20 미로 던전 탐험(10% 확률 랜덤 전투 인카운터) -> 턴제 전투 정산 -> 퀘스트 보고(골드/XP 획득, 수집 퀘스트 시 아이템 자동 소모).
* **하드코어 제약:** 전투 중 파티 전원이 사망할 경우 디스크에 보관된 세이브 파일(`save.json`)을 즉시 강제 포맷(초기화)하여 영구적 죽음(TPK) 실현.

---

## 2. Structure (디렉터리 및 소스 책임 구조)
```text
crawlmaster/
│
├── CMakeLists.txt              # CMake 3.20+, FetchContent를 통한 SFML 2.6.1, nlohmann/json 3.11.3 패키징
│
├── include/ / src/
│   ├── core/
│   │   ├── Game.hpp/.cpp       # SFML sf::RenderWindow 관리, 게임 루프(processEvents -> update -> render) 가동
│   │   ├── GameState.hpp       # 추상 상태 인터페이스 (handleInput, update, draw)
│   │   └── GameStateManager.hpp/.cpp  # FSM 상태 스택/전이 제어 매니저
│   │
│   ├── model/
│   │   ├── Character.hpp/.cpp  # D&D 6대 능력치, 레벨업(HP/주문슬롯 증가), AC 공식 구현
│   │   ├── Equipment.hpp       # 장구류 추상화 (WEAPON, ARMOR, SHIELD)
│   │   ├── ConcreteItems.hpp   # ConsumableItem, WeaponItem, ArmorItem 구체 구현
│   │   ├── Item.hpp            # 모든 아이템의 추상 인터페이스
│   │   ├── ItemFactory.hpp/.cpp # ID 문자열로 아이템 구체 할당
│   │   ├── Monster.hpp         # 몬스터 추상 인터페이스 및 ConcreteMonster 구현
│   │   ├── MonsterFactory.hpp/.cpp # 몬스터 확률 기반 가중치 랜덤 생성기
│   │   ├── Party.hpp/.cpp      # 4인 멤버 배열, 가방 인벤토리, save.json 로드/세이브 및 오염 파일 복구
│   │   └── Quest.hpp/.cpp      # KILL, COLLECT 타입 퀘스트 진행도 트래커
│   │
│   ├── controller/
│   │   ├── TitleState.hpp/.cpp # ASCII 로고 안내 및 시작 화면
│   │   ├── TownState.hpp/.cpp  # TUI 길드/상점/교회/성 분기 선택 및 로직
│   │   ├── DungeonState.hpp/.cpp # W/S/A/D 키보드 입력, 벽 충돌 판정 및 인카운터 롤
│   │   └── CombatState.hpp/.cpp # 선제권 턴 정렬, d20 명중/피해 전투 룰러, 스킬 처리
│   │
│   └── view/
│       └── DungeonRenderer.hpp/.cpp # 1인칭 깊이별 와이어프레임 벽면 그리기, 실시간 미니맵(FOW) 및 HUD 렌더러
│
└── src/test_harness.cpp        # D&D AC 산정, 레벨업 HP 상승, 퀘스트 수량 및 아이템 차감 단언 테스트 하네스
```

---

## 3. Core Logic (핵심 알고리즘)

### 3.1 DFS 기반 랜덤 미로 및 다중 순환 경로 생성 (`DungeonMap.cpp`)
1. 20x20 격자 맵 전체를 `TileType::WALL`로 충전.
2. `(1,1)` 에서 탐색 시작. 방향(북,동,남,서)을 무작위로 섞은 후, 2칸 전방이 범위 내이고 벽일 때 중간 벽과 대상 벽을 `TileType::EMPTY`로 뚫으며 재귀 호출(`generateDFS`).
3. 미로가 완성되면, 가로지르는 통로 사이에 낀 내부 벽 중 양 끝이 통로인 가로/세로 벽을 무작위로 8개 제거(`createLoops`)하여 단일 미로에서 순환 경로를 가진 복잡한 미로로 수정.

### 3.2 1인칭 3D 원근 사영 테이블 (`DungeonRenderer.cpp`)
* 2D 평면 위에 고속 렌더링하기 위해 시야 깊이 $d \in [0, 3]$에 따라 좌표 상수를 동결:
  * $d=0$ (좌우 측벽): Y축 20~748, 폭 350
  * $d=1$ (전방 벽): 높이 320 (Y축 100~420), 폭 200
  * $d=2$ (전방 벽): 높이 200 (Y축 160~360), 폭 120
  * $d=3$ (전방 벽): 높이 100 (Y축 210~310), 폭 60
* 플레이어 시선 방향(북,동,남,서)을 기준으로 전방의 벽 셀을 탐색하여 깊이 $d$와 위치에 따라 벽 아웃라인 선분을 드로잉.

### 3.3 d20 전투 룰엔진 (`CombatState.cpp` / `Character.cpp`)
* **명중 판정:** $d20 + \text{보정치} + 2\text{(숙련 보너스)} \ge \text{Target AC}$. (자연수 20일 경우 크리티컬 히트로 무기 대미지 주사위 2회 롤 합산). 자연수 1일 경우 즉각 빗나감.
* **민첩 보정치 선제권 정렬:** 전투 진입 시 아군과 적군의 민첩 보정치 + d20 결과값을 내림차순 정렬하여 `TurnEntity` 배열 완성.
* **클래스별 특화 스킬:**
  * **전사 (Warrior):** `Slash` (명중 보정 +2 및 물리 대미지 +2 가산).
  * **마법사 (Mage):** `Magic Missile` (주문 슬롯 1 소모, 100% 자동 명중 magic 대미지 1d4 + 1).
  * **성직자 (Cleric):** `Cure Wounds` (주문 슬롯 1 소모, 아군 HP 1d8 + WIS 보정치 치유).
  * **도적 (Rogue):** `Sneak Attack` (d20 롤링, 명중 시 단검 1d4 + 기습 대미지 1d4 가산).

---

## 4. Decision History (설계 의사결정)
1. **Event-Driven 키 입력 처리:** 실시간 키 검사 시 한 번 눌렀을 때 수십 칸 전진하는 오작동 방지를 위해 `sf::Event::KeyPressed`를 바인딩하여 1회 조작당 1회 이동 보장.
2. **세이브 안정성 경계 (Sandboxing):** 사용자의 악의적인 `save.json` 훼손(문자열 필드 주입 등) 시 크래시를 차단하도록 `Party::loadFromFile` 전체에 `try-catch` 구문을 삽입하고, 에러 시 즉시 `resetToDefault()`를 호출해 디폴트 세이브 파일로 안전 강제 복구하는 보안 경계 수립.
3. **완전한 3D 연산 배제:** 2D 드로잉 라이브러리에서 복잡한 투영 행렬 계산을 제거하기 위해 1인칭 사영 좌표 상수를 사전 계산 테이블화하여 코드 크기를 대폭 축소하고 레트로 Wizardry 네온 와이어프레임 감성을 극대화.

---

## 5. Pitfalls (실패 예방 및 문제 해결 지식)
* **D&D 보정치 정수 내림 버그:** C++의 정수 나눗셈 `(Score - 10) / 2`는 9점일 때 `0`을 반환하므로 D&D 규칙의 `-1`과 어긋난다.
  * *조치:* `int diff = score - 10; return (diff < 0) ? (diff - 1) / 2 : diff / 2;` 와 같이 음수 정수 분기를 추가해야 D&D 5e 내림 공식이 정확히 매핑된다.
* **Linux SFML FetchContent 링크 의존성:** Linux 시스템에서 SFML 빌드 시 udev 및 X11 OpenGL 라이브러리가 없으면 컴파일 에러 발생.
  * *조치:* 호스트 개발 머신에 `libudev-dev`, `libx11-dev`, `libgl1-mesa-dev` 패키지를 선행 설치하도록 가이드한다.
* **한글 직업명 폰트 깨짐 버그:** IBM PC 영문 전용 `PerfectDOSVGA437.ttf` 폰트 사용 시 캐릭터 상태창의 직업명 한글(전사, 마법사 등)이 깨져서 `ㅁ`으로 노출되는 문제 발생.
  * *조치:* 한글/영문을 동시에 지원하는 픽셀 폰트인 `neodgm.ttf` (네오둥근모)를 에셋에 다운로드하고, `Game::loadResources`에서 이를 우선 로드하도록 분기를 수정하였다. 또한 SFML의 `sf::Text::setString()`은 멀티바이트 UTF-8 `std::string`을 ANSI로 오해하므로 한글이 포함된 문자열은 반드시 `sf::String::fromUtf8(str.begin(), str.end())`로 래핑하여 넘기도록 모든 UI 텍스트 드로우 부분을 핫픽스하여 완벽히 해결했다.
* **전부 몬스터 턴에서의 전투 멈춤 버그:** `CombatState` 기동 및 `nextTurn()` 제어 흐름 내에서 몬스터 공격 실행 후 다음 턴으로 순환하는 `nextTurn()` 연쇄 구동이 누락되어 멈추는 문제 발생.
  * *조치:* 몬스터 턴 행동 수행 완료 직후 `nextTurn(); return;`을 삽입해 몬스터 턴이 다 돌고 다시 아군의 입력 대기 상태로 루프가 부드럽게 이어지도록 제어 흐름을 복구했다.
* **전투 복귀 시 던전 맵/좌표 초기화 버그:** 전투 시작 및 종료 시 `changeState`를 매번 호출하여 기존 `DungeonState` 정보가 파괴되고 맵이 매번 DFS로 신규 난수 재작성되며 플레이어가 `(1,1)`로 강제 복구되는 문제 발생.
  * *조치:* `GameStateManager`에 정의된 스택 구조를 가동했다. 몬스터 조우 시 `pushState`를 사용하여 기존 던전 상태를 일시 중지(Pause)해두고, 전투가 승리하거나 도망쳐 복귀할 때는 `popState`를 호출해 활성 전투 상태만 소멸시킴으로써 기존 탐험 지점의 맵 구조와 플레이어 좌표를 무손실 재개하도록 변경했다.

---

## 6. UI/Design Layout (디자인 사양 요약)
* **화면 해상도:** 1024x768 고정, 마우스 제거 및 100% 키보드(숫자키, 방향키, Enter, ESC) 제어.
* **레이아웃:** 좌측 1인칭 와이어프레임 창(700x500), 우측 상단 미니맵 FOW 표시, 우측 하단 파티 상태창, 하단 실시간 텍스트 스크롤 로그창 구성.
* **색상 팔레트:** 레트로 네온 그린 (`#33FF33`), 주황색 경고/골드 (`#FFB000`), 피해 적색 (`#FF3333`).
