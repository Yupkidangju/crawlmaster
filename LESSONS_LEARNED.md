# LESSONS_LEARNED.md (프로젝트 교훈 및 기술 회고 기록)

본 문서는 **Crawlmaster** 프로젝트 설계 및 개발 과정에서 도출된 한계점, 해결 전략, 향후 비슷한 수준의 C++ 프로젝트 진행 시 유의해야 할 아키텍처적 교훈을 기술한다.

---

## 1. 초기 설계 단계에서의 맹점 극복
* **SFML 윈도우의 이벤트 큐 폭주 문제:**
  * **문제:** 키보드 입력을 단순 루프 내 `sf::Keyboard::isKeyPressed`로 받아올 경우, 전진(W) 키를 한 번 눌렀을 때 플레이어가 60FPS 프레임마다 매번 1칸씩 전진하여 제어 불능 상태가 되는 맹점이 식별되었다.
  * **해결책:** 키 입력 처리는 `sf::Event::KeyPressed` 시스템 이벤트를 구독하는 Event Driven 방식을 동결하여, 키를 완전히 눌렀다 뗄 때 한 번만 이동 트리거가 격발되도록 `GameStateManager`에 격리 설계하였다.

* **3D 투영 연산 최적화:**
  * **문제:** 매 프레임 투영 행렬(Projection Matrix)을 빌드하고 모든 벽면 버텍스에 변환 연산을 적용하는 것은 2D 벡터 드로잉 라이브러리인 SFML 하에서 과도한 보일러플레이트 코드를 양산할 리스크가 있었다.
  * **해결책:** 시야 범위 4칸 이내의 와이어프레임 사영 수치를 상수로 사전 정의하는 고정 거리 수치 테이블(Fixed Distance Projection Table)을 고안했다. 이는 코드를 극도로 경량화하면서 80년대 고전 Wizardry 룩앤필을 효율적으로 연출하는 최적의 설계 결정이었다.

---

## 2. 메모리 안정성 확보 및 스마트 포인터 운용 규칙
* **메모리 누수 방지 원칙:**
  * 게임의 상태 전환(`GameState`) 시 이전 상태가 파괴되지 않아 백그라운드 메모리 누수가 일어날 위험성이 항상 존재한다.
  * 본 프로젝트는 전역 `std::unique_ptr<GameState>` 스마트 포인터를 통해 FSM 상태를 캡슐화하여, 새로운 상태가 할당되는 순간 이전 상태 객체가 컴파일러 수준에서 안전하게 소멸자(`~GameState()`)를 호출하며 해제되도록 메모리 경계를 동결했다.
  * 몬스터와 캐릭터의 상호작용, 아이템 정보 전송 시에도 생포인터를 넘기지 않고 `std::shared_ptr<Item>` 또는 상수 참조(`const Item&`)를 적극 사용하여 복사 오버헤드와 생명주기 꼬임 문제를 예방했다.

---

## 3. D&D 룰엔진 구현과 수치 연산 상의 미묘한 오차
* **C++ 정수 나눗셈의 내림 성질:**
  * **문제:** D&D 5e 공식 능력치 보정치 공식 `Modifier = (Score - 10) / 2`는 9점일 때 `-1`이 되어야 하나, C++ 정수 나눗셈은 `truncation towards zero`를 보이기 때문에 `-1 / 2` 결과가 0으로 도출되는 치명적인 수치 오차가 발생했다.
  * **해결책:** `int diff = score - 10; return (diff < 0) ? (diff - 1) / 2 : diff / 2;` 와 같이 정수 나눗셈 실행 전 음수 보정을 더해 소수점 아래로 강제 버림되도록 수정하여 해결했다.

## 4. 빌드 의존성 및 다중 스레드 환경 관리
* **Linux 환경 SFML 의존성:**
  * **문제:** CMake `FetchContent` 파이프라인에서 SFML 라이브러리를 동적 기동할 때, Linux 환경의 경우 udev, X11, OpenGL 개발 파일(`libudev-dev`, `libx11-dev`, `libgl1-mesa-dev`)이 누락되어 로컬 빌드가 실패하는 문제가 있었다.
  * **해결책:** 호스트 OS 시스템 라이브러리를 사전 설치하여 CMake가 정상적으로 native 윈도우 그래픽 헤더를 링크하도록 복구했다. 환경 의존성이 심한 외부 모듈은 빌드 가이드 문서(`BUILD_GUIDE.md`)에 반드시 명시하여 전파해야 한다는 교훈을 얻었다.

