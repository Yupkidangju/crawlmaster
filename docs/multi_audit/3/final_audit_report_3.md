# Crawlmaster Turn 3 최종 멀티 감사 보고서

## 1. 감사 메타데이터

- 감사 순번: 3
- 작성일: 2026-09-04 (Asia/Seoul)
- 대상: `/mnt/Projects_SSD/cpp/crawlmaster`
- 기준: `HEAD 927753278f46b92a015197ee229edce4f52e0657`와 현재 미커밋 working tree
- 감사 모드: `AI_AUDIT_DOC_STANDARD.md` 기반 표준 감사
- 사용자 목표: 캐릭터 생성부터 플레이·사망까지, 퀘스트 전체, 3층 맵 생성과 save/load의 멱등성·소멸·재생성, 문서 요청과 구현의 모순을 전체 점검
- 최종 판정: **HOLD**
- 집계: Critical 0, Major 18, Minor 14, Info 3
- 소스/테스트/설정/제품 문서 수정: 없음
- 허용된 쓰기: 이 감사 turn의 보고서와 무결성 manifest만

## 2. 판정 기준

다음 기준을 함께 적용했다.

1. `spec.md`와 승인 설계 문서의 명시적 계약
2. 실제 production 호출 경로와 저장 상태 전이
3. 현재 테스트가 이름 붙여 검증하는 실패 모드
4. 저장·로드·rollback·RNG의 데이터 무결성 불변조건
5. 기대 동작이 충돌하는 경우 결함을 창작하지 않고 `Needs Spec Clarification`으로 분류

Release CTest 13/13은 정상 경로의 유효한 양성 증거다. 그러나 테스트되지 않은 seedless migration, rollback 복원 실패, poison turn-start 사망, escape cleanup과 같은 경로를 반증하지 않는다.

## 3. 범위와 제외 범위

### 포함

- 문서: `spec.md`, `designs.md`, `DESIGN_DECISIONS.md`, `IMPLEMENTATION_SUMMARY.md`, `README.md`, `CHANGELOG.md`, `BUILD_GUIDE.md`, `audit_roadmap.md`, `tasks/*`, `LESSONS_LEARNED.md`
- 캐릭터: identity, 4d6/reroll/point-buy, class trait, 장비·스킬·상태, 성장, 개별 사망, TPK, GameOver, 직렬화
- 퀘스트: 신규 3종과 legacy 3종의 수주·진행·현장 해결·보고·보상·완료 원장·중요품·UI/i18n
- 월드: 3층 생성, 파생 seed, landmark/object, FOW, 층 이동, snapshot, v1~v4 migration, New/Continue/TPK
- 저장: atomic replace, backup/quarantine, typed result, rollback, shutdown, RNG checkpoint, symlink 경계
- 검증: CMake/CTest 등록, current Release CTest, locale parity, 기존 raster 증거, production-linked controller tests

### 제외 또는 미확인

- 생성·벤더·외부 의존성 소스는 감사 대상에서 제외했다.
- clean Windows 10/11, macOS, 실제 OS high-DPI/IME, 30~60분 3층 장시간 플레이와 법률/지원 승인은 현재 환경에서 재검증하지 못했다.
- 현재 v0.10 working tree에 결속된 hosted Windows artifact/SLSA/SPDX는 존재하지 않아 `Not Covered`다.
- 실제 power-loss, 다중 writer와 Windows reparse-point 동작은 실행하지 않았다.

## 4. 작업면 인벤토리

| 작업면 | 주요 권위/구현 | 주요 검증 |
| --- | --- | --- |
| 캐릭터 생성·수명 | `Character`, `RecruitmentDraft`, `CharacterCreationState`, `CombatState`, `GameOverState` | Agency/Combat/Controller/HUD tests |
| 퀘스트 | `Quest`, `Party`, `TownState`, `QuestJournalState`, `DungeonState` | Content/Controller/locale tests |
| 3층 월드 | `DungeonMap`, `DungeonWorld`, `DungeonRenderer` | Content/Controller/UI raster tests |
| 저장·복구 | `Party`, `Persistence`, `SessionRng`, `TitleState`, `Game` | TestHarness/RNG replay/Controller tests |
| 문서·릴리스 | 제품 문서, `CMakeLists.txt`, CI, package 문서 | Git diff, CTest, source SHA 대조 |
| 신뢰 경계 | save/config 경로, JSON parser, symlink, backup/quarantine | malformed fixture, failure injection, 독립 probe |

## 5. 감사자 배정

초기 5개 독립 관점과 Coverage Gap용 supplement 1개를 사용했다.

1. A01 캐릭터 생명주기
2. A02 퀘스트 전체
3. A03 월드 생성·영속성
4. A04 계약·문서·구현 정합성
5. A05 복원력·런타임·데이터 무결성
6. S04 캐릭터/퀘스트/world E2E coverage 보완

맵·save 데이터 무결성은 A03과 A05가 서로 독립적으로 증거를 제시했으므로 고위험 이중 coverage 요건을 충족했다.

## 6. 불변 원본 manifest

- Manifest: `/mnt/Projects_SSD/cpp/crawlmaster/docs/multi_audit/3/source_report_manifest.json`
- Manifest SHA-256: `396f27c1d0b292b611906886a6bad3152a48308d97574a3764b00fc1cb0af998`
- Sidecar: `/mnt/Projects_SSD/cpp/crawlmaster/docs/multi_audit/3/source_report_manifest.sha256.json`
- 누락 원본: 없음

