# Crawlmaster 상용 Wizardry 스타일 게임 멀티 감사 보고서

## 1. Audit Metadata

- Audit Turn: 1
- Audit Date: 2026-09-03 (Asia/Seoul)
- Project Root: /mnt/Projects_SSD/cpp/crawlmaster
- Project: Crawlmaster 0.9.4, C++20 / SFML 2.6.x desktop game
- User Goal: Wizardry 스타일 게임으로서 인터페이스, 게임 요소, 콘텐츠가 상용 수준인지 감사하고 부족한 부분의 개선 방법을 확인
- Audit Mode: Standard-backed
- Primary Standard: /mnt/Projects_SSD/cpp/crawlmaster/AI_AUDIT_DOC_STANDARD.md
- Multi-audit Contract: /home/eunho1/.codex/skills/multi-audit/references/report-contract.md
- Final Decision: **HOLD — 현재 트리는 정식 유료 출시 후보가 아니라 기능성 프로토타입/수직 슬라이스 단계**

## 2. User Goal and Decision Basis

이 감사에서 상용 수준은 AAA급 그래픽이나 특정 가격을 의미하지 않는다. 가격이 아직 정해지지 않은 소규모 유료 PC 게임이라도 다음 조건을 충족하는 신뢰 가능한 출시 후보를 뜻한다.

1. 시작부터 종결까지 완주 가능한 제품 목표와 진행 곡선이 있다.
2. 인터페이스가 실제 게임 상태를 정확히 보여주고, 조작·취소·오류·파괴적 행동을 예측 가능하게 처리한다.
3. 선언한 몬스터·아이템·퀘스트·스킬이 정상 플레이 경로에서 도달 가능하고 의미 있는 선택으로 소비된다.
4. 저장·복구·빌드·패키지·라이선스·플랫폼 검증이 고객 환경에서 재현 가능하다.
5. 문서·스토어 주장·실제 빌드가 같은 기능 범위를 설명한다.

레트로 와이어프레임 미학, 턴제 전투, 높은 난이도와 하드코어 TPK 자체는 결함으로 보지 않았다. 비교 기준으로 공식 Wizardry 제품 설명의 10층 미로, 최종 목표/보스, 파티 관리·탐색·주문·전투 QoL을 참고했으나 10층이나 동일 기능 수를 기계적으로 요구하지 않았다.

- Wizardry official product reference: https://wizardry.info/en/product/game/wizardry-proving-grounds-of-the-mad-overlord/
- Xbox Accessibility Guidelines: https://learn.microsoft.com/en-us/xbox/accessibility/guidelines
- Steamworks release process: https://partner.steamgames.com/doc/store/releasing
- SFML license: https://www.sfml-dev.org/license/

## 3. Scope and Exclusions

### Included

- 권위/제품 문서: spec.md, designs.md, README.md, CHANGELOG.md, BUILD_GUIDE.md, IMPLEMENTATION_SUMMARY.md, DESIGN_DECISIONS.md, LESSONS_LEARNED.md, audit_roadmap.md
- 구현: 전체 include/, src/, CMakeLists.txt
- 검증: src/test_harness.cpp, Debug/Release 빌드, CTest, Linux Xvfb startup
- 자산/데이터: 5개 언어 JSON, 3개 번들 폰트, save.json
- 신뢰 경계: save/config 경로, 손상 복구, TPK reset, 파일 쓰기, 의존성 및 자산 provenance
- 상용 표면: 설치/패키징, 런타임 재배치, CI/CD, 지원 플랫폼, legal/support identity

### Excluded or Unverified

- Windows/MSVC와 macOS 실기 빌드·실행
- 실제 Steam App ID, depot 업로드, 심사와 가격 설정
- 장시간 완주 및 정량 밸런스 플레이
- 5개 언어 전체 화면의 실기 캡처·CJK raster 판독·고DPI 검증
- build/_deps 아래 제3자 구현 자체의 심층 보안 감사
- 폰트/EULA의 최종 법률 판단
- Git 계보: 현재 workspace는 Git 저장소로 인식되지 않음

이 미검증 범위는 PASS로 간주하지 않는다. 다만 현재 코드와 직접 실행만으로 이미 상용 출시를 차단하는 Critical/Major 문제가 확정되어 최종 판정은 INCONCLUSIVE가 아니라 HOLD다.

## 4. Work-Surface Inventory

| Surface | Observed |
| --- | --- |
| Product docs | 핵심 D3D 문서와 9개 이전 감사 계보 존재 |
| Source | C++ 소스 20개, 헤더 24개 |
| Tests | 단일 TestHarness, 14개 test 함수, 132개 assert; CTest 등록 0개 |
| Dungeon | 20x20 DungeonMap 1개, 입장마다 새 DFS 미로 생성 |
| Classes | Warrior, Mage, Rogue, Cleric 4개 |
| Monsters | 8종, 진행도와 무관한 전역 스폰 풀 |
| Skills/spells | 12종, 직업별 레벨 1~3 자동 습득 |
| Items | canonical 19종; 상점 구매 8종; 비상점 11종 중 Mage 시작 장비 2종을 제외한 9종은 정상 획득 경로 없음 |
| Quests | spec 3종, Castle/UI/runtime 연결 2종 |
| Localization | KO/EN/JA/ZH-TW/ZH-CN 각 141 keys, 그러나 다수 사용자 문자열은 코드에 직접 하드코딩 |
| Assets | 번들 TTF 3개와 언어 JSON만 존재; 제품 아이콘/스크린샷/오디오/notice bundle 없음 |
| Build | Debug 전체 빌드/하네스 성공, Release 전체 빌드 실패 |
| Runtime | Xvfb에서 1024x768 창 기동 확인; 실제 상태별 GUI 판독은 미검증 |
| Packaging/CI | first-party install/package 규칙, CMake preset, CI workflow, Windows artifact, SBOM/signing 없음 |
| Trust boundary | CWD 기반 save/config, direct truncate write, backup/atomic rename 없음 |

초기 제품 콘텐츠 해시(빌드 및 docs/multi_audit 제외):

```text
6496943024b1df2b476e3deae44f39474651e97832e66796de72133d6d16d4ab
```

## 5. Agent Allocation and Rationale

6개 에이전트를 사용했다. 인터페이스, 게임 시스템, 콘텐츠는 서로 다른 제품 질문이며, 저장/파괴적 동작과 상용 배포는 두 독립 보고서에서 이중 검증해야 했기 때문이다.