---

## 5. [v0.8.0 추가] 엄격한 C++ 컴파일러 경고와 헤더 전방 선언 관리
* **미사용 매개변수 경고 (-Werror=unused-parameter)의 컴파일 실패:**
  * **문제:** 다형성 구조 상 부모 클래스 인터페이스(`Item::applyEffect`, `Skill::execute` 등)는 다양한 컨텍스트를 위해 많은 매개변수를 요구하지만, 실제 하위 구체 클래스에서는 그중 일부만 사용하는 경우가 빈번하다. C++20 `-Werror` 옵션 하에서는 이러한 unused-parameter가 빌드를 깨뜨리는 장애물이 된다.
  * **해결책:** 구체 클래스의 구현부에서 사용하지 않는 매개변수 명을 주석 처리(`/*party*/`, `/*allies*/`)하는 표준적 방어 코딩을 체득하여, 억지로 변수를 참조하는 보일러플레이트 코드 없이 깨끗하게 경고를 회피했다.

* **Scoped Enum의 Elaborated Type Specifier 제한 문법:**
  * **문제:** `enum class` 타입을 선언부 매개변수로 지정할 때, C++ 표준에 의해 `enum class CharacterClass charClass` 처럼 `class` 키워드를 중복 Elaborate하는 것이 금지된다.
  * **해결책:** `CharacterClass charClass` 와 같이 명시하고, 헤더 상단에 `enum class CharacterClass;` 와 같은 적절한 전방 선언을 매칭하여 헤더 간 순환 참조를 막고 컴파일 안정성을 개선했다.

* **CMakeLists.txt 빌드 타겟 유지보수:**
  * **문제:** 신규 다형성 클래스 파일(`Skill.cpp`, `SkillFactory.cpp`)을 생성했으나 CMake 소스 지정에 누락되어 링커가 `undefined reference` 에러를 격발시켰다.
  * **해결책:** 기능이 독립된 소스 추가 시 CMakeLists.txt의 SOURCES 리스트와 테스트 하네스의 TEST_SOURCES 리스트를 수동으로 항상 동기화하여 빌드 타겟의 온전함을 담보했다.

---

## 6. [v0.9.0 추가] 실시간 번역 갱신 및 키 충돌 방지 설계 교훈
* **실시간 다국어 번역과 스택 뷰 갱신 타이밍:**
  * **문제:** 설정 화면(`SettingsState`)에서 언어 설정을 바꾼 뒤 이전 상태(마을, 던전, 전투 등)로 `popState`를 통해 복귀했을 때, 하위 상태(State)들의 UI 텍스트 레이블이 즉각 번역되지 않고 이전 상태를 그대로 유지하여 비주얼 갱신이 누락되는 결함이 존재했다.
  * **해결책:** `SettingsState`에서 돌아왔을 때의 화면 실시간 반응성을 확보하기 위해, 각 State의 `draw()` 루프 최상단에서 `updateTuiContent()`나 직접 번역 데이터를 `LocalizationManager::getInstance().get()`으로 매 프레임 재바인딩해 렌더링하도록 뷰 구조를 개선했다. 이로써 추가적인 복잡한 옵저버 패턴 연동 없이도 완벽한 실시간 반응성을 달성할 수 있었다.
* **W/A/S/D 조작 키와의 단축키 충돌 해결 (Design Decision):**
  * **문제:** 설정(Settings) 진입 키를 처음에는 `S` 키로 매핑했으나, 던전 탐험 화면(`DungeonState`)에서 `W/A/S/D`를 방향키로 조작하기 때문에 `S` 키(후진)와 설정 단축키 `S`가 중복 충돌하여 컴파일러가 `duplicate case value` 에러를 격발시켰다.
  * **해결책:** 설정 진입 키를 `O` 키(Options)로 통일화하였다. 텍스트 레이아웃 및 웰컴 가이드의 모든 텍스트도 `O` 키로 통일하여 정합성을 유지하였다. 다중 입력을 사용하는 복합 게임 씬 디자인 시에는 입력을 구조화하고 충돌 테스트를 반드시 선행해야 한다는 교훈을 얻었다.

