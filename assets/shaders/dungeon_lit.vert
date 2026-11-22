#version 120
// Crawlmaster v1.0.0 3D 던전 조명 셰이더 (Vertex)
// SFML 2.6 OpenGL 호환성 컨텍스트(GLSL 120+)에서 동작하며 Linux/Windows 모두 지원한다.

varying vec2 v_texCoord;
varying vec4 v_color;
varying vec2 v_screenPos;

void main() {
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    v_texCoord = (gl_TextureMatrix[0] * gl_MultiTexCoord0).xy;
    v_color = gl_Color;
    v_screenPos = gl_Position.xy;
}
