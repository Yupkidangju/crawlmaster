# Crawlmaster 외부 릴리스 gate 재감사 — Re-audit #3

작성일: 2026-09-04 (Asia/Seoul)
입력 감사: `docs/audit/audit_report_11.md`
보완 계획: `docs/audit/remediation_plan_12.md`
검증 source SHA: `4f988483bf5cbcfdce4c79a6aabab4a67a7043f9`
Git remote: `git@github.com:Yupkidangju/crawlmaster.git`
Hosted run: [build-test-package #8 / run 33786241695](https://github.com/Yupkidangju/crawlmaster/actions/runs/33786241695)
감사 기준: `AI_AUDIT_DOC_STANDARD.md`, `spec.md`, `DESIGN_DECISIONS.md`, `audit_roadmap.md`
최종 판정: **HOLD — FIN-F014 hosted gate는 해소됐으나 FIN-F015 attestation/legal과 사람 실행 evidence가 미완료**

## 1. 감사 성격과 범위

- 보고서 11과 이전 번호 보고서는 수정하지 않았다.
- 이번 재감사는 보고서 11의 미완료 외부 gate 중 remote/source 결속, hosted Ubuntu/Windows build·test·package·startup, artifact/SBOM/scanner와 attestation 실제 발급 여부를 직접 확인했다.
- Windows MSVC 실패를 감추거나 `/WX`를 낮추지 않고 hosted run의 최초 오류를 원인별로 보완했다. 변경과 재현 계보는 `remediation_plan_12.md`에 기록했다.
- 제품 저작권자, EULA/privacy/support contact 승인, 실제 OS high-DPI 판독과 clean-save 30~60분 완주는 사람이 제공한 증거가 없어 완료로 간주하지 않았다.
- 이번 판정은 Windows Server 2022 GitHub-hosted runner의 자동 실행 증거다. Windows 10/11 clean consumer PC에 VC++ runtime이 없는 조건과 물리 모니터 high-DPI를 대신하지 않는다.

## 2. Remote와 source SHA 결속

```text
git rev-parse HEAD
git ls-remote origin refs/heads/main
```

두 명령은 hosted run 시작 시점에 모두 다음 SHA를 반환했다.

```text
4f988483bf5cbcfdce4c79a6aabab4a67a7043f9
```

Run 33786241695의 두 job도 checkout log에서 같은 SHA를 확인했다.

- Ubuntu job: [100751603417](https://github.com/Yupkidangju/crawlmaster/actions/runs/33786241695/job/100751603417)
- Windows job: [100751603117](https://github.com/Yupkidangju/crawlmaster/actions/runs/33786241695/job/100751603117)
- Windows toolchain: Windows Server 2022, Visual Studio 2022, MSVC 19.44.35228.0, x64
- Linux toolchain: Ubuntu 24.04, GNU 13.3.0, x86_64

## 3. Hosted 명령 증거

| Gate | Ubuntu 24.04 | Windows Server 2022 / MSVC |
| --- | --- | --- |
| Release configure/build | PASS | PASS, `/W4 /WX` |
| CTest | PASS, 13/13 | PASS, 13/13 |
| Install/CPack | PASS, TGZ | PASS, ZIP |
| SHA-256 sidecar | PASS | PASS |
| Packaged resource verify | PASS | PASS |
| Arbitrary-CWD startup | PASS, Xvfb 5초 생존 | PASS, native process 5초 생존 |
| Custom artifact SPDX | PASS | PASS |
| Independent Syft SPDX | PASS | PASS |
| Grype high gate | PASS, 0/15 package matches | PASS, 0/25 package matches |
| Artifact upload | PASS | PASS |
| GitHub native build attestation | FAIL: account/repository plan gate | FAIL: account/repository plan gate |
| GitHub native SBOM attestation | SKIPPED after build attestation failure | SKIPPED after build attestation failure |

전체 workflow 결론이 `failure`인 이유는 test/package/startup 회귀가 아니라 마지막 native attestation persistence 단계다.

## 4. Artifact 직접 재검증

### Hosted artifact

- Windows: [Crawlmaster-Windows artifact 9905938125](https://github.com/Yupkidangju/crawlmaster/actions/runs/33786241695/artifacts/9905938125)
  - GitHub artifact digest: `sha256:23fca56ecf442571795680d59cb4e9a62cd0af9af2dd8af40d50e7ae82f8afa0`
  - size: 16,382,840 bytes
- Linux: [Crawlmaster-Linux artifact 9905751081](https://github.com/Yupkidangju/crawlmaster/actions/runs/33786241695/artifacts/9905751081)
  - GitHub artifact digest: `sha256:148982d4b7c7a36e3f720410ba5fa2de4a1e890e56e09c08825624bd2b293ce8`
  - size: 16,535,918 bytes
- 둘 다 source SHA `4f988483bf5cbcfdce4c79a6aabab4a67a7043f9`에 결속되며 GitHub 표시 만료일은 2026-12-02다.

### Package와 SBOM

```text
sha256sum Crawlmaster-Windows-run33786241695.zip
sha256sum Crawlmaster-Linux-run33786241695.zip
unzip -q <outer-artifact> -d <temporary-directory>
sha256sum -c Crawlmaster-0.9.4-Windows-AMD64.zip.sha256
sha256sum -c Crawlmaster-0.9.4-Linux-x86_64.tar.gz.sha256
unzip -l Crawlmaster-0.9.4-Windows-AMD64.zip
tar -tzf Crawlmaster-0.9.4-Linux-x86_64.tar.gz
```

결과:

- outer artifact digest는 GitHub API digest와 양쪽 모두 일치했다.
- Windows package: `Crawlmaster-0.9.4-Windows-AMD64.zip`
  - SHA-256: `5b0d17bfb2b288962ece53b7c651853e154a692583b221ea0b450f83047334dc`
  - ZIP sidecar: PASS
  - executable, 5 locale, 3 font, README/notices/provenance와 5 license 원문 포함
- Linux package: `Crawlmaster-0.9.4-Linux-x86_64.tar.gz`
  - SHA-256: `39cf576f029ee02f4723554825563087883771b1f6bd64ed577d9938186bebc4`
  - TGZ sidecar: PASS
- Custom SPDX: 양쪽 18 shipped files. Windows 25 packages, Linux 15 packages.
- Syft inventory: Windows 2 packages, Linux 1 package.
- Grype JSON: 양쪽 `matches=[]`.
- Windows PE runtime imports에는 `MSVCP140.dll`, `VCRUNTIME140.dll`, `VCRUNTIME140_1.dll`이 포함되지만 package에는 번들되지 않았다. Hosted image에는 runtime이 있어 startup은 통과했다. 따라서 clean Windows 10/11 consumer 환경에서는 Microsoft VC++ Redistributable 전제 또는 runtime bundling 정책을 별도 확정해야 한다.

## 5. FIN-F014 재판정

**판정: Verified for the requested hosted gate / clean consumer runtime residual**

- 같은 remote SHA에서 Ubuntu와 Windows hosted build가 수행됐다.
- Windows는 우연히 선택된 MinGW가 아니라 MSVC 19.44 x64로 `/W4 /WX` 전체 target을 빌드했다.
- Windows CTest 13/13, install, CPack ZIP, sidecar, packaged asset verify와 arbitrary-CWD native 5초 startup이 모두 성공했다.
- Windows package/SBOM/scanner artifact URL과 digest를 보존했다.
- 다만 hosted Windows Server 2022의 5초 smoke는 clean Windows 10/11 장시간 실기와 VC++ Redistributable 부재 조건의 증거가 아니다. 이 잔여 위험은 정식 Windows 지원 선언 전에 닫아야 한다.

## 6. FIN-F015 재판정

**판정: Partially Verified / Human Review Required — PASS 차단**

확보된 증거:

- package checksum, complete artifact SPDX, independent Syft inventory와 Grype high gate
- bundled font의 파일별 provenance/license 원문
- 두 hosted artifact의 source SHA와 digest

발급되지 않은 증거:

- GitHub native build provenance attestation
- GitHub native SPDX SBOM attestation
- `gh attestation verify` 성공 결과와 attestation summary URL
- 제품 copyright owner, LICENSE/EULA/privacy 처리, support contact와 배포 승인에 대한 사람 sign-off

두 job의 실제 오류는 동일하다.

```text
Failed to persist attestation: Feature not available for user-owned private repositories.
To enable this feature, please make this repository public.
```

GitHub 공식 문서는 Free/Pro/Team의 private/internal repository에서 artifact attestation을 사용할 수 없고 GitHub Enterprise Cloud가 필요하다고 명시한다. 현재 저장소는 user-owned private repository이므로 `actions/attest`가 bundle을 저장하지 못했고 검증 URL도 생성되지 않았다.

- 공식 정책: https://docs.github.com/en/actions/how-tos/secure-your-work/use-artifact-attestations/use-artifact-attestations
- 사용 중인 action 계약: https://github.com/actions/attest

해결 경로는 다음 중 하나를 사람/소유자가 선택해야 한다.

1. repository를 public으로 전환한 뒤 동일 source에서 `actions/attest`와 `gh attestation verify`를 재실행한다.
2. GitHub Enterprise Cloud private repository로 이전/승격한다.
3. private repository를 유지하고 Sigstore public-good keyless bundle을 별도로 발급한다. 이 경우 repository/workflow identity와 artifact digest가 공개 transparency log에 기록되는 점을 승인하고, 기존 `gh attestation verify` 계약을 `cosign verify-blob-attestation` 또는 검증 가능한 동등 계약으로 먼저 문서화해야 한다.

## 7. 사람 실행 evidence

| Evidence | Current state | PASS에 필요한 최소 증거 |
| --- | --- | --- |
| Product legal/support owner | Not provided | 승인자 실명/역할/날짜, copyright owner, 배포 LICENSE/EULA/privacy 결정, 공개 support contact, 유료 배포 승인 |
| OS high-DPI | Not provided | 목표 OS·GPU·배율(예: 150/200%)·해상도, 5 locale 핵심 화면 캡처, clipping/tofu/focus 판독 기록 |
| Clean-save 30~60분 완주 | Not provided | package SHA, OS, seed, 시작/종료 시각, Victory/TPK, 저장/재실행 결과와 blocker 기록 |
| Multi-seed balance | Not provided | 여러 seed의 완주율, 시간, encounter/소모품/TPK 수치와 허용 기준 |

Hosted Windows의 1024×768 production raster와 5초 startup은 자동 회귀 증거로 유효하지만 실제 OS scaling/high-DPI 또는 장시간 플레이를 대체하지 않는다.

## 8. 전체 finding ledger

| Finding | Report 11 | Re-audit #3 | Gate |
| --- | --- | --- | --- |
| FIN-F001 | Verified | **Inherited Verified; 13/13 regression PASS** | 해소 유지 |
| FIN-F002 | Verified within demo contract | **Inherited; long-play evidence absent** | 사람 evidence 잔존 |
| FIN-F003 | Verified | **Inherited Verified** | 해소 유지 |
| FIN-F004 | Verified | **Inherited Verified** | 해소 유지 |
| FIN-F005 | Verified | **Inherited Verified; hosted regression PASS** | 해소 유지 |
| FIN-F006 | Verified | **Inherited Verified; process replay PASS** | 해소 유지 |
| FIN-F007 | Verified | **Inherited Verified; Windows persistence regression PASS** | 해소 유지 |
| FIN-F008 | Verified | **Inherited Verified** | 해소 유지 |
| FIN-F009 | Verified within Linux demo contract | **Hosted Windows raster PASS; OS high-DPI still unverified** | 사람 evidence 잔존 |
| FIN-F010 | Verified | **Inherited Verified** | 해소 유지 |
| FIN-F011 | Verified locally | **Hosted Linux/Windows regression PASS** | 해소 유지 |
| FIN-F012 | Verified | **Inherited Verified** | 해소 유지 |
| FIN-F013 | Verified | **Hosted Release build/CTest PASS** | 해소 유지 |
| FIN-F014 | Partially Verified | **Verified for requested hosted gate** | clean consumer runtime residual |
| FIN-F015 | Partially Verified / Human Review Required | **Partially Verified / Human Review Required** | attestation·legal PASS 차단 |
| FIN-F016 | Verified | **Inherited Verified** | 해소 유지 |

## 9. Coverage gap과 최종 판정

- Remote/SHA/hosted Ubuntu: Covered
- Hosted Windows MSVC build/test/package/startup: Covered
- Artifact/SBOM/scanner와 독립 다운로드 검증: Covered
- Signed build/SBOM attestation: Not Covered; plan/repository capability blocker
- Product legal/support approval: Not Covered; human decision required
- OS high-DPI: Not Covered; physical/interactive evidence required
- 30~60분 완주와 multi-seed 정량 balance: Not Covered; human playtest required

**최종 판정: HOLD**

- Critical open: 0
- Local code/document Major Needs Fix: 0
- FIN-F014 requested hosted gate: 해소
- External Major blocker: FIN-F015 attestation와 legal/support human sign-off
- Product evidence blocker: 실제 OS high-DPI와 30~60분/multi-seed playtest

따라서 현재 산출물은 같은 SHA의 Linux/Windows hosted demo artifact로 재현 가능하지만, 상용 PASS 또는 유료 배포 승인 상태는 아니다.

## 10. 다음 재감사 입력

1. attestation 경로(public, Enterprise Cloud private, 승인된 Sigstore public transparency)를 확정하고 build/SBOM bundle 및 실제 검증 URL/명령 결과를 제공한다.
2. legal/support sign-off 문서를 승인자·날짜와 함께 제공한다.
3. OS high-DPI 캡처/판독표와 clean-save 30~60분 multi-seed playtest 기록을 제공한다.
4. Windows clean consumer runtime 정책(VC++ Redistributable 전제 또는 bundling)을 확정하고 해당 환경 startup을 재검증한다.

이 네 증거가 같은 승인 package/source에 결속된 뒤 다음 번호 보고서에서만 PASS를 재판정한다.

## 11. Coder handoff

```text
`/mnt/Projects_SSD/cpp/crawlmaster/docs/audit/audit_report_12.md`를 먼저 읽고 FIN-F015 attestation 경로와 Windows VC++ runtime 배포 정책을 제품 문서에서 확정하세요. 사람 승인 없이 repository 공개 전환이나 Sigstore transparency log 기록을 수행하지 말고, 승인된 경로에서 build/SBOM 검증 URL을 보존하세요. legal/support sign-off와 OS high-DPI·30~60분 multi-seed playtest 증거를 같은 package SHA에 결속한 뒤 다음 번호 보고서에서만 PASS를 재판정하세요.
```