---

## 7. [v0.9.2 추가] C++ 임시 객체 수명(Lifetime)과 UB, 그리고 i18n 핫픽스 교훈
* **임시 std::string 객체와 invalid iterator pair UB:**
  * **문제:** `lm.get("KEY").begin()`과 `lm.get("KEY").end()`처럼 임시 `std::string` 객체를 반환하는 연속 호출을 하나의 범위(`[begin, end)`)로 묶어 `sf::String::fromUtf8`에 전달할 경우, 두 iterator가 서로 다른 임시 객체를 가리키게 되어 Sentinel 도달이 불가능한 invalid iterator pair 미정의 동작(UB)이 발생했다. 이로 인해 타이틀이 사라지거나 렌더링 시 대량의 메모리성 깨진 글자가 출력되는 visual corruption 및 OOB read 위험이 식별되었다.
  * **해결책:** `LocalizationManager`에 `sf::String` 객체를 안전하게 구성하여 직접 반환하는 `getSf(key)` 헬퍼를 추가하였다. 헬퍼 내부에서는 지역 변수 `utf8Str`에 반환값을 담아 생명 주기를 격리 보장하므로 iterator pair의 미정의 동작을 완벽히 소멸시켰다. 임시 객체의 iterator를 인자로 전달할 때는 변수 할당을 통한 생명 주기 연장이나 캡슐화 헬퍼 기용이 절대적으로 필수적임을 깨달았다.
* **로컬 i18n 문자열 조립 분기와 다국어 정렬:**
  * **문제:** `TownState.cpp` 내에 20여 개 하드코딩된 `lm.get("Language") == "ko"` 조건문이 존재했으나, 번역 JSON에 `Language` 키가 없어 항상 거짓으로 판정되어 영어 분기로만 고정되고 5개 국어(한/영/일/번/간)의 실시간 다국어 지원 기조가 파괴되는 문제가 식별되었다.
  * **해결책:** `selectLang` 헬퍼 함수를 소스 코드 내에 설계하여, 5개 언어별 문자열을 한 번에 주입받아 `lm.getLanguage()` enum 분기에 따라 동적으로 반환하도록 리팩토링하였다. 이를 통해 C++ 단의 분기 코드를 현저히 정돈하면서도 5개 국어 i18n 정책을 완벽하게 만족할 수 있었다.
* **TestHarness 의존성 누락에 따른 빌드 실패:**
  * **문제:** `LocalizationManager`가 `sf::String`을 반환하게 됨에 따라 SFML String 헤더를 참조하게 되었고, 이로 인해 `TestHarness` 컴파일 시 SFML 헤더가 누락되어 빌드가 중단되는 링커 에러가 발생했다.
  * **해결책:** `CMakeLists.txt` 내 `TestHarness` 타겟의 `target_link_libraries`에 `sfml-graphics`, `sfml-window`, `sfml-system`을 추가하여 SFML 인클루드 경로 및 라이브러리 링크를 안전하게 완성했다. 공용 싱글톤 매니저 클래스 수정 시 이를 사용하는 유닛 테스트 빌드 타겟의 의존성 또한 함께 갱신되어야 함을 다시 한번 절감했다.

---

## 8. [v0.9.2-핫픽스 추가] CJK 다국어 폰트 Fallback 및 회귀 테스트 보강 교훈
* **CJK 글리프 결여와 이중 폰트 분기 로드 전략:**
  * **문제:** 기존 `neodgm.ttf` 폰트는 한글/영문 전용이어서 일본어 가나 및 CJK 한자 영역이 없어 일본어/중국어 설정 시 대체 사각형(ㅁ)으로 출력되는 UI 품질 훼손이 발생했다.
  * **해결책:** 다국어 번들 폰트 `DroidSansFallbackFull.ttf`를 추가 적재하고, `Game::getFont()`에서 현재 로컬라이징 언어가 CJK(일어/중문)인 경우 `m_cjkFont`를 반환하고 한글/영어는 기존 `m_font` (`neodgm.ttf`)를 반환하도록 설계했다. 이로써 런타임 다국어 폰트 렌더링 파이프라인을 유연하게 수립하였다.
