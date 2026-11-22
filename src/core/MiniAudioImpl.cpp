// miniaudio 단일 헤더 구현 번역 단위.
// CMake에서 이 파일에만 외부 C 라이브러리 경고 억제를 적용해 프로젝트 C++20 코드는 -Werror / /WX를 유지한다.
#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_ENCODING
#define MA_NO_FLAC
#define MA_NO_MP3
#include <miniaudio.h>