| Agent | Perspective | Core Question |
| --- | --- | --- |
| A01 | Product contract | 출시 범위·완료 주장·문서와 구현이 같은 제품을 설명하는가 |
| A02 | Interface/UX | 실제 상태·조작·접근성·i18n이 상용 UI로 신뢰 가능한가 |
| A03 | Game systems | 전투·파티·성장·경제·저장이 의미 있는 선택과 규칙을 구현하는가 |
| A04 | Content depth | 던전·퀘스트·보상·종결 아크와 재플레이 변주가 충분한가 |
| A05 | Engineering/runtime | Release 빌드·테스트·FSM·저장·RNG가 재현 가능하고 안전한가 |
| A06 | Commercial release | 패키징·플랫폼·자산 권리·공급망·운영 gate가 준비됐는가 |

## 6. Immutable Source Report Manifest

- Manifest: /mnt/Projects_SSD/cpp/crawlmaster/docs/multi_audit/1/source_report_manifest.json
- Manifest SHA-256: 508933a51f84c6fbaa773888a40f3b273f1eeca65283a08bfafc878f27e6e1ac
- Sidecar: /mnt/Projects_SSD/cpp/crawlmaster/docs/multi_audit/1/source_report_manifest.sha256.json
- Sidecar SHA-256: 057eb72e8744f5dd549f76dc0e893d50f6764c219d56a2c584c2e5dbd9db93bf
- missing_source_reports: []

| Report | SHA-256 | Completion |
| --- | --- | --- |
| sub_audit_01_product_contract.md | 01bc811fbecde81177e3611306451fe7b252973f13cb226ca5e5e285ecbb0170 | Complete / Sealed |
| sub_audit_02_interface_ux.md | 33c1140380416fc4f9aea7864ec5d04f1a6922cc3397630e915c6fe40be36129 | Complete / Sealed |
| sub_audit_03_game_systems.md | 34e696b1f6c7fcde94682a68a2dabf05fffb02f17cd70d1b1328dc839c20c507 | Complete / Sealed |
| sub_audit_04_content_depth.md | 4554d1205bd6b20f712fbd76630efc93929ed4f368fdf26a54713b97487b3d09 | Complete / Sealed |
| sub_audit_05_engineering_runtime.md | 2c011a22c6d4bb927ff76613cdccfcbdcfebdc766cd45f5721f478935a6aafa3 | Complete / Sealed |
| sub_audit_06_commercial_release.md | 5b4f7c6f6bb3ba1fba2a94c6273febabe28671055ed1fcba2968c9dea9c2ffb7 | Complete / Sealed |

## 7. Evidence and Commands

주요 직접 실행 결과:

```text
cmake -S . -B build/audit-commercial -DCMAKE_BUILD_TYPE=Debug
cmake --build build/audit-commercial -j2
./build/audit-commercial/TestHarness
-> Debug 전체 빌드 성공, 14개 test 함수 출력 성공

ctest --test-dir build/audit-commercial -N
-> Total Tests: 0

cmake -S . -B build/audit-commercial -DCMAKE_BUILD_TYPE=Release
cmake --build build/audit-commercial -j2
-> Crawlmaster target는 링크
-> TestHarness는 NDEBUG로 assert가 제거된 뒤 unused-variable가 -Werror가 되어 exit 2

timeout 8s xvfb-run -a ...
-> Crawlmaster window 1024x768 기동 확인

readelf -d build/audit-commercial/Crawlmaster
-> developer build tree의 절대 RUNPATH와 libsfml-*.so.2.6 의존성 확인

rg 기반 reachability 검사
-> 전투/던전 보상에서 Party::addItem 호출 없음
-> qst_hunt_spiders는 spec 외 제품 호출 경로 없음
-> TileType::DOOR는 선언만 있고 맵 생성 대입 없음
```

## 8. Coverage Gap Check

| Work Surface / Audit Question | Assigned Agents | Evidence | Coverage | Follow-up |
| --- | --- | --- | --- | --- |
| 제품 목표·상용 출시 범위 | A01, A04, A06 | spec/README/roadmap/source | Covered | FIN-F001 |
| 인터페이스 상태 정확성·조작 | A02, A01, A06 | controller/renderer/input/runtime startup | Covered | FIN-F007~F009 |
| 게임 시스템·전투·경제 | A03, A01, A04 | model/controller/spec/tests | Covered | FIN-F004~F006 |
| 콘텐츠 도달성·던전·완결성 | A04, A03, A01 | factory/call path/FSM/content counts | Covered | FIN-F002~F003 |
| 저장·복구·TPK 데이터 무결성 | A05, A06, A01/A03 | file path/write/load/FSM/test output | Covered, independently twice | FIN-F010~F012 |
| 빌드·테스트·런타임 | A05, A06, A01 | Debug/Release/CMake/CTest/Xvfb | Covered | FIN-F013 |
| 패키징·CI·플랫폼 | A06, A05 | CMake/readelf/inventory | Covered for current Linux tree | FIN-F014 |
| 의존성·폰트 권리·법적 고지 | A06, A01, A05 | CMake/font metadata/root inventory | Covered for evidence absence | FIN-F015 |
| 전체 5-locale 실제 raster·고DPI | A02, A06 | source-derived bounds/contrast, Xvfb startup only | Partially Covered | HOLD 유지; Linux/Windows 실화면 재감사 |
| 장시간 완주·밸런스·리텐션 | A03, A04 | static/runtime model evidence only | Not Covered | 현재 완주 경로 자체가 없어 선행 구현 후 측정 |
| Windows/macOS 실제 패키지 | A05, A06 | 현재 환경 증거 없음 | Not Covered | 지원 플랫폼 확정 후 hosted/실기 gate |

사용자 핵심 목표는 실제 파일·명령 증거로 모두 조사했다. 미해소 플랫폼/장시간 공백은 상용 PASS를 차단하지만, 이미 확정된 차단 finding을 무효화하지 않는다.

## 9. Canonical Findings

### [FIN-F001] 상용 제품 범위와 출시 acceptance가 닫혀 있지 않음

- Sources: A01-F001, A04-F009, A06-F006, A06-F007
- Areas: Product contract, release lane, target platform, audio scope
- Severity: Major
- Status: Needs Spec Clarification
- Summary: 현재 문서는 완성형/포스트런치 표현을 사용하지만 데모·Early Access·정식 출시 중 어느 lane인지, 대상 플레이어·지원 OS·완주 길이·가격 TBD 정책·오디오 약속·상용 acceptance가 없다.
- Verified Evidence: spec.md는 장르/기술/기능을 정의하지만 release lane과 완주 기준이 없고, audit_roadmap.md는 아직 없는 다층 던전과 오디오를 Post-Launch로 부른다. 사운드는 비목표인데 Settings는 BGM/SFX 값을 기능처럼 노출한다.
- Expected Basis: 상용 수준을 판정하려면 출시 약속과 미완료 범위를 구분해야 한다.
- Actual: 기술 Phase 완료와 고객 출시 완료가 문서에서 혼용된다.
- Impact: scope creep와 과대 홍보가 반복되고 어떤 Major gap이 defer 가능한지 판정할 수 없다.
- Required Action: spec.md에 Demo/EA/1.0 lane, 대상 사용자, 지원 OS, 완주 정의, 목표 플레이 시간, 가격 TBD, 오디오/접근성/패키지 gate를 먼저 확정하고 README/roadmap을 동기화한다.
- Re-audit Method: 모든 README/store-facing 기능을 domain/UI/package/test evidence에 1:1 매핑한다.
- Synthesis Rationale: 수량을 창작하지 않고, 제품 계약 부재 자체만 유지했다.

