# CHANGELOG.md (변경 이력 로그)

모든 중요 기능 추가, 리팩토링, 버그 수정 내역은 본 문서에 SemVer(Semantic Versioning) 기준으로 성실히 기록한다.

---

## [Unreleased] - Turn 1 audit remediation

### Hosted gate fixes
* Linux hosted runner에서 production-linked RNG Continue test가 DISPLAY 부재로 중단되지 않도록 해당 CTest를 Xvfb 경로에 연결했다. Windows는 기존 직접 실행 경로를 유지한다.
* Windows hosted gate가 MSYS2 MinGW를 우연히 선택하지 않도록 Visual Studio 17 2022 x64 generator와 명시적 Release config를 사용한다.
* MSVC `/W4 /WX`를 유지하면서 Windows persistence/resource 경로의 deprecated CRT 호출을 secure CRT로 교체하고 테스트·CharacterInfo inventory·Combat 대상/turn index 경계의 암시적 손실 가능 정수 변환을 제거했다.

### Re-audit 11 remediation
* 공격형 Skill을 일반 공격과 동일한 `CombatRules` 명중/Bless/natural roll/무기 dice/type/Skeleton mitigation 경로로 통합했다.
* schema v2 checkpoint에 raw RNG draw count를 추가하고 Continue가 별도 process에서도 seed/stream을 복원하도록 수정했다.
* 19 item, 8 monster, 12 skill, 3 quest의 이름/설명과 전투·상태 로그를 5 locale catalog/placeholder로 이동했다.
* Town 전체 substate와 Combat overlay에서 `O` 설정 진입을 일관되게 처리하고 200% 대형 텍스트 전용 compact layout을 추가했다.
* 혼합 CJK/ASCII가 tofu로 렌더되던 Droid font를 Noto Sans CJK로 교체하고 Ubuntu Mono 파일명을 내부 family와 일치시켰다.
* 5 locale × 3 scale의 font 및 production State/substate raster, source literal guard, artifact SPDX generator, OSV/Grype gate를 추가했다.
* root legacy `save.json`을 `tests/fixtures/save_v1.json`으로 이동하고 Skill API/screen-shake 문서 drift를 정리했다.
* Linux 전체 CTest 13개와 MinGW Windows 전체 target compile/ZIP 정적 검증을 추가했다. Hosted MSVC/attestation은 아직 `UNVERIFIED`다.

### Added (추가됨)
* 한 층 수직 슬라이스에 Door 랜드마크, 최종 BossGate, Dragon Whelp 보스, Victory/Game Over 상태를 연결했다.
* 3개 canonical quest, 19개 item 획득원 registry, monster drop/quest item reward와 완료 이력을 추가했다.
* HUD/Combat/Content/Agency 계약 테스트와 CTest 등록, Linux/Windows CI, CMake install/CPack, SPDX 직접 의존성 manifest 및 third-party license bundle을 추가했다.
* Text Scale과 High Contrast 설정, 모집 preview/reroll/confirm, New/Continue 분리, 판매/해고/전투 아이템 확인 흐름을 추가했다.

### Changed (변경됨)
* 모든 게임 난수를 한 session seed의 `std::mt19937` stream으로 통합했다.
* 전투는 weapon dice count/type, Bless, natural 1/20, Skeleton resistance, 장비 class/STR 제한, tier gold 공식을 domain 규칙으로 사용한다.
* save/config를 OS별 per-user path와 schema v2로 이동하고 v1 migration을 지원한다. 활성 dungeon 좌표는 저장하지 않는 town-checkpoint 정책으로 확정했다.
* 오디오는 현재 비목표이므로 작동하지 않는 BGM/SFX 설정 UI를 text/accessibility 설정으로 교체했다.
* SFML과 nlohmann/json을 immutable commit으로 고정하고 SFML을 정적 링크한다.

### Fixed (수정됨)
* Release에서 `assert`가 제거되어 TestHarness가 빌드되지 않던 문제와 CTest 0건 문제를 수정했다.
* 손상 save/config가 정상 진행을 자동 초기화하던 문제를 quarantine/backup 복구로 수정했다.
* TPK가 stale DungeonState를 남기고 정상 save를 초기화하던 문제를 root replace와 비파괴 Game Over로 수정했다.
* Dungeon HUD의 고정 4인/HP 표시를 실제 Party snapshot으로 교체했다.