| Report | Perspective | SHA-256 | Completion | Supplement Of |
| --- | --- | --- | --- | --- |
| `sub_audit_01_character_lifecycle.md` | Character lifecycle | `f8b941645aa8faeadcdecd2298c10505e0f6c8de95149a5d5055d030cc2f9cbb` | Complete | None |
| `sub_audit_02_quest_system.md` | Quest system | `d6be9dfdce7f48183fd7fc82c135852a62a05a0837cde0d5572b7359bade43b0` | Complete | None |
| `sub_audit_03_world_persistence.md` | World persistence | `751cbc870b41750e890b5516a8108f58b3bec0d31970c440de6d0334bbf35c03` | Complete | None |
| `sub_audit_04_contract_docs.md` | Contract/docs | `57e41ccc90f13b4f6993f8ad0fc5fd0ee60106cf494f86e8aaf4cf4ba922340e` | Complete | None |
| `sub_audit_04_contract_docs_supplement_1.md` | Coverage supplement | `8ec7a1c2d3616d025406ec37a50857ede79dc2b4715c7ed3c86106c1373dd46f` | Complete | `sub_audit_04_contract_docs.md` |
| `sub_audit_05_resilience_runtime.md` | Resilience/runtime | `e269961a22a56fb03592cefa82607535645df787ac60e7ce5bf8d92a7516b990` | Complete | None |

## 7. 직접 실행 및 증거

- `git diff --check` — PASS
- 5 locale JSON parse/key parity — 각 452 key, PASS
- 현재 Release binary보다 새로운 관련 소스·헤더·테스트 없음 확인
- `ctest --test-dir build/release --output-on-failure --no-tests=error` — **13/13 PASS**, 41.65초
- 직접 실행: `TestHarness`, HUD/Combat/Content/Agency/Localization/Controller contract, resource verify — PASS
- 독립 process RNG Continue — PASS
- 기존 current-tree UI raster 420장, font raster 15장 존재 확인
- `git show 4f988...:CMakeLists.txt` — hosted evidence의 source는 0.9.4, 현재 tree는 0.10.0임을 확인
- `python reserve_audit_turn.py`는 `python` 별칭 부재로 미실행; 동일 스크립트를 `python3`로 정상 실행했다. 프로젝트 결함이 아니다.

## 8. Coverage Gap Check

| 작업면 / 질문 | 배정 | 실제 증거 | Coverage | 후속 |
| --- | --- | --- | --- | --- |
| 캐릭터 생성·성장·상태·개별 사망·TPK | A01, A04, S04 | 문서/호출 경로/부분 controller·model test | Partially Covered | 4인 production lifecycle E2E 필요 |
| 신규·legacy 퀘스트 전체 | A02, A04, S04 | registry, field resolve, Moon Seal report rollback, save/load | Partially Covered | 세 ID별 Castle success/failure/repeat E2E 필요 |
| 3층 생성·snapshot·New Game | A03, A05, S04 | 이중 소스 감사, Content/Controller tests | Covered | 발견된 결함 수정 후 negative matrix |
| save/load/rollback/RNG 데이터 무결성 | A03, A05, Main | 이중 독립 보고서, source, failure tests/probe | Covered | FIN-F001~F007 재감사 |
| 문서-코드 양방향 정합성 | A04, S04, Main | 전체 제품 문서와 코드 대조 | Covered | authority 충돌 정리 |
| Release 정상 회귀 | A04, A05, Main | current Release CTest 13/13 | Covered | Debug/hosted는 별도 evidence 유지 |
| 자동 이동 boss activation 의도 | A04, S04, Main | 상충 문서와 코드 경로 | Partially Covered | 제품 계약 결정 필요 |
| UI/i18n 의미·접근성 | A01, A02, A04 | locale parity/raster/source | Partially Covered | semantic text assertion, 실제 high-DPI/IME |
| current v0.10 hosted Windows/SLSA | A04, Main | 과거 0.9.4 SHA 대조 | Not Covered | v0.10 commit 결속 hosted gate |
| vendor/generated | None | 명시적 제외 | Excluded | 필요 시 별도 공급망 감사 |

핵심 목표의 E2E coverage가 보완 후에도 부분적이며 Major 결함도 남아 있으므로 PASS 계열 판정은 금지한다.

## 9. Canonical Findings

### [FIN-F001] seed 없는 v1/v2 legacy migration이 결정론적이지 않다

- Sources: A01-F009, A02-F001, A03-F001, A04-F003, A05-F004
- Areas: Migration, world seed, quest/object placement
- Severity / Status: **Major / Needs Fix + seed 정책 명세 필요**
- Verified Evidence: `tests/fixtures/save_v1.json`에는 seed가 없고 `Party.cpp:287-302`는 `SessionRng::global().seed()`를 fallback으로 사용한다. global seed는 `SessionRng.cpp:10-29`의 process entropy다. `TitleState.cpp:68-87`은 이후 다른 새 seed를 저장할 수 있다.
- Impact: 동일 legacy bytes가 프로세스마다 다른 3층 월드로 이관되고 `world.seed`와 session metadata가 갈라질 수 있다.
- Required Action: legacy payload에서 versioned deterministic seed를 도출하거나 고정 fallback/거부 정책을 문서화하고 world/RNG metadata를 함께 설정한다.
- Re-audit: v1/v2 seedless fixture를 서로 다른 process seed에서 반복 load/migrate하여 world JSON, object 좌표, migrated seed가 동일한지 확인한다.

### [FIN-F002] rollback/TPK가 Party checkpoint만 복구하고 실제 global RNG는 복구하지 않는다

- Sources: A01-F003, A02-F003, A03-F006, A05-F001
- Areas: RNG checkpoint, combat/quest rollback, TPK
- Severity / Status: **Major / Needs Fix**
- Verified Evidence: save는 `Party.cpp:102-105`에서 global seed/draw count를 기록하고 load는 `Party.cpp:327-336`의 필드만 복원한다. 실제 global 재구성은 `TitleState.cpp:82-84`에만 있다. rollback과 TPK는 `loadFromFile()`만 호출한다.
- Impact: I/O 실패 여부에 따라 보상·레벨업 HP·다음 encounter가 달라지고, GameOver close가 advanced RNG를 정상 checkpoint에 다시 쓸 수 있다.
- Required Action: Party/World/Quest와 RNG를 한 transaction/checkpoint API로 묶고 pre-commit 실패 시 함께 복원한다.
- Re-audit: 실패 전후 seed/draw count와 다음 N개 roll을 independent process baseline과 비교한다.