### [FIN-F002] 단일 재생성 미로에는 진행 아크·최종 목표·엔딩이 없음

- Sources: A01-F002, A01-F005, A03-F002, A03-F016, A04-F001, A04-F006, A05-F008
- Areas: Dungeon progression, campaign completion, persistence
- Severity: Major
- Status: Confirmed
- Summary: DungeonState는 생성될 때마다 새 20x20 미로 하나를 만들고, 시작 계단 복귀와 일반 전투만 제공한다. 층/깊이/목표/보스/엔딩 상태가 없다.
- Verified Evidence: src/controller/DungeonState.cpp:13-20, src/model/DungeonMap.cpp:24-50, include/controller/DungeonState.hpp:27-29. spec의 FSM에는 존재하지 않는 GameOverState만 있고 campaign-complete state가 없다.
- Expected Basis: 특정 층 수가 아니라 최소 하나의 완주 가능한 목표와 결과가 필요하다.
- Actual: 전투 승리는 같은 DungeonState로 돌아가며, 귀환하면 맵이 파괴되고 다음 입장에서 새 맵이 생성된다.
- Impact: 구매자가 게임을 완료할 수 없고 시작-중반-종반 pacing을 구성할 수 없다.
- Required Action: 먼저 짧은 한 구간 수직 슬라이스를 entry → landmark/event → reward → boss/final encounter → result/ending → save/reload로 완결한다. 이후 층 수를 늘린다.
- Re-audit Method: clean save로 시작해 종결 상태까지 자동/수동 완주하고 완료 상태의 중복 보상과 재로드를 확인한다.
- Synthesis Rationale: 던전 저장 문제는 FIN-F010에도 연결되지만, 여기서는 제품 완결성에 초점을 맞췄다.

### [FIN-F003] 선언 콘텐츠가 실제 탐험·드롭·퀘스트 경로에 연결되지 않음

- Sources: A01-F006, A02-F013, A03-F003~F005, A04-F002~F004
- Areas: Loot, dungeon events, quest reachability, canonical content registry
- Severity: Major
- Status: Confirmed
- Summary: 아이템은 19종이 정의됐지만 상점 8종과 Mage 시작 장비 2종 외 비상점 아이템 9종의 정상 획득 경로가 없다. 전투 보상은 XP/gold만 주며, Door는 생성되지 않고 trap/event/secret/treasure가 없다. spec 퀘스트 3종 중 2종만 런타임에 있다.
- Verified Evidence: CombatState::distributeRewards에는 addItem이 없고, DungeonState에도 pickup이 없다. qst_hunt_spiders는 spec 외 검색 결과가 없으며, TileType::DOOR 대입은 맵 생성에 없다. getShopCatalog의 19종 helper는 호출되지 않는다.
- Expected Basis: 정의된 콘텐츠가 shop/starter/dungeon/quest 중 명시적 획득원과 소비 경로를 가져야 한다.
- Actual: factory 데이터와 플레이 가능 콘텐츠가 분리됐다. 메이스 수집 퀘스트는 상점 구매로 즉시 우회되고 완료 이력이 없어 반복 보상도 가능하다.
- Impact: 던전 탐색 동기와 보상 발견이 없고 콘텐츠 볼륨 주장이 실제 경험보다 크다.
- Required Action: canonical content registry에 acquisition source, drop table, quest availability/completion policy를 둔다. 수직 슬라이스에 최소 목표 지점·위험/비밀·보장 보상을 연결하고 거미 퀘스트를 구현하거나 Deferred로 문서화한다.
- Re-audit Method: clean save에서 각 19 item과 3 quest의 도달성 매트릭스를 실행해 획득→사용/장착→판매→저장까지 확인한다.
- Synthesis Rationale: 일부 원본의 아이템 18종 표기는 직접 재계수 결과 19종으로 정정했다. reachability 결론은 유지된다.

### [FIN-F004] 파티 구성과 전투 지원 행동이 플레이어 선택을 소비하지 않음

- Sources: A02-F005, A02-F010, A03-F001, A03-F011
- Areas: Character creation, combat item/target selection, party strategy
- Severity: Major
- Status: Confirmed
- Summary: 길드가 이름과 직업을 자동 부여하고, Combat Item은 숨은 우선순위로 아이템/대상을 골라 즉시 소모한다. Cure Wounds도 명시 대상 대신 자동 대상을 선택한다.
- Verified Evidence: TownState.cpp:77-101의 rand() % 4와 RANDOM_NAMES, CombatState.cpp:382-535의 auto-selection/removeItem, Skill.cpp의 CureWounds targetIdx 미사용.
- Expected Basis: Wizardry 스타일 파티 게임의 핵심 가치는 역할 구성과 제한 자원 사용의 의사결정이다. 랜덤 모집을 선택해도 preview/reroll/확인 계약이 필요하다.
- Actual: 플레이어는 원하는 파티/대상/아이템을 결정할 수 없고 no-op에도 자원을 잃을 수 있다.
- Impact: 전략 깊이와 조작 신뢰가 함께 낮아진다.
- Required Action: 길드에 preview·confirm·reroll 또는 직접 생성 정책을 명시하고, 전투에는 item list → target → preview → confirm/cancel 상태를 추가한다. 효과 실패는 행동/아이템을 소비하지 않는다.
- Re-audit Method: 0~4인 파티와 여러 아이템/부상자 fixture로 선택·취소·실패·소모를 event transcript로 검증한다.
- Synthesis Rationale: 랜덤 모집 자체는 Needs Clarification일 수 있으나 현재 UI의 선택/설명 부재와 자동 소비는 확정 결함이다.

### [FIN-F005] 전투·장비·보상 공식의 핵심 규칙이 문서와 실제 계산에서 분리됨

