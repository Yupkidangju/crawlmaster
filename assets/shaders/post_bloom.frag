#version 120
// Crawlmaster v1.0.0 포스트프로세싱 셰이더 (아모스페릭 블룸 + 색보정 + 비네트 + 피격 플래시)

uniform sampler2D u_scene;
uniform vec2 u_texelSize;
uniform vec3 u_tint;
uniform float u_vignetteStrength;
uniform float u_bloomStrength;
uniform float u_damageFlash;
uniform float u_time;

void main() {
    vec2 uv = gl_TexCoord[0].xy;
    vec4 base = texture2D(u_scene, uv);

    // 9-탭 경량 소프트 블룸(밝은 횃불·마법·용암 피크만 추출)
    vec3 bloom = vec3(0.0);
    vec2 offs = u_texelSize * 2.5;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec3 s = texture2D(u_scene, uv + vec2(float(x), float(y)) * offs).rgb;
            float luma = dot(s, vec3(0.299, 0.587, 0.114));
            bloom += s * max(0.0, luma - 0.58);
        }
    }
    bloom /= 9.0;

    vec3 color = base.rgb + bloom * u_bloomStrength;

    // 다크 판타지 필름 톤매핑 & 바이옴 미세 색보정
    color *= u_tint;
    color = color / (color + vec3(0.75)) * 1.48;
    color = clamp(color, 0.0, 1.0);

    // 비네트
    vec2 centered = uv - vec2(0.5);
    float dist = length(centered * vec2(1.15, 1.0));
    float vig = 1.0 - smoothstep(0.36, 0.86, dist) * u_vignetteStrength;
    color *= vig;

    // 미세 필름 그레인 (과하지 않은 아날로그 회화 질감)
    float grain = fract(sin(dot(uv * 512.0 + u_time * 13.0, vec2(12.9898, 78.233))) * 43758.5453);
    color += (grain - 0.5) * 0.015;

    // 피격 시 가장자리 적색 플래시
    if (u_damageFlash > 0.001) {
        float edge = smoothstep(0.22, 0.78, dist);
        color = mix(color, vec3(0.78, 0.08, 0.08), u_damageFlash * (0.28 + 0.55 * edge));
    }

    gl_FragColor = vec4(clamp(color, 0.0, 1.0), base.a);
}
