# Third-Party Notices and Provenance Gate

작성일: 2026-09-03  
범위: Crawlmaster 0.9.4 pre-release demo candidate

## 코드 의존성

| 구성요소 | 버전/immutable revision | 라이선스 | 근거 |
| --- | --- | --- | --- |
| SFML | 2.6.1 / `69ea0cd863aed1d4092b970b676924a716ff718b` | zlib/png | FetchContent source의 `license.md` |
| nlohmann/json | 3.11.3 / `9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03` | MIT | FetchContent source의 `LICENSE.MIT` |

SFML과 nlohmann/json의 라이선스 원문은 package의 `share/doc/crawlmaster/licenses/`에 포함한다. `DEPENDENCY_MANIFEST.spdx.json`은 source component manifest이며 `scripts/generate_release_sbom.py`와 CI가 artifact-specific SPDX/Grype 증거를 만든다. 이 문서는 법률 자문을 대신하지 않는다.

## 번들 폰트

| 파일 | SHA-256 | 내부 family | 현재 판정 |
| --- | --- | --- | --- |
| `NotoSansCJK-Regular.ttc` | `b76b0433203017ca80401b2ee0dd69350349871c4b19d504c34dbdd80541690a` | Ubuntu fonts-noto-cjk / notofonts/noto-cjk | SIL OFL 1.1 |
| `UbuntuMono[wght].ttf` | `fbf1e748836994f730e602f7dcf2525564d6d78aa336080cbb73af909d0e08ee` | Ubuntu fonts-ubuntu / Canonical | Ubuntu Font Licence 1.0 |
| `neodgm.ttf` | `77305267996073aae07bad9313dad2e306a4128e55bfafbed4c41558fee57b4d` | neodgm/neodgm v1.601 | SIL OFL 1.1 |

파일별 hash, upstream/version, license 원문과 실제 CJK raster 증거는 `FONT_PROVENANCE.md`에 기록한다. 현재 번들은 원본과 byte-identical이며 변환하지 않았다.

## 제품 법률/지원 주체

제품 저작권자, EULA, privacy/support contact는 아직 정해지지 않았다. Title 화면은 소유권을 창작해 주장하지 않고 pre-release 상태만 표시한다. 폰트 기술 provenance와 별개로 이 항목은 `Human Review Required`이며 상용 PASS를 차단한다.
