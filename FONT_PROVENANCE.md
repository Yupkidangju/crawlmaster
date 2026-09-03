# 번들 폰트 provenance 및 재배포 증거

작성일: 2026-09-03 (Asia/Seoul)  
범위: Crawlmaster 0.9.4 pre-release demo candidate의 `assets/fonts/`

이 문서는 파일 동일성, upstream, 라이선스와 raster 증거를 기록한다. 법률 자문이나 제품 소유자의 최종 배포 승인을 대신하지 않는다.

## 1. 현재 번들

| 파일 | SHA-256 | 원출처/버전 | 라이선스 | 동일성 증거 |
| --- | --- | --- | --- | --- |
| `NotoSansCJK-Regular.ttc` | `b76b0433203017ca80401b2ee0dd69350349871c4b19d504c34dbdd80541690a` | Ubuntu `fonts-noto-cjk` `1:20240730+repack1-1build1`, upstream `notofonts/noto-cjk` | SIL OFL 1.1 | 설치 패키지의 `/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc`와 byte-identical |
| `UbuntuMono[wght].ttf` | `fbf1e748836994f730e602f7dcf2525564d6d78aa336080cbb73af909d0e08ee` | Ubuntu `fonts-ubuntu` `0.869+git20240321-0ubuntu2` | Ubuntu Font Licence 1.0 | 설치 패키지의 동일 파일과 byte-identical; 내부 family `Ubuntu Mono` |
| `neodgm.ttf` | `77305267996073aae07bad9313dad2e306a4128e55bfafbed4c41558fee57b4d` | 공식 `neodgm/neodgm` release `v1.601`, commit `cbe9a3c6b7067c62612570ee1261b39a31e5061b` | SIL OFL 1.1 | 공식 release asset을 streaming hash한 결과와 byte-identical |

배포 archive 메타데이터:

- `fonts-noto-cjk_20240730+repack1-1build1_all.deb`: SHA-256 `be378a571b8f49b4b9146f7d13b31990950d35ee3e0bf1999f554c3ed5670dc0`
- `fonts-droid-fallback_8.1.0r7-1~1.gbp36536bbuild1_all.deb`: 이전 Droid 파일의 조사 출처이며 현재 package에는 Droid font를 포함하지 않는다.
- NeoDunggeunmo release date: 2026-03-18 UTC.

## 2. 라이선스 원문

- `licenses/Noto-SIL-OFL-1.1.txt`
- `licenses/Ubuntu-Font-Licence-1.0.txt`
- `licenses/NeoDunggeunmo-SIL-OFL-1.1.txt`

공식 근거:

- Noto CJK upstream: https://github.com/notofonts/noto-cjk
- Ubuntu Font Licence: https://canonical.com/legal/font-licence
- NeoDunggeunmo upstream/release: https://github.com/neodgm/neodgm/releases/tag/v1.601

## 3. 교체 근거와 raster 검증

이전 `DroidSansFallbackFull.ttf`는 로드와 codepoint advance 검사는 통과했지만 실제 혼합 일본어/중국어+ASCII 렌더에서 다수 문자가 tofu 사각형으로 표시됐다. 해당 파일은 source asset에서 제거하고 작업 복구용으로만 `build/remediation-artifacts/font-replaced/`에 보존했다.

`FontRasterTests`는 현재 실제 SFML/FreeType 경로에서 다음을 수행한다.

- KO/EN: NeoDunggeunmo, JA/ZH-TW/ZH-CN: Noto Sans CJK 선택
- 5 locale × 75/100/200% = 15 PNG 생성
- 실제 UI/content/log sample의 모든 비공백 glyph가 missing-glyph texture와 다른지 확인
- 생성 PNG가 비어 있지 않은지 확인

증거 디렉터리: `build/final-debug/font-raster-evidence/`.

## 4. 재현 명령

```bash
sha256sum assets/fonts/*
fc-scan assets/fonts/*
cmake --build build/final-debug --target FontRasterTests --parallel 2
xvfb-run -a ./build/final-debug/FontRasterTests build/final-debug/font-raster-evidence
```

## 5. 남은 human gate

- 제품 저작권자와 legal/support contact
- LICENSE/EULA/privacy/support 정책의 적용 범위
- 최종 유료 배포 승인과 상표 검토

폰트 파일과 라이선스의 기술적 provenance는 현재 증거로 닫지만, 위 제품 주체 결정은 `Human Review Required`다.