* **유닛 테스트 단에서의 폰트 글리프 커버리지 검증 (`hasGlyph`):**
  * **문제:** 단순히 번역 키 로딩 유무만 체크하는 단위 테스트는 폰트가 특정 글리프(예: 일본어 'あ', 번체 '國')를 갖고 있지 않아 사각형으로 깨져 나오는 릴리즈 단계의 결함을 걸러낼 수 없었다.
  * **교정:** `hasGlyph`는 코드포인트 매핑 보조 검사일 뿐 실제 glyph 모양이나 State의 폰트 참조 갱신을 보증하지 않는다. 대표 혼합 문자열의 화면 검증, State 재바인딩, 번역 키 완전성 검사를 별도 게이트로 둬야 한다.

## 9. [v0.9.4] 테스트 저장소 격리와 번역 키 회귀 방지 교훈
* **문제:** 테스트가 기본 `save.json`을 사용해 사용자 진행 데이터를 변경할 수 있었고, Town 허브가 조회하는 키 9개가 번역 JSON에 없어 화면에 키 이름이 그대로 노출됐다.
* **해결책:** `Party`의 기본 저장 경로를 테스트 프로세스에서 고유 임시 디렉터리로 주입하고 사용자 저장소의 바이트 불변성을 단언했다. Town 허브 필수 키는 5개 언어에 모두 존재하는지 자동 검증한다.
* **유니코드 유효성 파괴 시 소스 코드 디코딩 에러 대응:**
  * **문제:** 소스 코드 내에 잘못 잘린 유니코드 바이트 조각이 포함되면 빌드 또는 에디터/백엔드 디코딩 시 `failed to detect charset` 에러가 격발되어 파일 쓰기가 차단되는 현상이 일어났다.
  * **해결책:** 망가진 소스 파일을 디스크 상에서 안전하게 `rm` 삭제하거나 초기화한 후, UTF-8 형식을 명시적으로 지정하여 새롭게 재생성하는 방어적 툴링 운용을 체득하였다.
# v0.9.4 감사 수정 교훈

- `hasGlyph`와 JSON key 검사는 실제 화면 가독성을 보증하지 않으므로 혼합 문자열 시각 검증을 별도 게이트로 둔다.
- 언어 변경 시 문자열 갱신과 SFML `sf::Text` 폰트 참조 갱신을 같은 생명주기에서 처리한다.
- 테스트 영속 데이터는 실행 위치와 분리된 임시 경로를 기본값으로 사용한다.

## 10. Turn 1 상용성 감사 remediation 교훈

* Debug에서 `assert`가 통과해도 Release에서는 assertion 자체가 제거된다. 제품 gate는 Release-safe expectation과 CTest 등록 여부를 함께 검증해야 한다.
* 손상 JSON을 기본값으로 덮는 동작은 복구가 아니라 데이터 손실이다. parser는 크기·schema·canonical registry를 검증하고, quarantine/backup을 초기화 명령과 분리해야 한다.
* atomic rename만으로 durability가 끝나지 않는다. file/directory fsync 결과와 `CommittedDurabilityUnknown`을 구분해 실제 commit 상태를 숨기지 않아야 한다.
* State constructor에서 stack 전이를 실행하면 아직 push되지 않은 객체와 root 전이가 경합한다. 첫 turn은 push 완료 후 update에서 시작하고 production controller event transcript로 잠근다.
* 콘텐츠 ID가 factory에 존재하는 것과 플레이어가 획득할 수 있는 것은 다르다. shop/starter/drop/quest source와 one-time completion을 함께 검증한다.
* build-tree 실행 성공은 package 성공이 아니다. 실제 archive를 풀어 checksum, resource verifier, arbitrary-CWD startup을 순서대로 실행해야 한다.