### [FIN-F003] 보상·보고·월드 변경의 rollback이 디스크 재로드 실패 시 메모리에서 무너진다

- Sources: A02-F002, A05-F002
- Areas: Quest report, combat reward, object/floor transaction
- Severity / Status: **Major / Needs Fix**
- Verified Evidence: `TownState.cpp:63-74`, `CombatState.cpp:773-782`, `DungeonState.cpp:401-440`은 선변이·후저장 후 실패 시 디스크 load에 의존하며 restore 결과를 일부 무시한다.
- Impact: primary/backup 부재·손상·권한 race에서는 gold/XP/item/key/completed/world가 부분 커밋되고 후속 save 또는 retry에서 중복될 수 있다.
- Required Action: durable commit 전 publish하지 않는 in-memory transaction 또는 완전한 pre-state snapshot을 사용하고 restore 실패 시 진행을 차단한다.
- Re-audit: valid/no-primary/corrupt-backup/restore-I/O 조합에서 모든 domain field와 bytes가 pre-state인지 확인한다.

### [FIN-F004] town checkpoint와 dungeon autosave의 복원 계약이 충돌하며 별도 town baseline이 없다

- Sources: A01-F019, A04-F001, A05-F003
- Areas: TPK, checkpoint authority, world persistence
- Severity / Status: **Major / Needs Spec Clarification**
- Verified Evidence: `spec.md:53-54`는 town/ending checkpoint와 last town 복구를, `spec.md:352-354`는 2초 FOW/world save를 동시에 요구한다. `DungeonState.cpp:230-243,363-377`은 같은 `Party::saveToFile()`를 쓰며 별도 town snapshot/path는 없다.
- Impact: TPK가 party/world/FOW 중 무엇을 되돌리는지 재현 가능한 기준이 없고 현재 구현은 최신 dungeon save 전체를 읽는다.
- Required Action: town baseline과 incremental world snapshot을 분리할지, 최신 durable save를 checkpoint로 정의할지 문서를 먼저 결정하고 구현·테스트를 맞춘다.
- Re-audit: town 저장 후 FOW/object/party를 변경·autosave하고 TPK/Continue 결과를 선택한 복원 matrix와 비교한다.

### [FIN-F005] schema v4 필수 필드 누락을 정상값으로 보정해 진행을 조용히 잃는다

- Sources: A01-F015, A02-F006, A03-F002
- Areas: v4 schema, quest progress, corruption recovery
- Severity / Status: **Major / Needs Fix**
- Verified Evidence: `Party.cpp:222-293`은 `keyItems`, `activeQuests`, `completedQuestIds`, campaign/RNG 필드를 optional/default로 읽고 `Quest.cpp:134-173`도 v4 field 상당수를 `contains/value`로 처리한다.
- Impact: 부분 손상 v4가 quarantine되지 않고 quest/campaign/RNG 진행을 빈 기본값으로 재저장할 수 있다.
- Required Action: v4 required keys/type/registry 값을 엄격 검증하고 tolerant default는 문서화된 v1~v3 migration에만 둔다.
- Re-audit: 각 v4 필드를 하나씩 삭제한 negative fixture가 `Corrupt`, no-mutation, quarantine을 만족하는지 확인한다.

### [FIN-F006] 중요품·retrieve quest·world object의 양방향 불변식이 없어 soft-lock/불가능 상태를 수용한다

- Sources: A02-F004, A04-F005, A05-F007
- Areas: Key item, quest/world cross-state
- Severity / Status: **Major / Needs Fix**
- Verified Evidence: `Party.cpp:222-233,304-325`는 ready retrieve에 key가 있는 한 방향만 확인한다. `DungeonState.cpp:433-435`는 orphan key가 이미 있으면 `addKeyItem()` 실패로 objective를 끝내지 못한다.
- Impact: active-unready+key는 영구 soft-lock, completed+key는 보고 후 소비 계약 위반이다.
- Required Action: key↔active-ready↔resolved object와 completed↔no-key 관계를 양방향 검증하거나 상호작용을 명시적으로 idempotent하게 만든다.
- Re-audit: quest/object/key 조합 matrix를 load·E interaction·report까지 검증한다.

### [FIN-F007] world snapshot validator가 Door와 gate/entry semantic을 완전히 검증하지 않는다

- Sources: A01-F008, A03-F003, A04-F004, A05-F008
- Areas: Map snapshot, landmark integrity
- Severity / Status: **Major / Needs Fix**
- Verified Evidence: 생성은 `DungeonMap.cpp:335-363`에서 Door 1개와 최장 거리 V/B를 만들지만 loader `:408-469`는 U/V/B 개수·연결성만 검사하고 Door 수, farthest gate, entry visited/stepped를 검사하지 않는다.
- Impact: landmark 소멸·중복 또는 이른 BossGate를 가진 변조 snapshot이 정상 load/re-save될 수 있다.
- Required Action: 생성/로드/저장에 공통 canonical validator를 두고 Door, gate distance, entry/FOW와 object semantic을 검증한다.
- Re-audit: Door 0/2, 가까운 B/V, entry bit 변조를 각각 거부하는지 확인한다.

### [FIN-F008] DISCOVERED objective와 FOW visited 관계가 닫히지 않는다

- Sources: A01-F007, A02-F007, A03-F005, S04-F006
- Areas: FOW, objective marker
- Severity / Status: **Minor / Needs Fix**
- Verified Evidence: world/Party loader는 `DISCOVERED => visited`를 검사하지 않고 `DungeonRenderer.cpp:185-214`도 visited를 재확인하지 않는다.
- Impact: 손상·변조 save가 fog 속 목표 위치를 노출할 수 있다.
- Required Action: loader와 renderer 양쪽에서 visited 조건을 강제한다.
- Re-audit: discovered/unvisited fixture load와 raster negative assertion을 추가한다.