### Security / Remaining gates
* 임시 파일 flush/fsync, backup 회전, atomic replace, 심볼릭 링크 대상 거부와 failure regression을 추가했다.
* Windows hosted/runtime, 장시간 밸런스, 전체 locale 실화면, 폰트 권리 및 legal/support identity는 아직 `UNVERIFIED` 또는 `Human Review Required`다.

## [0.9.4] - 2026-07-13

### Fixed (수정됨)
* `TestHarness`가 사용자 실행 경로의 `save.json`을 변경하던 문제를 수정했다. 테스트 시작 시 프로세스별 기본 세이브 경로를 고유 임시 디렉터리로 전환하고, 실행 전후 사용자 `save.json` 및 `config.json` 바이트 불변성을 단언한다.
* Town 허브가 조회하지만 5개 언어 리소스에 없던 9개 번역 키를 추가해 raw key가 화면에 노출되는 문제를 수정했다.

### Added (추가됨)
* `testTownHubLocalizationKeyCoverage` 회귀 테스트를 추가해 Town 허브의 9개 필수 키가 한국어·영어·일본어·중국어 번체·중국어 간체에 모두 존재하는지 검증한다.

### Changed (변경됨)
* 기존 `hasGlyph` 검증은 실제 화면 가독성의 보증이 아니라 코드포인트 보조 검사임을 명확히 한다. CJK 혼합 문자열 렌더링과 폰트 provenance 검증은 `audit_report_8.md`의 미해결 릴리즈 게이트로 유지한다.

---

## [0.9.3] - 2026-07-13

### Fixed (수정됨)
* `TestHarness` 실행 파일에도 번역 JSON과 폰트 자산을 빌드 후 복사하도록 CMake 후처리를 추가하여, `build/` 디렉터리에서 직접 실행할 때 오래된 자산으로 글리프 회귀 테스트가 실패하는 문제를 수정했다.

---

## [0.9.2] - 2026-07-13

### Added (추가됨)
* `TestHarness`에 Town 7대 서브상태 및 Combat 헤더의 실시간 다국어 로딩 및 getSf() 연속 호출 시의 메모리 정합성을 검증하는 `testTownAndCombatUIStatesI18nSafety` 회귀 테스트 추가 ([v0.9.2])
  * **[v0.9.2-핫픽스]** 5개 언어 번역 리소스 JSON 파일에 수록된 모든 CJK/일어 글리프의 폰트 지원 여부를 검증하는 `hasGlyph` 전체 codepoints 검증 로직 추가 연동.
* **[v0.9.2-핫픽스]** CJK 일어/중문 글리프를 완벽히 지원하는 다국어 번들 폰트 `DroidSansFallbackFull.ttf` 추가 및 `Game::getFont()`에서 현재 설정 언어에 따른 동적 폰트 스위칭/폴백 전략 구현.

### Changed (変更됨)
* `LocalizationManager` 싱글톤의 전역 BGM/SFX 볼륨 기본값을 `spec.md` 동결 스펙 사양에 부합하도록 BGM 60, SFX 80으로 상향 조정 ([v0.9.2])
* `LocalizationManager`에서 config.json 영속 저장 및 로딩 시 사용되는 JSON 키를 snake_case(`bgm_volume`, `sfx_volume`)에서 camelCase(`bgmVolume`, `sfxVolume`)로 변경하여 마스터 스펙 계약과 정렬 ([v0.9.2])
  * 하위 호환성을 위해 구형 snake_case 키가 감지될 시 자동 마이그레이션하여 load하도록 기능 보완.
* **[v0.9.2-핫픽스]** `TownState.cpp` 소스 코드 내부의 하드코딩되었던 20여 개 다국어 `selectLang` 분기 코드를 전면 철폐하고, 플레이스홀더 치환 함수(`replacePlaceholder`)를 경유한 JSON 번역 키 매핑으로 일원화.

### Fixed (수정됨)
* `TownState.cpp` 내 7대 서브상태(HUB, GUILD, SHOP, SHOP_BUY, SHOP_SELL, TEMPLE, CASTLE) 타이틀 갱신 시, 임시 std::string 객체의 end iterator를 사용하여 발생하는 invalid iterator pair 미정의 동작(UB) 및 타이틀 텍스트 소실 버그 수정 ([v0.9.2])
  * `LocalizationManager`에 `sf::String` 반환 헬퍼 `getSf()`를 추가하여 UTF-8 안전성을 보장하고, 타이틀이 항상 화면에 노출되도록 보장.