- Sources: A01-F007, A03-F008~F010, A03-F012~F013, A04-F005, A06-F013
- Areas: Combat math, status effects, equipment invariants, economy
- Severity: Major
- Status: Confirmed
- Summary: 2d6 무기가 1d6처럼 계산되고, Bless +2 명중은 소비되지 않으며, Skeleton 피해 저항과 적 자연 1/20 계약이 없다. Plate/Tower STR 제한은 model/deserialization에서 우회된다. 골드 보상도 동결 공식과 다르다.
- Verified Evidence: getDamageDiceCount 호출 부재, getBlessTurns가 전투 명중에서 읽히지 않음, damage type API 부재, Character::equip의 요구조건 미검사, CombatState.cpp:710-713의 5..15 × foe count.
- Expected Basis: spec.md의 명시적 전투/아이템/보상 계약과 데이터 불변조건.
- Actual: 화면에 보이는 장비·주문·몬스터 특성이 결과에 반영되지 않거나 다른 수식으로 처리된다.
- Impact: 빌드 다양성, 적 정체성, 난이도와 경제 tuning이 신뢰 불가능하다.
- Required Action: combat resolution과 equipment validation을 단일 domain service로 통합하고 damage dice count/type, bless, natural roll, requirement, reward ledger를 seedable test로 잠근다. 수식을 바꾸려면 문서를 먼저 갱신한다.
- Re-audit Method: 각 무기/스킬/적/STR 경계/보상 fixture를 고정 seed로 반복하고 기대 ledger와 비교한다.
- Synthesis Rationale: 같은 근본 원인인 rule data와 실제 consumer의 단절을 하나로 병합했다.

### [FIN-F006] 난이도 곡선·성장 소비처·재현 가능한 RNG가 없음

- Sources: A03-F007, A03-F014, A04-F005, A04-F008, A05-F010
- Areas: Difficulty, progression, replayability, determinism
- Severity: Major
- Status: Confirmed with product thresholds needing clarification
- Summary: 모든 깊이에서 같은 8종 풀과 1~3마리를 사용하고 Dragon Whelp도 첫 전투부터 3%로 등장할 수 있다. 레벨 3 이후 목표/성장 소비처가 없고 RNG가 호출마다 random_device로 분산되며 길드만 C rand를 쓴다.
- Verified Evidence: MonsterFactory의 단일 전역 확률표, CombatState::spawnMonsters 1~3, Dungeon/Combat/Character/Skill/Monster의 다수 random_device, seed injection/record API 부재.
- Expected Basis: 높은 난이도 자체가 아니라 의도된 위험/보상 곡선과 QA 재현성이 필요하다.
- Actual: 초반 random spike와 후반 단조로움을 측정하거나 고객 신고를 replay할 수 없다.
- Impact: 밸런스·회귀·지원 비용이 증가하고 재플레이 변주가 운에만 의존한다.
- Required Action: session-owned seedable RNG stream, depth/party-level encounter budget, 보상/회복/성장 목표치를 문서화한다. 정확한 수치는 플레이테스트로 결정한다.
- Re-audit Method: depth/party level별 수백 seed simulation과 실제 플레이로 win/TPK rate, encounter mix, 회복비, level time을 측정한다.
- Synthesis Rationale: 난이도 목표 수치는 미확정이지만 곡선과 재현 surface 부재는 확정이다.

### [FIN-F007] New Game/Continue와 파괴적 행동 UX가 사용자 의도와 불일치

- Sources: A01-F003, A02-F009~F010, A05-F009
- Areas: Title flow, onboarding, destructive actions, TPK
- Severity: Major
- Status: Confirmed
- Summary: Title은 New Game만 보여주지만 Town 진입 시 기존 save를 자동 로드한다. Continue locale key는 사용되지 않는다. 판매·해고·소모·TPK reset에 review/confirm/undo가 없고 TPK 로그는 상태 교체 전에만 쌓인다.
- Verified Evidence: TitleState는 New/Settings/Exit 3개, TownState constructor는 loadFromFile, 여러 mutation은 단일 숫자/Enter로 즉시 실행, Combat TPK는 reset 후 Title로 즉시 전환.
- Expected Basis: 신규 시작과 이어하기는 다른 의도이며 비가역 진행 삭제는 사전 고지가 필요하다.
- Actual: 새 게임/이어하기가 구분되지 않고 하드코어 규칙이 첫 던전 전에 설명·확인되지 않는다.
- Impact: 오입력과 진행 손실 위험이 크다.
- Required Action: Continue, New Game, save metadata/slot, reset confirm을 분리한다. 판매·해고·희귀 소모와 TPK 정책에 preview/cancel/복구 규칙을 추가한다.
- Re-audit Method: save 없음/정상/손상 상태와 모든 destructive action의 confirm/cancel 전후 bytes를 비교한다.
- Synthesis Rationale: TPK 규칙은 유지 가능하나 데이터 처리와 사용자 고지는 분리해야 한다.

### [FIN-F008] Dungeon HUD와 상태 피드백이 실제 게임 상태를 표시하지 않음

- Sources: A01-F008, A02-F001, A02-F008, A06-F014
- Areas: HUD truth, status effects, feedback
- Severity: Major
- Status: Confirmed
- Summary: DungeonRenderer는 실제 Party를 받지 않고 고정 4인/HP를 표시한다. 독·마비·버프와 중요한 저장/경고도 persistent status UI 없이 로그에만 남는다.
- Verified Evidence: DungeonRenderer::render 인자에는 Party가 없고 src/view/DungeonRenderer.cpp:253-263에 고정 WARRIOR/CLERIC/ROGUE/MAGE 문자열이 있다. CharacterInfo/Combat 패널에도 상태 턴 표시가 없다.
- Expected Basis: 핵심 의사결정 정보는 현재 domain state와 같아야 한다.
- Actual: 빈 파티나 손상된 파티도 같은 HUD를 보며 로그는 빠르게 사라진다.
- Impact: 회복·전투·도주 판단을 오도한다.
- Required Action: immutable PartyHUD snapshot을 renderer에 주입하고 0~4명, HP, 상태, slot을 동적으로 표시한다. 중요 오류/TPK/save failure는 modal/banner로 유지한다.
- Re-audit Method: 0/1/4명과 low HP/dead/poison/paralysis/buff fixture를 상태별로 렌더링하고 domain snapshot과 비교한다.
- Synthesis Rationale: 상태 진실성과 피드백 문제를 하나의 HUD 계약으로 병합했다.

### [FIN-F009] 조작·i18n·텍스트 접근성 계약이 상용 UI 수준에 미달