### [FIN-F009] persisted Character 이름이 생성 identity 계약을 우회한다

- Sources: A01-F001, A04-F008
- Areas: Character identity, UTF-8/control input
- Severity / Status: **Major / Needs Fix**
- Verified Evidence: `RecruitmentDraft.cpp:123-161`은 일부 control만 거부하고 `Character.cpp:460-480`은 이름을 non-empty/64 bytes로만 검사한다. spec은 Unicode 1~16자와 control 금지를 요구한다.
- Impact: 17+ code point, C1/control, malformed/공백 이름이 v4로 수용·재저장되고 UI/log 계약을 깨뜨린다.
- Required Action: 하나의 Unicode identity validator를 draft와 v3/v4 deserializer가 공유하고 trim/거부 정책을 명시한다.
- Re-audit: malformed UTF-8, C1/format/control, 17 code point, Unicode whitespace fixture를 검사한다.

### [FIN-F010] Character load가 class/level과 spell-slot semantic을 검증하지 않는다

- Sources: A01-F006
- Areas: Character class progression
- Severity / Status: **Major / Needs Fix**
- Verified Evidence: constructor/level-up은 caster에 level 기반 slot을 주지만 `Character.cpp:486-510`은 수치 범위만 검사한다.
- Impact: Mage/Cleric의 slot 소실 또는 Warrior/Rogue의 phantom slot이 canonical v4로 굳어진다.
- Required Action: class·level별 max slot 불변식을 검증하거나 명시적 가변 계약을 추가한다.
- Re-audit: 모든 class/level의 valid/invalid slot matrix를 load한다.

### [FIN-F011] poison turn-start 사망 후 non-TPK entity가 행동을 계속할 수 있다

- Sources: A01-F018
- Areas: Combat turn, individual death
- Severity / Status: **Major / Needs Fix**
- Verified Evidence: `CombatState.cpp:380-437`은 effect 전 dead만 건너뛰고 poison 처리 뒤 해당 entity가 죽었는지 재검사하지 않는다. 다른 party/foe가 살아 있으면 전체 victory/defeat도 false다.
- Impact: 죽은 캐릭터가 공격/스킬/아이템을 쓰거나 죽은 몬스터가 한 번 더 공격할 수 있다.
- Required Action: turn effect 직후 entity death를 재확인하고 action 없이 다음 턴으로 넘긴다.
- Re-audit: 2인 party와 2 foe에서 각각 한 entity만 poison으로 죽는 production turn test를 추가한다.

### [FIN-F012] 성공적인 도주가 전투 전용 STR/DEX/Bless buff를 지우지 않는다

- Sources: A01-F004
- Areas: Combat termination, buff lifecycle
- Severity / Status: **Major / Needs Fix**
- Verified Evidence: `CombatState.cpp:349-363`은 victory에서만 `clearCombatBuffs()`를 호출하고 성공 도주 `:531-555`는 즉시 pop한다.
- Impact: 전투 전용 buff가 탐험과 다음 encounter에 남고 live memory와 비직렬화 save가 달라진다.
- Required Action: 모든 combat terminal path를 공통 cleanup으로 통합한다.
- Re-audit: 각 buff 후 결정적 escape에서 값이 0이고 다음 전투/save와 일치하는지 확인한다.

### [FIN-F013] CharacterInfoState가 문서화된 독·마비·buff/dead 상태를 표시하지 않는다

- Sources: A01-F005, A04-F009
- Areas: Character UI, state observability
- Severity / Status: **Major / Needs Fix**
- Verified Evidence: `designs.md:291-302`는 상태/buff 행을 요구하지만 `CharacterInfoState.cpp` normal/large detail은 해당 getter를 사용하지 않는다. 기존 poison fixture raster에도 행이 없다.
- Impact: 사용자가 치료·자원 결정을 위해 캐릭터의 전체 현재 상태를 확인할 수 없다.
- Required Action: locale 기반 상태/buff/dead 표시와 semantic raster assertion을 추가한다.
- Re-audit: healthy/dead/poison/paralysis/STR/DEX/Bless 조합을 5 locale·scale에서 검사한다.

### [FIN-F014] save가 없는 New Game은 문서상 확인 단계를 우회한다

- Sources: A01-F002
- Areas: New Game, destructive confirmation
- Severity / Status: **Major / Needs Fix**
- Verified Evidence: `designs.md:331-345`는 항상 Confirm을 요구하지만 `TitleState.cpp:52-66`은 recoverable save가 없으면 바로 session/world를 초기화하고 저장한다.
- Impact: stale in-memory session을 포함해 경로별 confirmation/cancel 계약이 달라진다.
- Required Action: no-save에서도 명시 confirm을 거치거나 문서에서 예외를 명시한다.
- Re-audit: 빈 save directory에서 첫 Enter·Escape의 party/save bytes 불변을 검증한다.

### [FIN-F015] 정상 창 종료가 save 결과를 무시하고 닫힌다

- Sources: A05-F009
- Areas: Shutdown durability
- Severity / Status: **Major / Needs Fix**
- Verified Evidence: `Game.cpp:60-69`는 active session을 save한 뒤 `PersistenceResult`를 버리고 창을 닫는다.
- Impact: pre-commit failure에서 최근 character/quest/world 진행을 경고·재시도 없이 잃는다.
- Required Action: success/unknown/failure별 종료 UI와 retry/leave 정책을 구현한다.
- Re-audit: close event에 write/sync/unknown failure를 주입해 window, banner, bytes와 Continue를 확인한다.

### [FIN-F016] 자동 이동의 quest boss/BossGate 활성화 계약이 문서 사이에서 충돌한다