* `CombatState.cpp:792`에서 COMBAT_TITLE 헤더 갱신 시 `setString(std::string)`을 직접 사용하여 ANSI 변환 경로를 타고 한글이 깨지는 인코딩 오류 수정 ([v0.9.2])
  * `getSf("COMBAT_TITLE")`을 적용하여 UTF-8 형식으로 안전하게 처리하도록 정정.
* `TownState.cpp` 내 20여 개 다국어 문자열 조립 분기(`lm.get("Language") == "ko"`)가 실제 번역 JSON에 키가 없어 작동하지 않고 영어로 고정되던 결함 핫픽스 ([v0.9.2])
  * 5개 언어(한국어, 영어, 일본어, 중국어 번체, 중국어 간체)에 대응할 수 있도록 `selectLang` 다국어 선택 헬퍼를 적용하여 실시간 다국어 대응 완성.
  * **[v0.9.2-핫픽스]** 번체/일본어 번역에 혼입되었던 한국어 조사(`의`)를 올바른 다국어 표기(`的`, `の`)로 전면 교정 및 JSON 번역 키로 이관.
* `CMakeLists.txt` 내 `TestHarness` 빌드 타겟이 `LocalizationManager` 변경사항으로 인해 SFML 라이브러리를 필요로 하게 됨에 따라 링크 에러가 나던 의존성 빌드 오류 수정 ([v0.9.2])
  * `TestHarness` 타겟에 `sfml-graphics`, `sfml-window`, `sfml-system` 의존성을 명시적으로 타겟 링크 라이브러리에 연동.

## [0.9.1] - 2026-06-19

### Added (추가됨)
* 상점 메뉴 내 구매(`SHOP_BUY`)와 판매(`SHOP_SELL`) FSM 상태 분리 적용 및 인벤토리 아이템 판매 기능 구현 ([v0.9.1])
  * 판매 시 아이템 가격(`getGoldValue()`)의 50%에 해당하는 골드가 파티에 가산되고 가방에서 삭제 처리됨.
  * `TestHarness`에 판매 골드 연산 및 인벤토리 직렬화 보존을 검증하는 `testShopSelling` 유닛 테스트 보완.
* D3D AI 구현 문서 표준(`AI_IMPLEMENTATION_DOC_STANDARD.md`)에 따른 코어 사양 문서 전면 갱신 ([v0.9.1])
  * `spec.md`: 클래스별 상세 정의 내 개별 스킬/주문의 매핑 ID 명기, 상점 품목 제한(8종 기본) 조항 명시, `save.json`/`config.json` 직렬화 스키마 타입 계약(Typed Contracts) 및 초기값 설계 구체화.
  * `designs.md`: 상점 TUI (메인, 구매, 판매) ASCII 아트 레이아웃 구조도 추가.
  * `IMPLEMENTATION_SUMMARY.md`: 수정한 파일들의 책임 명시.

### Fixed (수정됨)
* 마을 성주실(`CASTLE` 서브스테이트)에서 퀘스트 수주 및 보고 시 골드/경험치/수집물 차감 정산 정보가 세이브 파일(`save.json`)에 즉각 영속 저장되지 않고 유실될 수 있던 결함 해결을 위해 명시적인 `party.saveToFile()` 추가 적용 ([v0.9.1])

## [0.9.0] - 2026-06-19

### Added (추가됨)
* 다국어 i18n 로컬라이제이션 매니저 (`LocalizationManager`) 싱글톤 구현 ([v0.9.0])
  * `assets/lang/` 아래 5개 국어(한국어, 영어, 일본어, 중국어 번체, 중국어 간체) JSON 번역 리소스 연동.
* 게임 초기 화면 및 플레이 도중 실시간 설정 변경이 가능한 `SettingsState` 구현 ([v0.9.0])
  * 가상 오디오 BGM/SFX 볼륨 제어, 언어 변경, 단축키 조작 가이드 표출 기능 탑재.
  * 전역 게임 설정 영속화를 위한 `./config.json` 세이브/로드 연동.
* 캐릭터 정보 화면(`CharacterInfoState`) 및 전투 화면(`CombatState`) TUI 다국어 치환 완료 ([v0.9.0])
* `TestHarness`에 언어 실시간 교체 검증, Fallback Fallback 확인, config.json 설정 영속 복구 유효성을 검증하는 `testLocalizationI18n` 유닛 테스트 보완 및 전체 통과 완료 ([v0.9.0])

