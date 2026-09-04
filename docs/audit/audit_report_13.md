# Crawlmaster public attestation 후속 재감사 — Re-audit #4

작성일: 2026-09-04 (Asia/Seoul)
입력 감사: `docs/audit/audit_report_12.md`
검증 source SHA: `4f988483bf5cbcfdce4c79a6aabab4a67a7043f9`
Git remote: `git@github.com:Yupkidangju/crawlmaster.git`
Hosted run: [build-test-package #8, attempt 2](https://github.com/Yupkidangju/crawlmaster/actions/runs/33786241695/attempts/2)
감사 기준: `AI_AUDIT_DOC_STANDARD.md`, `spec.md`, `DESIGN_DECISIONS.md`, `audit_roadmap.md`
최종 판정: **HOLD — FIN-F014와 FIN-F015 supply-chain gate는 해소, legal/high-DPI/장시간 사람 evidence 미완료**

## 1. 계보와 범위

- 보고서 12는 private repository에서 native attestation이 발급되지 않았던 당시 증거로 보존하고 수정하지 않았다.
- 사용자가 repository public 전환을 명시 승인하고 직접 완료한 뒤 visibility가 `public`임을 GitHub repository metadata와 공개 페이지에서 확인했다.
- Run `33786241695`의 실패 job을 재실행해 기존 source SHA와 workflow를 변경하지 않은 attempt 2를 만들었다.
- 이번 보고서는 attempt 2의 Ubuntu/Windows 전체 결과, artifact와 attestation URL, 실제 GitHub CLI verification을 재검증한다.
- legal/support 사람 승인, 물리 OS high-DPI와 30~60분/multi-seed 플레이 증거는 별도로 제공되지 않았으므로 PASS로 올리지 않는다.

## 2. Source와 hosted run 결속

- Run attempt: `2`
- Head/source SHA: `4f988483bf5cbcfdce4c79a6aabab4a67a7043f9`
- Ref: `refs/heads/main`
- Repository: `Yupkidangju/crawlmaster`, public
- Ubuntu job: [100873889974](https://github.com/Yupkidangju/crawlmaster/actions/runs/33786241695/job/100873889974)
- Windows job: [100873890184](https://github.com/Yupkidangju/crawlmaster/actions/runs/33786241695/job/100873890184)
- 두 job 결론: `success`

## 3. Hosted gate 결과

| Gate | Ubuntu 24.04 | Windows Server 2022 / MSVC |
| --- | --- | --- |
| Release build | PASS | PASS, MSVC 19.44 x64 `/W4 /WX` |
| CTest | PASS, 13/13 | PASS, 13/13 |
| Install/CPack | PASS, TGZ | PASS, ZIP |
| Package checksum/resource verify | PASS | PASS |
| Arbitrary-CWD startup | PASS, Xvfb 5초 | PASS, native 5초 |
| Custom/Syft SPDX | PASS | PASS |
| Grype high gate | PASS, 0 matches | PASS, 0 matches |
| Artifact upload | PASS | PASS |
| SLSA build provenance | PASS | PASS |
| SPDX SBOM attestation | PASS | PASS |

Workflow 전체 결론은 `success`다.

## 4. Attempt 2 artifact와 직접 검사

### Windows

- Artifact: [Crawlmaster-Windows 9919567279](https://github.com/Yupkidangju/crawlmaster/actions/runs/33786241695/artifacts/9919567279)
- Outer artifact digest: `sha256:e111b13baf5ad8e714ac6281ddb966655f96ec853973d32a6e352ff66b5b7c9f`
- Package: `Crawlmaster-0.9.4-Windows-AMD64.zip`
- Package SHA-256: `41e6997c405472266a48c836db3e160b9f1c104534f9f071e5ea7d2edb81422e`
- Custom SPDX: 18 shipped files / 25 packages
- Syft: 2 packages
- Grype: 0 matches

### Linux

- Artifact: [Crawlmaster-Linux 9919518161](https://github.com/Yupkidangju/crawlmaster/actions/runs/33786241695/artifacts/9919518161)
- Outer artifact digest: `sha256:de4148593761c96170c687a141c0e66759fb9f4eae346d1e3fb324fde1b4f27a`
- Package: `Crawlmaster-0.9.4-Linux-x86_64.tar.gz`
- Package SHA-256: `4e8e5636616140cfac244582f32c4ecb643e1b49529de04f762e794aaf1a8a79`
- Custom SPDX: 18 shipped files / 15 packages
- Syft: 1 package
- Grype: 0 matches

두 outer artifact를 다시 다운로드해 GitHub API digest를 대조했고, 내부 package sidecar도 각각 `OK`였다. GitHub 표시 만료일은 2026-12-03이다.

## 5. Attestation URL

| Platform | Predicate | Verification URL | Subject digest |
| --- | --- | --- | --- |
| Windows | SLSA provenance v1 | [45136689](https://github.com/Yupkidangju/crawlmaster/attestations/45136689) | `sha256:41e6997c405472266a48c836db3e160b9f1c104534f9f071e5ea7d2edb81422e` |
| Windows | SPDX 2.3 SBOM | [45136690](https://github.com/Yupkidangju/crawlmaster/attestations/45136690) | `sha256:41e6997c405472266a48c836db3e160b9f1c104534f9f071e5ea7d2edb81422e` |
| Linux | SLSA provenance v1 | [45136424](https://github.com/Yupkidangju/crawlmaster/attestations/45136424) | `sha256:4e8e5636616140cfac244582f32c4ecb643e1b49529de04f762e794aaf1a8a79` |
| Linux | SPDX 2.3 SBOM | [45136427](https://github.com/Yupkidangju/crawlmaster/attestations/45136427) | `sha256:4e8e5636616140cfac244582f32c4ecb643e1b49529de04f762e794aaf1a8a79` |

각 공개 summary는 commit/source/signer digest `4f988483...`, workflow `.github/workflows/ci.yml@refs/heads/main`, GitHub-hosted runner와 해당 subject digest를 표시한다.

## 6. GitHub CLI 독립 검증

GitHub 공식 release API에서 CLI `2.100.0` Linux amd64 archive와 checksum을 받아 다음 SHA-256을 대조했다.

- checksum file: `6b5916dffcfa6f593b1db7890f2ddc485318e99fa263acf73aa28ebb877b53cd`
- CLI archive: `e4d4bb4498e8d007abe545b6568926793ace1b6447da598294a610018cb164be`

Public REST attestation endpoint에서 package digest별 bundle 두 개를 JSONL로 추출하고 다음 네 명령을 실행했다.

```text
gh attestation verify <windows.zip> -R Yupkidangju/crawlmaster --bundle windows-attestations.jsonl
gh attestation verify <windows.zip> -R Yupkidangju/crawlmaster --bundle windows-attestations.jsonl --predicate-type https://spdx.dev/Document/v2.3
gh attestation verify <linux.tar.gz> -R Yupkidangju/crawlmaster --bundle linux-attestations.jsonl
gh attestation verify <linux.tar.gz> -R Yupkidangju/crawlmaster --bundle linux-attestations.jsonl --predicate-type https://spdx.dev/Document/v2.3
```

결과: 네 명령 모두 exit `0`.

구조화 출력은 다음 predicate/subject를 검증했다.

```text
Windows https://slsa.dev/provenance/v1 sha256:41e6997c...b81422e
Windows https://spdx.dev/Document/v2.3 sha256:41e6997c...b81422e
Linux   https://slsa.dev/provenance/v1 sha256:4e8e5636...f1a8a79
Linux   https://spdx.dev/Document/v2.3 sha256:4e8e5636...f1a8a79
```

인증 없는 GitHub CLI의 기본 API 조회는 login 요구로 exit `4`였으나, 공개 REST에서 받은 signed bundle을 `--bundle`로 검증하는 offline 경로는 성공했다. 이는 attestation의 signature, repository identity, predicate와 package digest 검증 결과다.

## 7. FIN-F014 재판정

**판정: Verified for the requested hosted gate**

- Ubuntu/Windows build, test, package, resource/startup과 artifact가 같은 source SHA에서 성공했다.
- Windows MSVC CTest 13/13과 native 5초 package startup 증거가 보존됐다.
- Clean Windows 10/11에서 VC++ Redistributable이 없는 조건은 여전히 별도 소비자 runtime 위험이다.

## 8. FIN-F015 재판정

**판정: Supply-chain Verified / Product legal Human Review Required**

해소:

- complete artifact SPDX, independent Syft, Grype high gate
- SLSA build provenance와 SPDX 2.3 SBOM attestation 실제 발급
- 공개 verification URL 네 개
- GitHub CLI package/bundle 검증 네 건

미해소:

- 제품 copyright/legal owner 사람 승인
- 배포 LICENSE/EULA/privacy 결정
- 공개 support contact와 최종 유료 배포 승인

기술적 attestation subgate는 해소됐지만 법률/지원 주체는 기술 증거로 대체할 수 없다.

## 9. 전체 finding ledger

| Finding | Re-audit #3 | Re-audit #4 | Gate |
| --- | --- | --- | --- |
| FIN-F001~F013 | Verified 또는 demo-scope Verified | **Inherited; hosted regression PASS** | 해소 유지, F002 장시간 별도 |
| FIN-F014 | Hosted gate Verified | **Verified** | clean consumer runtime 잔여 |
| FIN-F015 | Attestation/legal 미완료 | **Supply-chain Verified / Legal Human Review Required** | legal PASS 차단 |
| FIN-F016 | Verified | **Inherited Verified** | 해소 유지 |
| FIN-F009 external evidence | OS high-DPI 미완료 | **Not Covered** | 사람 evidence 차단 |

## 10. 최종 판정

**HOLD**

- Critical open: 0
- Local code/document Major Needs Fix: 0
- Hosted Ubuntu/Windows와 supply-chain attestation: PASS
- Product legal/support owner: Human Review Required
- 실제 OS high-DPI: Not Covered
- Clean-save 30~60분 완주와 multi-seed 정량 balance: Not Covered
- Clean Windows 10/11 VC++ runtime 조건: Unverified residual

Repository 공개와 native attestation으로 FIN-F015의 공급망 부분은 해소됐다. 그러나 사용자가 요구한 사람 legal/support 승인과 실제 OS/high-DPI/장시간 플레이 evidence가 아직 없으므로 상용 PASS 또는 유료 배포 승인은 불가하다.

## 11. 다음 재감사 입력

1. 승인자 실명/역할/날짜, copyright owner, LICENSE/EULA/privacy 결정, support contact와 유료 배포 승인.
2. 목표 OS/GPU/해상도/150~200% scaling에서 5 locale 핵심 화면 판독 및 캡처.
3. Attempt 2 package SHA에 결속한 clean-save 30~60분 multi-seed 완주 기록.
4. VC++ Redistributable 전제 또는 bundling 정책과 clean Windows 10/11 startup 결과.

위 사람/실기 증거가 확보된 뒤 다음 번호 보고서에서만 PASS를 재판정한다.

## 12. Handoff

```text
`/mnt/Projects_SSD/cpp/crawlmaster/docs/audit/audit_report_13.md`를 먼저 읽고, legal/support owner 승인과 OS high-DPI·30~60분 multi-seed playtest를 attempt 2 package SHA에 결속해 기록하세요. Windows VC++ runtime 배포 정책과 clean Windows 10/11 startup도 확인한 뒤 다음 번호 보고서에서만 상용 PASS를 재판정하세요.
```