- Sources: A04-F002, S04-F004
- Areas: Auto-move, boss interaction
- Severity / Status: **Major / Needs Spec Clarification**
- Verified Evidence: `spec.md:208`은 active quest boss 칸 진입 즉시 전투를, `designs.md:60`은 자동 이동이 목표를 자동 활성화하지 않는다고 한다. `DungeonState.cpp:245-345`은 auto step에서도 boss/BossGate 전투를 시작해 spec 쪽을 따른다.
- Impact: 예상치 못한 강제 전투인지 의도된 entry trigger인지 테스트 기준을 정할 수 없다.
- Required Action: 제품 계약을 먼저 하나로 정하고 다른 문서·코드·controller test를 동기화한다.
- Re-audit: boss/BossGate/item/NPC/stairs를 destination/중간 node로 둔 auto-path transcript를 실행한다.

### [FIN-F017] active roadmap/CHANGELOG가 v3·1층·0.9.4와 v4·3층·0.10을 혼합한다

- Sources: A04-F010
- Areas: Documentation authority, release gate
- Severity / Status: **Major / Needs Documentation Recovery**
- Verified Evidence: `audit_roadmap.md:23-64`는 active gate에서 v3/한 층을, `:73-80`은 v4/3층을 함께 쓴다. `CHANGELOG.md:9-17`은 0.10 heading이 중복되고 같은 Unreleased에 구 lane 문장이 남는다.
- Impact: 어떤 schema/phase를 현재 완료 기준으로 삼을지 오판할 수 있다.
- Required Action: 과거 기준을 historical/superseded로 격리하고 active 문서를 v0.10 하나로 정리한다.
- Re-audit: active 문서의 v3/한 층/0.9.4 표현이 historical context 밖에 남지 않는지 검사한다.

### [FIN-F018] current v0.10 hosted Windows/SLSA 완료 주장이 구 0.9.4 source에 결속돼 있다

- Sources: A04-F011
- Areas: Evidence provenance, hosted release
- Severity / Status: **Major / Needs Fix**
- Verified Evidence: 제품 문서가 인용한 `4f988...`의 CMake version은 0.9.4이고 현재 tree는 0.10.0이며 46개 tracked 파일에 대규모 변경이 있다.
- Impact: 신규 캐릭터·퀘스트·DungeonWorld/schema v4가 Windows package와 attestation에서 검증됐다는 인상을 준다.
- Required Action: current v0.10 immutable SHA로 hosted build/test/package/attestation을 재실행하거나 문서 주장을 `UNVERIFIED`로 낮춘다.
- Re-audit: artifact/attestation subject와 current commit SHA, test list, package version을 대조한다.

### [FIN-F019] 사용자 핵심 lifecycle E2E가 부분 coverage에 머문다

- Sources: A02-F010(coverage 부분만), A03-F010, A04-F006, A04-F007, A05-F010, S04-F002, S04-F003, S04-F005
- Areas: Character/quest/world integration tests
- Severity / Status: **Major / Hold**
- Verified Evidence: current Release CTest 13/13은 통과했지만 테스트는 한 명 creation, fixture TPK, 세 field resolve, Moon Seal report rollback, 단일 world round-trip으로 분절돼 있다.
- Impact: 4인 creation→개별 사망→TPK, 세 quest별 Castle report/repeat/failure, repeated Continue/re-entry/save-load-regenerate를 완결된 production flow로 보증하지 못한다.
- Required Action: finding별 failure-specific production-linked E2E를 추가하고 Debug/Release에서 실행한다.
- Re-audit: S04 Coverage Gap Matrix의 Not/Partially Covered 행을 실제 transcript로 모두 닫는다.

### [FIN-F020] Quest progress mutator가 음수를 허용해 self-invalidating save를 만든다

- Sources: A01-F010
- Severity / Status: Minor / Needs Fix
- Evidence: `Quest.cpp:18-27`은 음수를 clamp/reject하지 않지만 loader `:158-160`은 음수 progress를 거부한다.
- Action: progress를 `[0,target]`에 고정하고 negative/overflow round-trip test를 추가한다.

### [FIN-F021] Party 완료 원장과 `Quest::isCompleted`가 이중 권위로 갈라진다

- Sources: A01-F011
- Severity / Status: Minor / Needs Fix
- Evidence: `Party.cpp:475-482`는 completed ID만 넣고 retained Quest의 flag는 false로 남긴다.
- Action: 한 권위를 선택해 flag를 갱신하거나 제거하고 pointer/ledger coherence를 테스트한다.

### [FIN-F022] `Party::addMember(nullptr)`가 유효 slot처럼 수용된다

- Sources: A01-F012
- Severity / Status: Minor / Needs Fix
- Evidence: `Party.cpp:34-41`은 null을 검사하지 않으며 Town/CharacterInfo는 member를 역참조한다.
- Action: null을 거부하고 count/save/UI invariant를 테스트한다.

### [FIN-F023] save byte-level idempotence 계약이 정의되지 않았고 unordered set 순서를 그대로 쓴다

- Sources: A01-F013, A03-F008
- Severity / Status: Minor / Needs Spec Clarification
- Evidence: `Party.cpp:123-149`은 unordered set을 정렬 없이 array로 쓴다.
- Action: semantic-only인지 exact bytes인지 계약을 정하고 exact이면 sort/canonical dump를 적용한다.

### [FIN-F024] CharacterInfo의 full-HP healing potion no-op 소비 정책이 Combat과 다르다

- Sources: A01-F014
- Severity / Status: Minor / Needs Spec Clarification
- Evidence: Combat은 no-effect를 거부하지만 CharacterInfo는 full HP potion을 제거한다.
- Action: 전역 no-op 정책 여부를 정하고 공유 effect-commit 규칙으로 맞춘다.

### [FIN-F025] 실패한 Party load가 기존 memory는 두고 active-session flag만 지운다