### Changed (변경됨)
* W/A/S/D 이동 충돌(S키 뒤로 가기)을 방지하기 위해 게임 내(마을, 던전, 전투) 설정 화면 진입 단축키를 기존 `S` 키에서 `O` 키(Options)로 변경 및 일관성 재정립 ([v0.9.0])
* 설정 완료 및 복귀 시 하위 씬들의 텍스트가 실시간으로 번역되도록 `draw()` 시점 강제 갱신 로직 탑재 ([v0.9.0])

## [0.8.0] - 2026-06-19

### Added (추가됨)
* 객체지향형 `Skill` 추상 인터페이스 및 직업별 스킬/주문 12종 신규 구현 ([v0.8.0])
* 캐릭터 모델(`Character`)에 독, 마비 등의 상태이상 및 버프 속성 탑재, 턴 진행 업데이트 루프 연동 ([v0.8.0])
* 신규 무기/방어구 및 소모품 6종(그레이트소드, 마법 지팡이, 레이피어, 플레이트 아머, 타워 실드, 고급 치유 물약, 마나 물약, 힘의 물약, 민첩의 물약, 해독 스크롤) 추가 연동 및 팩토리 매핑 ([v0.8.0])
* 전투 시 `Skill/Spell` 선택 팝업 TUI 윈도우 인터페이스 구현 및 타겟팅 연동 ([v0.8.0])
* 물약 사용 효과(마나 충전, 스탯 버프, 해독 등) 분기 구현 ([v0.8.0])
* 몬스터 4종 추가 (거대 거미: 독 유발, 구울: 마비 유발, 고블린 주술사: 매직 미사일 사격, 새끼 용: 화염 브레스 및 DEX 구원 투사) ([v0.8.0])
* `TestHarness`에 상태이상 데미지 누적 및 버프 갱신 테스트 코드 탑재 및 검증 ([v0.8.0])

---

## [0.7.0] - 2026-06-19

### Added (추가됨)
* 마을 및 던전 탐험 화면에서 'I'/'C' 키 입력 시 진입 가능한 통합 캐릭터 정보 & 인벤토리 화면(`CharacterInfoState`) 구현 ([v0.7.0])
* 좌우 분할식 TUI UI 디자인 적용 (좌측: 캐릭터 상세 6대 스탯/보정치/AC 및 3종 장비 장착 현황, 우측: 공용 골드/인벤토리 가방 스크롤 렌더링 및 하단 설명 팝업 박스) ([v0.7.0])
* 포커스 영역(장착 슬롯 vs 가방 목록) 스위칭(Tab/방향키) 및 Enter 연동을 통한 양방향 장비 착용/해제/물약 사용 스왑 로직 구현 ([v0.7.0])
* D&D 룰에 의거한 클래스별 장비 착용 제한 규칙 적용 (마법사: 중무기/중갑/방패 장착 불가, 도적: 체인 메일/방패/롱소드 장착 불가) ([v0.7.0])
* `TestHarness`에 캐릭터 장착 슬롯 해제, 가방 아이템 스왑 반환, 마법사 장착 차단 및 자동 착용 교체를 검증하는 `testCharacterEquipmentSystem` 유닛 테스트 탑재 및 통과 완료 ([v0.7.0])

---

## [0.6.0] - 2026-06-18

### Added (추가됨)
* 미니맵 오토맵 고도화 렌더링 적용 (직접 밟은 바닥: 밝은 네온 그린, 안개만 걷힌 바닥: 어두운 녹색, 밝혀진 벽: 회색 표시) ([v0.6.0])
* 미니맵 영역 마우스 왼쪽 클릭 감지 및 BFS(너비 우선 탐색) 기반 최단 경로 자동 이동 알고리즘 구현 ([v0.6.0])
* 자동 이동 중 키보드 키를 누르면 이동이 취소(캔슬)되는 기능 추가 ([v0.6.0])
* 자동 이동 매 단계마다 플레이어 방향 전환, FOW 시야 갱신 및 10% 확률의 몬스터 인카운터 전투 전이 예외 처리 구현 ([v0.6.0])
* `TestHarness`에 BFS 최단 경로 탐색, 안개 차단 및 벽 차단 정상 동작을 보증하는 `testDungeonAutoMoveBFS` 유닛 테스트 탑재 및 통과 완료 ([v0.6.0])

