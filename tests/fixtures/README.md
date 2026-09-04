# 테스트 fixture

- `save_v1.json`: `schemaVersion`이 없고 snake_case 키를 사용하는 v1 migration 입력이다. 현재 제품 기본 저장이나 package 자산이 아니다.
- 테스트는 fixture를 임시 디렉터리에 복사해 읽어야 하며 이 디렉터리의 원본을 덮어쓰지 않는다.
- canonical 제품 저장 형식은 `spec.md`의 schema v4 예시와 `Party::saveToFile()` 출력이다.