- Sources: A01-F016
- Severity / Status: Minor / Needs Fix
- Evidence: `Party.cpp:174-176`에서 선제 false 처리 후 실패 반환하며 `Game.cpp` close save 조건이 달라진다.
- Action: 실패 시 이전 flag/state를 원자 보존하거나 명시적 non-session error state로 전환한다.

### [FIN-F026] domain `completeQuest`가 boss/NPC world RESOLVED 상태를 자체 검증하지 않는다

- Sources: A01-F017
- Severity / Status: Minor / Needs Fix
- Evidence: 정상 controller는 object를 resolve하지만 `Party.cpp:424-482` public report path는 이를 확인하지 않아 unloadable state를 만들 수 있다.
- Action: report transaction에서 canonical object state를 검증한다.

### [FIN-F027] completed quest ID가 unknown/duplicate 값을 수용한다

- Sources: A02-F005
- Severity / Status: Minor / Needs Fix
- Evidence: `Party.cpp:261-273`은 길이만 검사하고 set insertion 중복 결과를 무시한다.
- Action: canonical registry와 duplicate를 검증한다.

### [FIN-F028] public custom Quest acceptance 정책이 문서에 없다

- Sources: A02-F008
- Severity / Status: Info / Needs Spec Clarification
- Evidence: Town은 canonical-only지만 public constructor/accept는 arbitrary quest를 수용하고 다음 load가 거부할 수 있다.
- Action: canonical-only 또는 extensible registry를 결정한다.

### [FIN-F029] legacy-specific quest locale key 9개가 orphan이다

- Sources: A02-F009
- Severity / Status: Info / Needs Documentation Recovery
- Evidence: 5 locale의 `TOWN_MSG_QUEST_*` 9개 key에 runtime/test consumer가 없다.
- Action: 제거하거나 legacy-only 보존 이유와 검증을 기록한다.

### [FIN-F030] mutable world를 저장 전 검증하지 않는다

- Sources: A03-F004
- Severity / Status: Minor / Needs Fix
- Evidence: public mutable object/floor API가 있고 `DungeonWorld::toJson()`은 generated 여부만 검사한다.
- Action: load와 동일한 invariant validator를 save 직전에 적용한다.

### [FIN-F031] 손상 `.bak`가 반복 recovery candidate로 남는다

- Sources: A03-F007
- Severity / Status: Minor / Needs Fix
- Evidence: primary가 없거나 primary/backup 둘 다 손상일 때 backup parse failure를 quarantine하지 않는다.
- Action: candidate별 typed quarantine/result를 구현한다.

### [FIN-F032] legacy no-arg `DungeonMap::generate()`가 global combat RNG를 소비한다

- Sources: A03-F009
- Severity / Status: Info / Needs Documentation Recovery
- Evidence: shipped world path는 안전하지만 no-arg overload는 global draw를 사용하고 tests가 계속 호출한다.
- Action: 제거/deprecate하거나 generation RNG를 분리하고 모든 entrypoint draw-count invariant를 테스트한다.

### [FIN-F033] save load와 write의 symlink 정책이 비대칭이다

- Sources: A05-F005
- Severity / Status: Minor / Needs Fix
- Evidence: write는 leaf symlink를 거부하지만 load는 따라간다. A05 probe에서 외부 valid save symlink load가 성공했다.
- Action: primary/backup/temp와 parent의 플랫폼별 symlink/reparse 정책을 명시하고 양쪽에 적용한다.
- Rationale: 같은 사용자 권한의 local file injection이며 대상 파일 쓰기는 거부돼 원본 Major를 Minor로 낮췄다.

### [FIN-F034] UI 본문 최소 16px 계약과 14px clamp가 다르다

- Sources: A04-F012
- Severity / Status: Minor / Needs Fix
- Evidence: `designs.md:350`은 body 16/secondary 14 minimum이나 `LocalizationManager.hpp:55-57`은 모든 역할을 14로 clamp한다.
- Action: text role별 helper를 분리하거나 계약을 실제 정책으로 갱신한다.

### [FIN-F035] orphan `getShopCatalog()`가 실제 8종 구매 목록과 다른 18종을 반환한다

- Sources: A04-F013
- Severity / Status: Minor / Needs Spec Clarification
- Evidence: Town은 8종 hard-code를 쓰지만 unused public API 이름/주석은 고급 아이템까지 shop catalog로 반환한다.
- Action: 구매 8종의 단일 진실원으로 만들거나 API 의미를 sellable registry로 명확히 바꾼다.

## 10. Critical/Major 직접 재검증

| Finding | Main 직접 확인 | 재개방 파일/명령 | 결과 | Gate |
| --- | --- | --- | --- | --- |
| FIN-F001 | Yes | v1 fixture, `Party.cpp:287-302`, `SessionRng.cpp`, `TitleState.cpp` | Confirmed | 차단 |
| FIN-F002 | Yes | `Party.cpp:102-105,327-336`, rollback/TPK callers | Confirmed | 차단 |
| FIN-F003 | Yes | Town/Combat/Dungeon mutation→save→load 경로 | Confirmed | 차단 |
| FIN-F004 | Yes | `spec.md:53-54,352-354`, Party API, Dungeon autosave | Confirmed conflict | 차단 |
| FIN-F005 | Yes | Party/Quest v4 parser `contains/value` 경로 | Confirmed | 차단 |
| FIN-F006 | Yes | key/quest/object load 및 interaction 경로 | Confirmed | 차단 |
| FIN-F007 | Yes | generator와 `DungeonMap::fromJson` validator 대조 | Confirmed | 차단 |
| FIN-F009 | Yes | spec identity와 draft/Character parser 대조 | Confirmed | 차단 |
| FIN-F010 | Yes | constructor/level-up/parser slot formula 대조 | Confirmed | 차단 |
| FIN-F011 | Yes | `CombatState::nextTurn` effect 전후 흐름 | Confirmed | 차단 |
| FIN-F012 | Yes | victory/escape cleanup 경로 | Confirmed | 차단 |
| FIN-F013 | Yes | designs와 CharacterInfo getter 소비 검색 | Confirmed | 차단 |
| FIN-F014 | Yes | Title no-save/file-present branch | Confirmed | 차단 |
| FIN-F015 | Yes | `Game::processEvents` close path | Confirmed | 차단 |
| FIN-F016 | Yes | spec/design/auto-step/boss call path | Confirmed conflict | 차단 |
| FIN-F017 | Yes | roadmap/CHANGELOG/current version 검색 | Confirmed | 차단 |
| FIN-F018 | Yes | `git show 4f988...:CMakeLists.txt`, current CMake | Confirmed | 차단 |
| FIN-F019 | Yes | current tests와 Release CTest 13/13 대조 | Confirmed gap | 차단 |