---

## [0.5.0] - 2026-06-17

### Added (추가됨)
* 4단계 감사 프레임워크(정합성, 위험요소, 아키텍처, 로드맵)를 통한 전체 시스템 검증 완료 ([v0.5.0])
* 단위 테스트 하네스(`TestHarness`)를 통한 D&D 룰엔진, JSON 직렬화/역직렬화, DFS 미로 생성, 손상 세이브 복구, 퀘스트 추적 및 아이템 자동 차감 100% 통과 보증 ([v0.5.0])
* 1024x768 해상도 하에서 레트로 네온 그린 룩앤필의 1인칭 와이어프레임 뷰포트 및 TUI 레이아웃 비주얼 최종 정돈 ([v0.5.0])
* 한글과 영문을 동시 지원하는 레트로 비트맵 폰트 `neodgm.ttf` (네오둥근모) 에셋 추가 ([v0.5.0])
* `.antigravity/archive` 지식 저장소 내 지식 복구 DNA 아카이브 덤프 스크립트화 대비 구성 및 최종 폴리싱 완료 ([v0.5.0])

### Fixed (수정됨)
* 캐릭터 이름 뒤 한글 직업명(전사, 마법사 등) 및 던전 로그 등이 SFML 문자열 인코딩 유실로 인해 `ㅁ` 기호로 깨지던 폰트 에러 수정 (`sf::String::fromUtf8` 명시 변환 핫픽스 적용) ([v0.5.0])
* 전투 종료(승리/도망) 시 `changeState` 대신 FSM 스택 방식인 `pushState`/`popState` 구조를 적용하여, 모험 중이던 던전 맵 구조 및 좌표 롤백(입구 회귀) 현상 수정 ([v0.5.0])
* 전투 턴 연산 중 몬스터 AI 공격 완료 후 다음 턴으로 전이하는 `nextTurn()` 호출이 누락되어 전투가 멈추던 버그 수정 ([v0.5.0])
* TUI 변수 범위 섀도잉 및 Linux 컴파일 시 SFML 라이브러리 의존성 링크 에러 보정 완료 ([v0.5.0])

---

## [0.4.0] - 2026-06-17

### Added (추가됨)
* 추상 `Monster` 인터페이스 및 구체 `ConcreteMonster` 구현과 `MonsterFactory` 랜덤 몬스터 스폰 기믹 구축 ([v0.4.0])
* 퀘스트 종류(`KILL`, `COLLECT`) 및 보상 정산과 영속 저장을 지원하는 `Quest.cpp` 구현 ([v0.4.0])
* 던전 탐험 이동 시 10% 확률로 몬스터 인카운터를 격격하고 전투 상태로 전환하는 기믹 구축 ([v0.4.0])
* 턴제 전투 엔진 `CombatState.cpp` 구현 (Initiative DEX 턴 정렬, d20 명중/피해 주사위 판정, 크리티컬 히트) ([v0.4.0])
* 클래스별 전투 스킬/마법 4종(Slash, Magic Missile 슬롯 차감, Cure Wounds 자동 치료, Sneak Attack 기습 피해) 구현 ([v0.4.0])
* 전투 승리 시 경험치/골드 분배 및 활성 퀘스트 진행 킬카운트 동기화 세이브 연계 ([v0.4.0])
* 아군 파티 전원 전멸(TPK) 시 세이브 데이터를 강제 삭제 리셋하고 타이틀 화면으로 강제 복귀시키는 하드코어 롤백 기동 ([v0.4.0])
* 마을 퀘스트보드 서브스테이트 `CastleSubState` 구현 (수주 상태 동적 표출 및 수집 퀘스트 완료 시 실제 아이템 차감 검증) ([v0.4.0])
* `TestHarness`에 퀘스트 킬 진척도 수량 체크, 수집품 완료 보고 및 가방 아이템 삭제 단언 테스트 추가 및 100% 통과 ([v0.4.0])

---

## [0.3.0] - 2026-06-17

