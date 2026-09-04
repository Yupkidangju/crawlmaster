# Crawlmaster Turn 3 remediation 재감사 보고서

## 1. 메타데이터와 판정

- 작성일: 2026-09-04 (Asia/Seoul)
- 원 finding: `docs/multi_audit/3/final_audit_report_3.md`의 FIN-F001~FIN-F035
- 기준 tree: `HEAD 927753278f46b92a015197ee229edce4f52e0657` + 현재 미커밋 working tree
- 방법: 문서 계약 선행 수정, failure-specific regression, production-linked controller E2E, Debug/Release/패키지 재검증
- 원본 manifest: `396f27c1d0b292b611906886a6bad3152a48308d97574a3764b00fc1cb0af998`, helper verify PASS
- **로컬 remediation 판정: PASS — 35/35 addressed**
- **전체 release 판정: HOLD — current v0.10 hosted Windows/SLSA와 human/OS 실기 gate는 Not Covered**

이 보고서는 구현자 self-review를 포함한다. 원 Turn 3 보고서와 manifest는 수정하지 않았으며, 별도 감사자의 독립 판정은 이 보고서보다 우선할 수 있다.

## 2. 동결한 계약

1. TPK/rollback 권위는 별도 town 파일이 아니라 가장 최근 성공한 full-session Party·Quest·World·RNG checkpoint다.
2. 자동 이동도 quest boss/BossGate 칸 진입 시 전투를 시작한다. 아이템·NPC·계단은 `E`를 요구한다.
3. 동일 domain/RNG 상태의 save는 exact-byte canonical이다.
4. 효과 없는 소모품은 전투 안팎 모두 비소비다.
5. Quest는 canonical-only, shop catalog는 실제 구매 8종이다.
6. 14px은 고정 1024x768 TUI의 일반 본문 최소값이고 핵심 상태/선택은 16px 이상이다.

## 3. Finding 재감사 ledger

| Finding | 재감사 상태 | 수정·검증 증거 |
| --- | --- | --- |
| FIN-F001 | Verified | seedless v1/v2를 canonical JSON+salt FNV-1a seed로 이관하고 world/RNG seed를 통일했다. 서로 다른 process seed의 A/B 결과를 `compare_files`로 검증했다. |
| FIN-F002 | Verified | 성공 load와 TPK가 global RNG seed/draw를 복원한다. mutation rollback은 `PartyCheckpoint`로 RNG까지 되돌리며 다음 roll/카운트를 검증했다. |
| FIN-F003 | Verified | Town/Combat/Dungeon/CharacterInfo/CharacterCreation rollback을 pre-state in-memory snapshot으로 변경했다. primary/backup을 제거한 저장 실패에서도 gold/XP/item/key/quest/world/buff/RNG가 복원된다. |
| FIN-F004 | Verified | latest full-session checkpoint로 계약을 확정했다. dungeon autosave 뒤 party/FOW를 변경한 TPK가 마지막 성공 전체 snapshot을 복원한다. |
| FIN-F005 | Verified | v4 root, Character, Quest 필드를 필수화하고 각 필드 삭제 matrix를 `Corrupt`로 검증했다. tolerant default는 v1~v3에만 남겼다. |
| FIN-F006 | Verified | key↔retrieve ready↔resolved object와 completed↔no-key 양방향 검증을 load/save에 적용했다. orphan key 현장 상호작용은 idempotent하게 정상화한다. |
| FIN-F007 | Verified | 공통 world/map validator가 Door 1개, entry U/FOW, V/B 최장 거리, 연결성과 object 위치를 load/save 모두 검사한다. Door 0/2·early gate·entry bit 변조를 거부한다. |
| FIN-F008 | Verified | `DISCOVERED => visited`를 loader와 renderer 모두 강제하고 hidden marker fixture를 거부한다. |
| FIN-F009 | Verified | strict UTF-8/codepoint/control/format/Unicode whitespace 규칙을 `CharacterIdentityRules`로 공유한다. malformed/C1/format/17자/noncanonical 저장을 거부한다. |
| FIN-F010 | Verified | Mage/Cleric max slot=`level+1`, Warrior/Rogue=0을 모든 class×level matrix로 검증한다. |
| FIN-F011 | Verified | poison effect 직후 개별 사망을 다시 확인해 행동 없이 다음 turn으로 넘긴다. 2 party/2 foe production turn test가 통과했다. |
| FIN-F012 | Verified | victory와 escape가 공통 `clearPartyCombatBuffs()`를 사용한다. 성공 도주 뒤 STR/DEX/Bless가 모두 0이다. |
| FIN-F013 | Verified | CharacterInfo normal/large layout에 Dead/Poison/Paralysis/STR/DEX/Bless와 남은 턴을 표시한다. semantic string assertion과 5 locale raster를 통과했다. |
| FIN-F014 | Verified | save 유무와 관계없이 첫 Enter는 confirm만 열고 Esc 취소가 memory/save bytes를 보존한다. |
| FIN-F015 | Verified | `ShutdownState`가 save failure/unknown에서 창을 유지하고 Enter retry/Esc explicit leave를 제공한다. failure injection과 raster를 통과했다. |
| FIN-F016 | Verified | 문서를 entry-trigger로 통일했다. item/NPC/stair는 auto step으로 해결되지 않고 quest boss/BossGate는 CombatState로 전이한다. |
| FIN-F017 | Verified | active roadmap을 Turn 3/v0.10으로 전환하고 1~4절을 historical로 격리했다. CHANGELOG 중복 0.10 항목을 제거했다. |
| FIN-F018 | Verified (claim correction) | 0.9.4 hosted 증거를 historical로 명시하고 current v0.10 Windows/SLSA 완료 주장을 모두 `UNVERIFIED`로 낮췄다. 실제 hosted 재실행은 Not Covered다. |
| FIN-F019 | Verified (local) | 4인 UI 생성→개별 poison 사망→TPK, 세 quest 각각 failure/success/repeat/reload, repeated Continue/re-entry/New Game, auto objective matrix를 production-linked controller test로 추가했다. |
| FIN-F020 | Verified | 음수 progress는 0, 양수 overflow는 target까지 saturating clamp한다. |
| FIN-F021 | Verified | Party ledger를 완료 권위로 유지하면서 retained Quest pointer도 완료 전에 `setCompleted(true)`로 동기화한다. |
| FIN-F022 | Verified | `Party::addMember(nullptr)`를 거부하고 count 불변을 테스트했다. |
| FIN-F023 | Verified | unordered key/completed set을 정렬하고 save→load→save exact bytes를 검증했다. |
| FIN-F024 | Verified | CharacterInfo도 `CombatActionRules::canUseConsumable`을 사용해 full-HP/no-effect 소비와 RNG draw를 막는다. |
| FIN-F025 | Verified | load는 모든 validation 성공 전 기존 state/active flag/global RNG를 바꾸지 않는다. 실패 뒤 값과 member pointer identity를 검증했다. |
| FIN-F026 | Verified | 목적형 report는 대응 canonical world object가 RESOLVED일 때만 보상을 지급한다. |
| FIN-F027 | Verified | completed IDs는 canonical registry와 duplicate insertion을 엄격 검증한다. |
| FIN-F028 | Verified | `Party::acceptQuest`가 ID뿐 아니라 전체 canonical definition이 일치하는 Quest만 수용한다. |
| FIN-F029 | Verified | consumer가 없는 legacy-specific locale key 9개를 5개 catalog에서 제거하고 key parity를 재검증했다. |
| FIN-F030 | Verified | mutable Party/World 전체 invariant를 save 직전에 검증하며 invalid object 위치 저장이 실패한다. |
| FIN-F031 | Verified | primary와 backup이 모두 손상되면 두 candidate를 각각 quarantine한다. |
| FIN-F032 | Verified | global RNG를 소비하는 no-arg `DungeonMap::generate()`를 제거하고 모든 호출자가 seed를 명시한다. |
| FIN-F033 | Verified (Linux) | primary/backup leaf symlink read/write를 모두 거부하고 원본 bytes 불변을 검증했다. Windows reparse-point는 Not Covered다. |
| FIN-F034 | Verified | UI 계약을 실제 clamp와 맞춰 일반 본문 14px, 핵심 상태/선택 16px 이상으로 정리했다. |
| FIN-F035 | Verified | `getShopCatalog()`를 구매 가능한 8종으로 제한하고 Town 입력/렌더링이 같은 registry와 item price를 사용한다. |