직접 재검증하지 못한 Critical/Major finding은 없다. Critical 자체도 0건이다.

## 11. Cross-report conflicts

1. **Current test 실행 여부:** A01/A02/S04는 일부 build 위치만 보고 current 전체 실행을 미확인 또는 0건으로 판정했다. A04/A05와 메인 재실행은 current `build/release` 13/13을 확인했다. 따라서 “current Release CTest 미실행” 주장은 기각했지만 failure-mode coverage gap은 FIN-F019로 유지했다.
2. **v4 legacy active quest:** A05-F006은 v4가 legacy quest를 거부해야 한다고 했으나, v1~v3 active legacy quest는 migration 뒤 v4로 저장되어 완료·보고까지 살아야 한다는 `spec.md:199,575` 계약과 충돌한다. 이 finding은 false positive로 기각했다.
3. **Auto-move boss:** A04는 구현 결함으로, S04는 문서 충돌로 봤다. master spec은 entry trigger, designs는 no-auto-activation을 요구하므로 FIN-F016을 `Needs Spec Clarification`으로 판정했다.
4. **FOW marker severity:** A01은 Major, A02/A03/S04는 Minor로 평가했다. 정상 UI가 직접 만드는 상태가 아니라 malformed save 방어 공백이고 별도 진행 soft-lock은 FIN-F006에서 다루므로 FIN-F008은 Minor로 낮췄다.
5. **Symlink load severity:** A05는 Major였으나 현재 위협 모델에서 동일 사용자 local read/injection이고 write는 거부된다. 정책 비대칭은 유지하되 FIN-F033을 Minor로 낮췄다.

## 12. Finding adjudication ledger

| Source Finding | Decision | Canonical | Rationale |
| --- | --- | --- | --- |
| A01-F001 | Merged | FIN-F009 | persisted identity validator 공백 |
| A01-F002 | Accepted | FIN-F014 | no-save confirm 우회 |
| A01-F003 | Merged | FIN-F002 | TPK RNG 미복원 |
| A01-F004 | Accepted | FIN-F012 | escape cleanup 누락 |
| A01-F005 | Merged | FIN-F013 | CharacterInfo status 누락 |
| A01-F006 | Accepted | FIN-F010 | slot semantic validation |
| A01-F007 | Merged/Downscoped | FIN-F008 | malformed FOW 상태 |
| A01-F008 | Merged | FIN-F007 | landmark validator |
| A01-F009 | Merged | FIN-F001 | seedless migration |
| A01-F010 | Accepted | FIN-F020 | negative progress |
| A01-F011 | Accepted | FIN-F021 | completion dual state |
| A01-F012 | Accepted | FIN-F022 | null member |
| A01-F013 | Merged | FIN-F023 | byte idempotence |
| A01-F014 | Accepted | FIN-F024 | no-op policy ambiguity |
| A01-F015 | Merged | FIN-F005 | v4 required fields |
| A01-F016 | Accepted | FIN-F025 | failed-load session flag |
| A01-F017 | Accepted/Downscoped | FIN-F026 | shipped controller는 정상, domain guard 공백 |
| A01-F018 | Accepted | FIN-F011 | poison death turn |
| A01-F019 | Merged | FIN-F004 | checkpoint conflict |
| A02-F001 | Merged | FIN-F001 | seedless migration |
| A02-F002 | Merged | FIN-F003 | restore failure rollback |
| A02-F003 | Merged | FIN-F002 | RNG rollback |
| A02-F004 | Merged | FIN-F006 | orphan key soft-lock |
| A02-F005 | Accepted | FIN-F027 | completed ID validation |
| A02-F006 | Merged | FIN-F005 | missing quest progress |
| A02-F007 | Merged | FIN-F008 | FOW/object relation |
| A02-F008 | Accepted | FIN-F028 | custom quest policy |
| A02-F009 | Accepted | FIN-F029 | orphan locale keys |
| A02-F010 | Merged/Partially Rejected | FIN-F019 | coverage 유지, current CTest 0 주장은 반증 |
| A03-F001 | Merged | FIN-F001 | seedless migration |
| A03-F002 | Merged | FIN-F005 | v4 required fields |
| A03-F003 | Merged | FIN-F007 | landmark semantic |
| A03-F004 | Accepted | FIN-F030 | save-time validator |
| A03-F005 | Merged | FIN-F008 | FOW/object relation |
| A03-F006 | Merged | FIN-F002 | RNG checkpoint |
| A03-F007 | Accepted | FIN-F031 | corrupt backup |
| A03-F008 | Merged | FIN-F023 | byte idempotence |
| A03-F009 | Accepted | FIN-F032 | legacy RNG overload |
| A03-F010 | Merged | FIN-F019 | world lifecycle coverage |
| A04-F001 | Merged | FIN-F004 | checkpoint authority |
| A04-F002 | Reclassified | FIN-F016 | 구현 결함보다 문서 충돌 우선 |
| A04-F003 | Merged | FIN-F001 | seedless migration |
| A04-F004 | Merged | FIN-F007 | Door validator |
| A04-F005 | Merged | FIN-F006 | completed+key 상태 |
| A04-F006 | Merged | FIN-F019 | quest E2E gap |
| A04-F007 | Merged | FIN-F019 | character E2E gap |
| A04-F008 | Merged | FIN-F009 | identity load |
| A04-F009 | Merged | FIN-F013 | CharacterInfo status |
| A04-F010 | Accepted | FIN-F017 | active docs drift |
| A04-F011 | Accepted | FIN-F018 | old hosted source |
| A04-F012 | Accepted | FIN-F034 | typography drift |
| A04-F013 | Accepted | FIN-F035 | shop catalog ambiguity |
| A05-F001 | Merged | FIN-F002 | RNG rollback |
| A05-F002 | Merged | FIN-F003 | restore failure rollback |
| A05-F003 | Merged | FIN-F004 | town checkpoint absence |
| A05-F004 | Merged | FIN-F001 | seedless migration |
| A05-F005 | Accepted/Downscoped | FIN-F033 | local symlink policy mismatch |
| A05-F006 | Rejected | None | migrated legacy active quest의 v4 생존 계약과 충돌 |
| A05-F007 | Merged | FIN-F006 | key soft-lock |
| A05-F008 | Merged | FIN-F007 | Door validator |
| A05-F009 | Accepted | FIN-F015 | shutdown save result |
| A05-F010 | Merged | FIN-F019 | failure-mode coverage |
| S04-F001 | Rejected by later evidence | None | 메인이 current Release CTest 13/13 재실행 |
| S04-F002 | Merged | FIN-F019 | character E2E gap |
| S04-F003 | Merged | FIN-F019 | quest E2E gap |
| S04-F004 | Merged | FIN-F016 | auto-move authority conflict |
| S04-F005 | Merged | FIN-F019 | world lifecycle gap |
| S04-F006 | Merged | FIN-F008 | FOW/object relation |

