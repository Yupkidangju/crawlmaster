# D3D 수정 개시 및 1차 검증 보고서 — v0.9.4

- **감사 기준:** `audit_report_8.md`의 REWORK REQUIRED 항목
- **검토 일자:** 2026-07-13 (Asia/Seoul)
- **수정 상태:** 1차 수정 완료, 재감사 전 단계

## 1. 반영한 수정

1. `Game::loadResources()`가 혼합 CJK/ASCII UI를 지원하는 시스템 Noto Sans CJK를 우선 탐색하도록 수정했다. 시스템 폰트가 없을 때 기존 번들 및 Droid 경로를 순차 폴백한다.
2. `TownState`, `TitleState`, `CombatState`의 지속 `sf::Text`가 draw/update 시점의 현재 게임 폰트를 다시 참조하도록 수정했다.
3. v0.9.4 버전을 README와 감사 로드맵에 반영하고 변경 이력을 기록했다.
4. `audit_report_8.md`에서 이미 반영된 Town 허브 9개 키와 TestHarness 임시 저장 경로 격리를 회귀 기준으로 확인했다.

## 2. 검증 결과

- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`: 통과
- `cmake --build build -j2`: 통과
- `(cd build && ./TestHarness --run-all)`: 전체 시나리오 통과
- Town 허브 9개 키의 5개 언어 리소스 존재 검사: 통과
- TestHarness 세이브 경로: `/tmp/crawlmaster-test-*`로 격리

## 3. 남은 항목

- 실제 Xvfb 5개 언어 화면 캡처와 혼합 ASCII/CJK 판독 검증은 다음 재감사에서 수행해야 한다.
- `DroidSansFallbackFull.ttf`의 canonical source, checksum, license, NOTICE 및 재배포 허가는 Human Review Required로 남아 있다.
- Town/Combat의 모든 사용자-facing 하드코딩 문자열을 JSON 키로 이관하는 작업은 아직 완료되지 않았다.

## 4. 판정

이번 보고서는 수정 개시와 1차 자동 검증을 기록한다. `audit_report_8.md`의 전체 REWORK REQUIRED를 PASS로 변경하지 않으며, 남은 시각 검증·문서화·폰트 provenance 확인 후 Re-audit #4를 수행한다.