### Added (추가됨)
* D&D 5e 규칙 기반 6대 능력치 및 수정된 내림 보정치 공식을 탑재한 `Character.cpp` 구현 ([v0.3.0])
* 캐릭터 4d6 Drop-Lowest 방식 능력치 주사위 롤러 구현 ([v0.3.0])
* 클래스 4종(전사, 마법사, 도적, 성직자) 기본 HP, 주문 슬롯 설정 및 레벨업(만렙 3) 공식 연동 ([v0.3.0])
* D&D 5e 공식 갑옷(체인 메일, 스케일 메일 등) 장착 슬롯 및 AC 산정 연산 구현 ([v0.3.0])
* 최대 4인의 캐릭터 배열, 공용 인벤토리 및 골드를 영속 세이브/로드하는 `Party.cpp` 구현 ([v0.3.0])
* 세이브 데이터 강제 오염(오동작 데이터 파싱) 시 크래시를 차단하고 초기값으로 안전 롤백하는 예외 격리 안전 경계 구현 ([v0.3.0])
* 마을 상태(`TownState.cpp`)에 TUI 기반 길드(영웅 생성/해제), 상점(장비 8종 매매), 교회(골드 소모 HP/슬롯 완치) TUI TUI 구현 ([v0.3.0])
* `TestHarness`에 D&D AC 산정, addXp 레벨업 HP 상승 검증, 예외 데이터 파손 복구 검증 테스트 추가 및 100% 통과 ([v0.3.0])

---

## [0.2.0] - 2026-06-17

### Added (추가됨)
* 20x20 크기의 DFS(깊이 우선 탐색) 무작위 미로 생성 로직 구축 (`DungeonMap.cpp`) ([v0.2.0])
* 미로 생성 완료 후 임의의 벽 8개를 뚫어 다중 순환 경로를 형성하는 로직 탑재 ([v0.2.0])
* 1인칭 원근 사영 테이블 기반 와이어프레임 3D 뷰포트 렌더러 구현 (`DungeonRenderer.cpp`) ([v0.2.0])
* FOW(Fog of War) 안개 해제 방식의 20x20 미니맵 렌더링 추가 ([v0.2.0])
* 하단 로그창 수동/자동 메시지 스크롤 렌더링 및 HUD 네온 테두리선 출력 ([v0.2.0])
* `W/S/A/D` 이동 입력 바인딩 및 맵 경계/벽 충돌 판정 구현 ([v0.2.0])
* 계단(`UPSTAIRS`) 타일 도달 시 `ESC` 키를 통해 마을로 복귀하는 양방향 FSM 연동 ([v0.2.0])
* `TestHarness`에 BFS 그래프 탐색 미로 연결성 단언 테스트 추가 및 100% 통과 검증 ([v0.2.0])

---

## [0.1.0] - 2026-06-17

### Added (추가됨)
* 프로젝트 마스터플랜 및 명세 문서 `spec.md` 생성 ([v0.1.0])
* 1인칭 와이어프레임 UI 레이아웃 사양 및 디자인 토큰 명세 `designs.md` 생성 ([v0.1.0])
* 전체 런타임 흐름, 파일별 책임 구조 및 사영 수치 테이블 명세 `IMPLEMENTATION_SUMMARY.md` 생성 ([v0.1.0])
* CMake FetchContent 의존성 및 빌드 검증 가이드 `BUILD_GUIDE.md` 생성 ([v0.1.0])
* 4단계 감사 프레임워크 및 Phase별 검증 마일스톤 `audit_roadmap.md` 생성 ([v0.1.0])
* CMakeLists.txt 스캐폴딩 및 빌드 파이프라인 수립 ([v0.1.0])
* Game, GameState, GameStateManager FSM 기동 소스 구현 ([v0.1.0])
* TitleState (ASCII 메인 로고 및 점멸 안내), TownState (마을 임시 상태) 구현 ([v0.1.0])
* TestHarness (단위 테스트 프레임워크) 및 main.cpp 진입점 소스 구현 ([v0.1.0])

### Fixed (수정됨)
* C++ 정수 나눗셈의 0 방향 내림(truncation towards zero)으로 인해 D&D 능력치 음수 보정치 계산이 어긋나는 버그 수정 ([v0.1.0])
  * `(score - 10) / 2` 공식을 `int diff = score - 10; (diff < 0) ? (diff - 1) / 2 : diff / 2;` 로 정정. `spec.md` 및 `test_harness.cpp` 동시 반영 완료.

---

* [v0.1.0]: 초기 버전 문서 세트(D3D Protocol 기준 5개 코어 스펙 문서) 동결 완수.