## 13. PASS 전 필수 조치

1. `spec.md`에서 town checkpoint/world autosave/TPK 복원 matrix와 auto-move boss activation을 먼저 확정한다.
2. seedless legacy migration 정책을 확정하고 world seed와 RNG checkpoint를 하나로 만든다.
3. quest/combat/world transaction에 Party·World·Quest·key item·RNG의 완전한 rollback을 구현한다.
4. v4 required schema와 map/object/key/character semantic validation을 강화한다.
5. poison 사망 턴, escape buff cleanup, CharacterInfo status, New Game confirm, shutdown save failure를 수정한다.
6. FIN-F019의 4인 character, 세 quest, repeated world lifecycle production E2E를 Debug/Release에서 추가·실행한다.
7. active 문서를 v0.10 하나로 동기화하고 current SHA에 결속된 hosted Windows/package/attestation evidence를 새로 만든다.

## 14. 남은 위험과 미확정 사항

- Accepted Risk로 승인된 항목은 없다.
- exact byte idempotence, out-of-combat no-op item, custom quest API, shop catalog 의미는 제품 결정이 필요하다.
- current Release 13/13은 통과했지만 clean Windows, high-DPI/IME, 장시간 3층 플레이는 `UNVERIFIED`다.
- 실제 power-loss/multi-writer/Windows reparse-point는 별도 환경 검증이 필요하다.

## 15. Needs Spec Clarification 및 Inconclusive 영역

- FIN-F004: TPK가 party와 world/FOW 중 어느 구성요소를 last-town baseline으로 되돌릴지 결정이 필요하다.
- FIN-F016: 자동 이동의 quest boss/BossGate 진입을 즉시 전투로 볼지 `E` 확인 대상으로 볼지 결정이 필요하다.
- FIN-F023: save 멱등성의 기준이 JSON semantic equality인지 exact byte equality인지 결정이 필요하다.
- FIN-F024: 효과 없는 소모품 비소비 규칙을 CharacterInfo의 비전투 사용에도 적용할지 결정이 필요하다.
- FIN-F028/FIN-F035: custom quest와 shop catalog public API의 확장 정책이 미확정이다.
- clean Windows, 실제 high-DPI/IME, 장시간 플레이, power-loss/multi-writer는 이번 로컬 감사로 결론 낼 수 없다.

## 16. 재감사 체크리스트

- [ ] FIN-F001~F007 failure matrix와 independent-process 결과
- [ ] FIN-F009~F015 character lifecycle regression
- [ ] FIN-F016/FIN-F004 문서 계약 선행 확정
- [ ] 4인 creation→play→individual death→TPK production E2E
- [ ] 세 objective quest별 accept→resolve→report→reload→repeat/failure E2E
- [ ] repeated Continue/town re-entry/save-load/New Game world semantic 비교
- [ ] Debug/Release 13/13, locale/raster semantic assertion
- [ ] current v0.10 SHA의 hosted Windows package/startup/SLSA/SPDX
- [ ] 원본 manifest `verify` 재실행 및 source tree 외 예상치 못한 변경 없음 확인

## 17. 최종 판정

**HOLD**

정상 current Release CTest는 13/13 통과했고, 신규 3층 생성·v4 round-trip·세 objective의 기본 현장 경로·TPK root replacement 등 상당 부분은 실제로 연결돼 있다. 그러나 seed 없는 legacy 이관, rollback/RNG 원자성, checkpoint authority, strict v4 validation, poison 사망 턴과 combat cleanup에 Major 결함이 남아 있다. 캐릭터와 퀘스트의 완전한 production E2E도 보완 감사 후 여전히 부분 coverage다. 따라서 현재 트리를 기능·데이터 무결성 관점에서 PASS로 올릴 수 없다.

## 18. Coder Handoff

`/mnt/Projects_SSD/cpp/crawlmaster/docs/multi_audit/3/final_audit_report_3.md`를 먼저 읽고, 각 finding을 프로젝트 문서와 실제 코드에 대조하여 검증한 뒤 우선순위대로 수정하세요. 계약 변경이 필요하면 관련 문서를 먼저 갱신하고, 수정 후 테스트·빌드·재감사 증거를 기록하세요.