- Sources: A01-F009, A02-F002~F004, A02-F007, A02-F011, A04-F007, A06-F015
- Areas: Localization, navigation, accessibility, onboarding
- Severity: Major
- Status: Confirmed
- Summary: 5개 locale JSON은 있지만 Town/Dungeon/Combat/CharacterInfo와 item/monster/skill에 한국어·영어 literal이 남는다. 안내된 Enter/ESC/숫자 동작이 handler와 다르고 Settings O는 모든 상태에서 동작하지 않는다. 10~16px 중심 고정 텍스트, 저대비 토큰, scale/wrap/remap 부재도 확인됐다.
- Verified Evidence: 사용자-facing literal 검색, setCharacterSize 검색, A02의 sRGB 계산(#114411 대 #050B05 1.76:1), State별 key handler 직접 대조. Xvfb는 창 기동만 확인했다.
- Expected Basis: spec의 모든 UI text localization, 일관된 입력, Xbox Accessibility Guidelines의 text/contrast/navigation best practice.
- Actual: 지원 언어가 섞이고 일부 화면 안내 키는 무반응이다. 실제 5-locale raster/high-DPI는 미검증이다.
- Impact: 비한국어 사용자와 저시력/키보드 사용자의 진행을 막고 store language claim을 위반할 수 있다.
- Required Action: 모든 표시 문자열을 ID+placeholder catalog로 옮기고 공통 input/focus/back contract를 만든다. text scale, high contrast, wrap/scroll, focus 표시를 추가하고 상태별 5-locale UI test를 만든다.
- Re-audit Method: 5개 언어로 모든 State/substate/overlay를 순회해 raw literal/key, overflow, focus, advertised key, 대비를 자동·실기 검증한다.
- Synthesis Rationale: 개별 화면 문제는 하나의 UI 정책/검증 부재에서 반복된다.

### [FIN-F010] 세이브 스키마·초기값·던전 진행 계약이 서로 충돌

- Sources: A01-F004~F005, A03-F006, A03-F016, A05-F007~F008, A06-F011~F012
- Areas: Save schema, migration, defaults, run persistence
- Severity: Major
- Status: Confirmed; dungeon resume policy needs clarification
- Summary: spec은 camelCase/nested equipment와 pot_mana 초기 지급을 선언하지만 구현은 snake_case/flat eq_*와 heal 2개만 쓴다. README는 dungeon coordinates/progress 저장을 약속하지만 Party JSON에는 map/seed/position/FOW가 없다.
- Verified Evidence: spec.md:289-340, Character.cpp:384-469, Party.cpp:88-202, README.md:19/57, DungeonState constructor.
- Expected Basis: 하나의 versioned canonical schema와 명확한 run-resume 정책.
- Actual: spec-shaped save가 예외 reset으로 갈 수 있고, 재입장/재시작 시 던전 진행이 사라진다.
- Impact: migration·지원도구·cloud save와 신규 경제가 신뢰 불가능하다.
- Required Action: schema_version과 migration/validation을 추가하고 필드명·4인 제한·범위·초기 inventory를 통일한다. dungeon run을 저장할지 town-only로 할지 결정해 문서와 코드를 맞춘다.
- Re-audit Method: spec fixture/current fixture/구버전/invalid data의 round-trip과 던전 재개 시나리오를 검증한다.
- Synthesis Rationale: 원자성은 FIN-F011, schema 의미는 본 finding으로 분리했다.

### [FIN-F011] CWD 기반 비원자 저장과 손상 reset이 고객 진행을 파괴할 수 있음

- Sources: A01-F012, A02-F006, A03-F015, A05-F005~F006, A06-F008~F010
- Areas: Data integrity, save/config, corruption recovery, mutation persistence
- Severity: **Critical**
- Status: Confirmed
- Summary: save/config는 process CWD의 파일을 직접 truncate해 쓰고 stream 최종 상태, temp/atomic rename, backup이 없다. load 오류는 corrupt save를 격리하지 않고 resetToDefault를 호출해 진행을 지운 뒤 true를 반환한다. custom path 복구도 기본 path에 쓸 수 있다. CharacterInfo와 Combat item mutation은 저장되지 않는다.
- Verified Evidence: Party.cpp:88-202, LocalizationManager.cpp:129-135, default ./save.json/./config.json, CharacterInfoState mutation paths, Combat item remove path. Debug harness의 corrupt custom file 실행에서도 다른 default save path 쓰기가 관찰됐다.
- Expected Basis: AI_AUDIT 표준의 데이터 손상 Critical 기준과 상용 RPG 진행 보존 불변조건.
- Actual: crash/disk full/permission/partial JSON이 마지막 정상 진행의 비가역 손실로 이어질 수 있고 save failure가 UI에 전달되지 않는다.
- Impact: 캐릭터·장비·퀘스트·설정 영구 손실. PASS 불가.
- Required Action: OS별 per-user data directory, same-directory temp write → flush/fsync → atomic replace, rotating backup, schema validation/quarantine, typed Load/SaveResult와 UI 재시도를 도입한다. TPK reset은 corruption recovery와 다른 명령으로 분리한다. 모든 mutation은 transaction 또는 명시적 dirty/flush 정책을 사용한다.
- Re-audit Method: read-only, disk-full simulation, truncated/type-invalid save, custom path, kill-during-write, TPK를 각각 실행해 원본/backup/오류 UI/재시작 결과를 확인한다.
- Synthesis Rationale: A06의 두 Critical을 하나의 근본적인 durability/recovery 경계로 병합하고 Critical을 유지했다.

### [FIN-F012] TPK 상태 전이가 stale DungeonState를 남김

- Sources: A05-F009, A02-F009
- Areas: FSM lifetime, GameOver, TPK recovery
- Severity: Major
- Status: Confirmed
- Summary: [Dungeon, Combat] stack에서 Combat의 changeState(Title)는 top Combat만 제거해 [Dungeon, Title]을 만든다. 문서의 GameOverState는 구현되지 않았다.
- Verified Evidence: GameStateManager.cpp:9-16은 top 1개만 pop, DungeonState는 Combat을 push, CombatState TPK는 Title로 change.
- Expected Basis: TPK 후 이전 run state가 남지 않고 새 session root가 명확해야 한다.
- Actual: 이전 맵/로그/auto-path를 가진 DungeonState가 아래에 남아 후속 전환마다 stack을 오염시킨다.
- Impact: 하드코어 reset 후 stale 진행 재노출·메모리 증가·예측 불가능 전이가 가능하다.
- Required Action: clear-and-replace root transition을 추가하고 GameOver 화면을 구현하거나 spec에서 제거한다. 저장 성공/실패를 확인한 뒤 상태를 전환한다.
- Re-audit Method: fake state stack과 실제 TPK에서 stack depth/type 및 New/Exit/재입장 결과를 확인한다.
- Synthesis Rationale: 데이터 삭제는 FIN-F011, FSM lifetime은 별도 유지했다.

### [FIN-F013] Release 빌드와 자동 테스트 gate가 유효하지 않음

- Sources: A01-F011, A02-F011, A03-F017, A05-F001~F002, A05-F004, A06-F002~F003
- Areas: Release build, CTest, production path, GUI smoke
- Severity: Major
- Status: Confirmed
- Summary: Debug 하네스는 통과하지만 Release 전체 빌드는 assert 제거와 -Werror로 실패한다. CTest에는 0 tests이며 하네스는 Game/controller/renderer를 링크하지 않아 실제 루프를 검증하지 않는다.
- Verified Evidence: Release build exit 2의 unused-variable 목록, ctest -N Total Tests: 0, CMake TEST_SOURCES와 production SOURCES 대조.
- Expected Basis: Release에서도 작동하는 assertions와 실제 제품 경로의 결정적 회귀 증거.
- Actual: green Debug 로그가 HUD, FSM, 저장 timing, loot, input과 패키지 실행을 보장하지 않는다.
- Impact: 현재 완료 주장과 상용 QA가 증거로 재현되지 않는다.
- Required Action: Release-safe CHECK/EXPECT, CTest 등록, shared production core, seedable headless campaign test, Xvfb/Windows GUI input transcript를 도입한다.
- Re-audit Method: clean Debug/Release에서 모든 target과 CTest가 성공하고 의도적 failure가 non-zero인지 확인한다.
- Synthesis Rationale: 테스트의 존재가 아니라 release-validity와 product-path coverage를 기준으로 병합했다.

### [FIN-F014] 재배치 가능한 제품 패키지와 cross-platform release pipeline이 없음

- Sources: A01-F013, A05-F003, A06-F001, A06-F006, A06-F017
- Areas: Packaging, runtime paths, CI/CD, supported platforms
- Severity: Major
- Status: Confirmed
- Summary: first-party install/package 규칙이 없고 Release binary는 개발자 build tree의 SFML 절대 RUNPATH를 가진다. assets/save/config도 cwd 기반이며 Windows/Linux artifact CI가 없다.
- Verified Evidence: CMakeLists에 install/제품 CPack/CTest/CI 없음, readelf RUNPATH=/home/eunho1/.../build/.../_deps/sfml-build/lib, root CI/preset inventory 없음.
- Expected Basis: 고객 PC에서 독립 실행 가능한 versioned package와 동일 source commit의 플랫폼 gate.
- Actual: build cache를 지우거나 다른 cwd/PC로 옮기면 라이브러리·자산·세이브 경로가 달라질 수 있다.
- Impact: 골드 마스터를 만들고 재현·지원·rollback할 수 없다.
- Required Action: executable-relative resources, per-user data, install(TARGETS/DIRECTORY), Linux $ORIGIN/AppImage/Steam Runtime 또는 동등 전략, Windows DLL 배치, Linux+MSVC CI package smoke와 checksum/signature를 구축한다.
- Re-audit Method: clean hosted builders에서 package를 만들고 임의 cwd/새 사용자 환경에서 설치·실행·삭제·업데이트를 검증한다.
- Synthesis Rationale: 배포 채널 자체는 미확정이나 현재 패키지 부재는 확정이다.

### [FIN-F015] 의존성·폰트 provenance와 legal/support identity가 출시 gate를 통과하지 못함

- Sources: A01-F014, A05-F011, A06-F004~F005, A06-F016
- Areas: Supply chain, font rights, notices, product identity
- Severity: Major
- Status: Confirmed for evidence absence; Human Review Required for rights
- Summary: SFML/json tag는 있지만 immutable lock/SBOM/scanner가 없고 system SFML 2.6.x를 우선 선택한다. 번들 TTF 3개의 source/version/license/redistribution record와 제품 LICENSE/THIRD_PARTY_NOTICES가 없다. PerfectDOSVGA437.ttf의 내부 family는 Ubuntu Mono로 보여 filename과 provenance도 불일치한다.
- Verified Evidence: CMake find_package/GIT_TAG, root legal-file inventory, fc-scan/file/hash 결과, title의 고정 DEEPMIND copyright 문구.
- Expected Basis: shipped binary/asset의 재현 가능한 출처와 사람 승인 권리표.
- Actual: SFML의 상업 사용 가능성은 공식 문서로 확인했지만 그것이 폰트 권리를 증명하지 않는다.
- Impact: 유료 재배포 권리, 고객 고지, support/version 식별을 입증할 수 없다.
- Required Action: immutable dependency manifest/SBOM/license scan과 각 TTF의 원출처·checksum·license·변환 조건을 확보한다. 불명확 폰트는 명확한 상업 재배포 라이선스로 교체하고 legal owner가 LICENSE/EULA/privacy/support/credits를 승인한다.
- Re-audit Method: package의 SBOM/notices/checksums와 source artifact를 대조하고 Human Review sign-off를 기록한다.
- Synthesis Rationale: 법률 결론은 내리지 않고 증거 부재와 Human Review gate만 확정했다.

### [FIN-F016] 완료·구조 문서가 현재 구현을 과대 설명함

- Sources: A01-F010, A02-F012, A06-F018
- Areas: Documentation authority, implementation ownership
- Severity: Major
- Status: Confirmed
- Summary: audit_roadmap/IMPLEMENTATION_SUMMARY가 100%/통과/포스트런치 표현을 사용하고, 존재하지 않는 TownRenderer/CombatRenderer/UIRenderer를 현재 책임 파일처럼 적는다.
- Verified Evidence: 실제 view tree에는 DungeonRenderer만 있고 Town/Combat/CharacterInfo/Settings가 controller에서 직접 draw한다. Release/UI/content findings는 완료 문구와 충돌한다.
- Expected Basis: 보조 문서는 현재 코드보다 강한 완료 권위를 주장하지 않아야 한다.
- Actual: 신규 코더와 QA가 잘못된 file ownership과 출시 상태를 믿게 된다.
- Impact: 같은 drift와 누락된 검증이 반복된다.
- Required Action: 제품 계약을 먼저 확정한 뒤 완료/Deferred/Unverified를 다시 분류하고 실제 source/test/package evidence 링크로 책임표를 복구한다.
- Re-audit Method: 문서의 모든 파일 링크와 완료 항목을 rg --files, call graph, test/package artifact와 대조한다.
- Synthesis Rationale: 단순 링크 오류는 Minor지만 상용 완료 과대주장과 결합되어 Major로 유지했다.

## 10. Critical/Major Direct Re-verification

| Canonical Finding | Directly Checked By Main | Evidence Re-opened or Command Re-run | Result | Gate Impact |
| --- | --- | --- | --- | --- |
| FIN-F001 | Yes | spec/README/roadmap release·audio·완주 검색 | Confirmed | PASS blocked |
| FIN-F002 | Yes | DungeonState/DungeonMap/FSM, boss/ending search | Confirmed | PASS blocked |
| FIN-F003 | Yes | addItem, qst_hunt_spiders, DOOR, catalog call search | Confirmed; item count corrected to 19 | PASS blocked |
| FIN-F004 | Yes | Town rand, Combat item, CureWounds call path | Confirmed | PASS blocked |
| FIN-F005 | Yes | dice count, bless, resistance, equip, reward formula | Confirmed | PASS blocked |
| FIN-F006 | Yes | random_device/mt19937/rand, spawn pool, level path | Confirmed | PASS blocked |
| FIN-F007 | Yes | Title/Town load, destructive mutation, TPK path | Confirmed | PASS blocked |
| FIN-F008 | Yes | DungeonRenderer fixed HUD and status draw path | Confirmed | PASS blocked |
| FIN-F009 | Yes | literals, key handlers, text sizes, Xvfb window | Confirmed; raster remains partial | PASS blocked |
| FIN-F010 | Yes | spec/README vs Party/Character serializer | Confirmed | PASS blocked |
| FIN-F011 | Yes | save/config write/load paths and Debug corruption output | Confirmed Critical | PASS blocked |
| FIN-F012 | Yes | GameStateManager stack transitions | Confirmed | PASS blocked |
| FIN-F013 | Yes | clean Release rebuild exit 2, ctest -N = 0 | Confirmed | PASS blocked |
| FIN-F014 | Yes | CMake install/package search, readelf RUNPATH, CI inventory | Confirmed | PASS blocked |
| FIN-F015 | Yes | dependency declarations, font metadata/hash, legal files | Confirmed evidence gap | PASS blocked |
| FIN-F016 | Yes | view file inventory vs docs | Confirmed | PASS blocked |

모든 Critical/Major canonical finding을 메인 모델이 직접 재검증했다. 미재검증 Major는 없다.

## 11. Cross-Report Conflicts

1. **아이템 수 18 vs 19:** A01/A03 일부 문구는 18개라고 했고 A04/A06은 19개라고 했다. spec 표와 ItemFactory를 직접 재계수해 19개로 판정했다. 도달성 finding은 유지한다.
2. **Debug 녹색 vs Release 실패:** Debug TestHarness 성공은 사실이나 Release 전체 빌드는 실패하고 CTest는 0개다. 모델 테스트 성공을 제품/Release PASS로 승격하지 않는다.
3. **하드코어 TPK 의도 vs 데이터 안전:** permadeath 자체는 승인된 설계다. 그러나 corruption recovery가 같은 reset 함수로 합쳐지고 confirm/backup/save-result가 없는 것은 별도 Critical 결함이다.
4. **레트로 고정 UI vs 접근성:** 1024x768 와이어프레임 미학은 유지 가능하다. 낮은 대비, 실제 상태와 다른 HUD, tiny text, 불일치 입력은 미학이 아니라 기능/접근성 문제다.
5. **오디오 비목표 vs Settings 노출:** 오디오 미구현은 그 자체로 결함이 아닐 수 있다. 하지만 무효한 BGM/SFX control을 shipped feature처럼 노출한 문서/UI drift는 출시 scope 결정이 필요하다.

## 12. Finding Adjudication Ledger

| Source Findings | Decision | Canonical Finding | Rationale |
| --- | --- | --- | --- |
| A01-F001 | Accepted | FIN-F001 | Product scope gap |
| A01-F002 | Merged | FIN-F002 | Campaign completion |
| A01-F003 | Merged | FIN-F007 | New/Continue semantics |
| A01-F004~F005 | Merged | FIN-F010 | Save schema/run contract |
| A01-F006 | Merged | FIN-F003 | Content reachability |
| A01-F007 | Merged | FIN-F005 | Reward formula |
| A01-F008 | Merged | FIN-F008 | HUD truth |
| A01-F009 | Merged | FIN-F009 | i18n/UI |
| A01-F010 | Merged | FIN-F016 | Completion docs |
| A01-F011 | Merged | FIN-F013 | Product-path tests |
| A01-F012 | Merged | FIN-F011 | Durability |
| A01-F013 | Merged | FIN-F014 | Packaging |
| A01-F014 | Merged | FIN-F015 | Font rights |
| A02-F001, F008 | Merged | FIN-F008 | HUD/status feedback |
| A02-F002~F004, F007, F011 | Merged | FIN-F009 | Localization/navigation/accessibility |
| A02-F005, F010 | Merged | FIN-F004 / FIN-F007 | Combat choice and onboarding |
| A02-F006, F009 | Merged | FIN-F011 / FIN-F007 | Persistence and destructive UX |
| A02-F012 | Merged | FIN-F016 | UI ownership docs |
| A02-F013 | Merged | FIN-F003 | Missing quest |
| A03-F001, F011 | Merged | FIN-F004 | Player agency |
| A03-F002, F016 | Merged | FIN-F002 / FIN-F010 | Dungeon arc/run save |
| A03-F003~F005 | Merged | FIN-F003 | Loot/quest loop |
| A03-F006 | Merged | FIN-F010 | Schema drift |
| A03-F007, F014 | Merged | FIN-F006 | RNG/difficulty |
| A03-F008~F010, F012~F013 | Merged | FIN-F005 | Combat/equipment/economy rules |
| A03-F015 | Merged | FIN-F011 | Save corruption |
| A03-F017 | Merged | FIN-F013 | State-loop test gap |
| A04-F001, F006 | Merged | FIN-F002 | Progress/end |
| A04-F002~F004 | Merged | FIN-F003 | Dungeon/content reachability |
| A04-F005, F008 | Merged | FIN-F006 | Pacing/replay |
| A04-F007 | Merged | FIN-F009 | Onboarding |
| A04-F009 | Merged | FIN-F001 | Commercial acceptance |
| A05-F001~F002, F004 | Merged | FIN-F013 | Build/test/startup |
| A05-F003 | Merged | FIN-F014 | Relocatable runtime |
| A05-F005~F006 | Merged | FIN-F011 | Save/config integrity |
| A05-F007~F008 | Merged | FIN-F010 | Schema/progress |
| A05-F009 | Accepted | FIN-F012 | Stale state |
| A05-F010 | Merged | FIN-F006 | Determinism |
| A05-F011 | Merged | FIN-F015 | Dependency provenance |
| A06-F001, F006, F017 | Merged | FIN-F014 / FIN-F001 | Packaging/platform scope |
| A06-F002~F003 | Merged | FIN-F013 | Release QA |
| A06-F004~F005, F016 | Merged | FIN-F015 | Supply chain/legal |
| A06-F007 | Merged | FIN-F001 | Audio scope |
| A06-F008~F010 | Merged | FIN-F011 | Critical data boundary |
| A06-F011~F012 | Merged | FIN-F010 | Save contract |
| A06-F013 | Merged | FIN-F005 | Economy |
| A06-F014 | Merged | FIN-F008 | HUD |
| A06-F015 | Merged | FIN-F009 | i18n |
| A06-F018 | Merged | FIN-F016 | Documentation drift |

기각된 실질 finding은 없다. 숫자 충돌은 FIN-F003에서 정정했다.

## 13. Required Actions Before Passing

### P0 — 데이터와 검증 신뢰 회복

1. FIN-F001의 제품 lane과 1차 출시 범위를 문서로 확정한다.
2. FIN-F011의 per-user atomic save/config, backup/quarantine, typed error와 TPK 분리를 구현한다.
3. FIN-F010의 versioned canonical schema와 migration/default/run-resume 정책을 확정한다.
4. FIN-F013의 Release-safe tests, CTest와 production-path smoke를 복구한다.
5. FIN-F012의 TPK root transition을 수정한다.

### P1 — 한 개의 상용 수직 슬라이스 완결

1. FIN-F002에 따라 한 던전 구간을 목표·랜드마크·종결 전투·결과 화면·저장/재개까지 닫는다.
2. FIN-F003의 loot/quest/event reachability를 실제 플레이 경로에 연결한다.
3. FIN-F004~F006의 party choice, item/target selection, combat invariants, encounter curve와 seed를 정렬한다.
4. FIN-F007~F009의 New/Continue, 실제 HUD, 상태 피드백, 5-locale/input/accessibility를 정렬한다.

### P2 — 실제 배포 후보 만들기

1. FIN-F014의 relocatable Linux/Windows package와 CI artifacts를 만든다.
2. FIN-F015의 dependency/font rights/SBOM/notices/legal/support gate를 닫는다.
3. FIN-F016의 완료 문서와 실제 파일 책임을 동기화한다.
4. fresh package에서 Windows/Linux GUI smoke, 5-locale screen review, 장시간 완주를 수행한다.

### 권장 제품 전략

현재 콘텐츠를 곧바로 다층 정식 게임으로 확장하기보다 먼저 **완결 가능한 상용 데모 또는 Early Access 수직 슬라이스**로 재포지셔닝하는 편이 안전하다. 한 층이라도 목표, 변주, 보상, boss/result, 저장, UI, 패키지를 끝까지 닫은 뒤 층과 콘텐츠를 늘려야 반복적인 drift를 줄일 수 있다.

## 14. Accepted and Remaining Risks

### Accepted as design, not defects

- 레트로 와이어프레임 시각 방향
- 턴제·주사위 기반 전투
- 4인 파티와 단순화된 D&D 규칙 자체
- 의도적으로 설명되고 안전하게 분리될 경우의 하드코어 TPK
- 오디오를 명시적 비목표로 유지하는 선택

### Remaining risks

- Windows/macOS/고DPI/CJK 실제 화면은 미검증
- 장시간 밸런스와 목표 플레이 시간은 미측정
- 폰트 권리와 legal 문서는 Human Review 미완료
- Git source lineage가 현재 snapshot에서 확인되지 않음

## 15. Clarifications and Inconclusive Areas

다음은 구현 전에 제품 오너 결정이 필요하다.

1. 다음 목표가 commercial demo, Early Access, 1.0 중 무엇인가.
2. 한 run의 목표 길이, 층 수, 최종 목표와 엔딩은 무엇인가.
3. dungeon run을 중단 지점에서 resume할 것인가, town-only save로 제한할 것인가.
4. 랜덤 모집을 핵심 규칙으로 유지할 것인가, 직접 캐릭터 생성을 제공할 것인가.
5. quest는 일회성인가 반복형인가.
6. 오디오와 BGM/SFX UI는 출시 범위인가.
7. 지원 OS와 배포 채널은 무엇인가.
8. TPK 영구 리셋에 backup/ironman slot/export 중 어떤 복구 정책을 허용할 것인가.

## 16. Re-audit Checklist

- [ ] 제품 lane/지원 OS/완주 기준이 spec에 확정됨
- [ ] Critical FIN-F011의 원자 저장·backup·corruption quarantine·오류 UI가 failure injection으로 통과
- [ ] canonical schema/version/migration/defaults 및 dungeon resume가 문서·코드·fixture에서 일치
- [ ] TPK 후 stack root와 save 결과가 정상
- [ ] clean Debug/Release 전체 build와 Release CTest가 실제 tests를 실행
- [ ] fixed seed campaign/state transcript가 반복 재현됨
- [ ] 19 item/3 quest의 acquisition/reward/use/save reachability가 닫힘
- [ ] dice/bless/resistance/equip/reward 공식이 canonical ledger test를 통과
- [ ] 실제 Party HUD와 status feedback이 0/1/4명 fixture에서 일치
- [ ] 5 locale 전체 State의 입력·focus·text bounds·대비·CJK raster 검증
- [ ] 최소 수직 슬라이스를 clean save부터 ending까지 완주
- [ ] Linux/Windows relocatable package를 arbitrary cwd에서 실행
- [ ] font/dependency provenance, SBOM, notices, checksum/signature, legal/support review 완료
- [ ] 문서의 완료/Deferred/Unverified와 실제 evidence가 일치

## 17. Final Decision

**HOLD**

현재 Crawlmaster는 20x20 와이어프레임 탐험, 파티/전투/스킬/아이템 모델, 5개 언어 리소스와 Debug 테스트 골격을 가진 의미 있는 기술 프로토타입이다. 그러나 정식 유료 Wizardry 스타일 게임으로서는 다음 세 축이 동시에 닫히지 않았다.

- **제품/콘텐츠:** 완주 목표·boss/ending·층 progression·던전 event/loot·콘텐츠 도달성 부족
- **인터페이스/게임 시스템:** 가짜 Dungeon HUD, 불완전 i18n/input/accessibility, 낮은 player agency와 전투 규칙 drift
- **출시 신뢰:** Critical 저장 손실 경계, Release test 실패, 패키지/CI/권리 증거 부재

따라서 현 상태를 상용 출시 후보로 판정할 수 없다. P0를 먼저 해결하고 한 개의 완결된 상용 수직 슬라이스를 P1로 닫은 뒤 재감사해야 한다.

## 18. Coder Handoff

```text
`/mnt/Projects_SSD/cpp/crawlmaster/docs/multi_audit/1/final_audit_report_1.md`를 먼저 읽고, 각 finding을 프로젝트 문서와 실제 코드에 대조하여 검증한 뒤 우선순위대로 수정하세요. 계약 변경이 필요하면 관련 문서를 먼저 갱신하고, 수정 후 테스트·빌드·재감사 증거를 기록하세요.
```
