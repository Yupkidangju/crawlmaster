# D3D 수정 개시 보고서 — v0.9.3 테스트 자산 동기화

- **검토 일자:** 2026-07-13 (Asia/Seoul)
- **검토 대상:** `audit_report_6.md`
- **검토 결과:** 최종 PASS 판정의 실행 재현성 결함 1건 확인
- **수정 상태:** 1차 수정 완료 및 회귀 검증 완료

## 1. 확인된 문제

`audit_report_6.md`는 5개 언어 글리프 검증이 통과했다고 기록했지만, `BUILD_GUIDE.md`에 안내된 것처럼 `build/` 디렉터리에서 `./TestHarness --run-all`을 실행하면 빌드 출력 디렉터리의 오래된 `assets/lang/ja.json`을 읽을 수 있었다. 그 결과 일본어 `COMBAT_DEFEAT`의 U+2026 글리프 검증이 실패했다.

원인은 번들 자산 복사가 `Crawlmaster` 타겟의 빌드 후처리에만 연결되어 `TestHarness` 실행 자산과 최신 소스 자산의 동기화가 보장되지 않았던 것이다.

## 2. 적용한 수정

1. `CMakeLists.txt`에 공용 `Assets` 커스텀 타겟을 추가했다.
2. `Crawlmaster`와 `TestHarness`가 모두 `Assets` 타겟에 의존하도록 연결했다.
3. 매 빌드마다 번역 JSON 및 폰트 자산을 `build/assets/`로 복사하도록 변경했다.
4. `BUILD_GUIDE.md`, `README.md`, `CHANGELOG.md`, `audit_roadmap.md`를 v0.9.3 기준으로 동기화했다.
5. 테스트 실행으로 변경된 `save.json`은 기본 세이브 상태로 복원했다.

## 3. 검증 결과

다음 명령을 실행했다.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j2
(cd build && ./TestHarness --run-all)
```

결과:

- 빌드 성공
- 전체 13개 테스트 시나리오 통과
- Town/Combat 다국어 글리프 회귀 테스트 통과
- `build/assets/`에 최신 폰트 및 번역 자산 복사 확인

## 4. 후속 확인 사항

이번 수정으로 감사 보고서의 글리프 검증 결과를 빌드 디렉터리 직접 실행에서도 재현할 수 있게 되었다. 테스트 하네스가 공유 `save.json`을 변경하는 기존 테스트 격리 문제는 이번 수정 범위를 벗어나므로 별도 개선 대상으로 남긴다.