## 4. 실행 증거

- `git diff --check` — PASS
- 5 locale JSON parse/key parity — PASS
- `cmake --build build/debug --parallel 2` — PASS
- `ctest --test-dir build/debug --output-on-failure --no-tests=error` — **16/16 PASS**, 141.41초
- `cmake --build build/release --parallel 2` — PASS
- `ctest --test-dir build/release --output-on-failure --no-tests=error` — **16/16 PASS**, 43.01초
- 5 locale × 75/100/200% production UI raster — PASS; CharacterInfo conditions와 Shutdown 화면 대표 raster 직접 판독
- Linux CPack `Crawlmaster-0.10.0-Linux-x86_64.tar.gz` — PASS
- package arbitrary-CWD `--verify-resources` — PASS
- package SHA-256 sidecar — PASS
- Turn 3 `report_integrity.py verify --run-dir docs/multi_audit/3` — PASS, source report 6/6 verified

초기 전체 Debug 실행에서 strict contract가 오래된 TestHarness custom Quest fixture와 Combat spell-slot fixture를 거부했다. fixture를 canonical registry/class formula에 맞춘 뒤 focused test와 전체 Debug/Release를 다시 실행해 통과했다.

## 5. 남은 위험과 제외 범위

- current v0.10 immutable commit이 아직 없고 commit/push 권한도 요청되지 않아 hosted Windows MSVC/package/startup 및 SLSA/SPDX attestation은 실행하지 않았다.
- clean Windows 10/11, Windows reparse-point, 실제 OS high-DPI/IME, 30~60분 3층 장시간 밸런스, power-loss/multi-writer와 legal/support 승인은 `UNVERIFIED` 또는 Human Review 대상이다.
- 위 외부 gate는 로컬 FIN-F001~F035 remediation PASS를 뒤집지 않지만 전체 release PASS는 차단한다.

## 6. Coder claim과 재감사 verdict

- Coder claim: FIN-F001~F035 코드·문서·테스트 수정 완료.
- Re-audit verdict: 로컬에서 재현 가능한 35개 finding은 현재 문서·코드·Debug/Release evidence에 대해 Verified.
- Release verdict: external/current-SHA evidence 부재로 HOLD.

## 7. 후속 인계

current tree를 별도 감사자가 다시 확인할 경우 이 보고서를 coder claim으로만 사용하고, `docs/multi_audit/3/final_audit_report_3.md`의 원 finding ID별 failure matrix를 독립 재실행한다. hosted release를 진행하려면 먼저 승인된 commit/push로 immutable SHA를 만든 뒤 Windows/SLSA artifact subject를 그 SHA와 대조한다.
