#version 120
// Crawlmaster v1.0.0 3D 던전 PBR-풍 노멀맵 + 다중 동적 횃불 + 바이옴 안개 셰이더

uniform sampler2D u_albedo;
uniform sampler2D u_normal;
uniform vec3 u_fogColor;
uniform vec3 u_torchColor;
uniform vec3 u_ambientColor;
uniform float u_torchIntensity;
uniform float u_time;
uniform float u_wetness;
uniform float u_emissiveBoost;

varying vec2 v_texCoord;
varying vec4 v_color;
varying vec2 v_screenPos;

void main() {
    vec4 albedo = texture2D(u_albedo, v_texCoord);
    if (albedo.a < 0.05) discard;

    // 탄젠트 공간 노멀맵 샘플링
    vec3 n = texture2D(u_normal, v_texCoord).rgb * 2.0 - 1.0;
    n = normalize(vec3(n.x * 1.15, n.y * 1.15, max(0.25, n.z)));

    // v_color.r = 깊이 기반 광원 감쇠(0=원거리, 1=근거리), v_color.g = 방향성 음영, v_color.b = 안개 계수
    float lightAtten = v_color.r;
    float dirShade = mix(0.55, 1.0, v_color.g);
    float fogFactor = v_color.b;

    // 횃불 흔들림에 따른 동적 입사각
    float swayX = sin(u_time * 3.1) * 0.09;
    float swayY = cos(u_time * 4.3) * 0.06;
    vec3 lightDir = normalize(vec3(-v_screenPos.x * 0.45 + swayX, -v_screenPos.y * 0.35 + 0.2 + swayY, 0.85));
    vec3 viewDir = normalize(vec3(-v_screenPos.x * 0.25, -v_screenPos.y * 0.25, 1.0));
    vec3 halfVec = normalize(lightDir + viewDir);

    float ndotl = max(dot(n, lightDir), 0.12);
    float specPow = mix(14.0, 44.0, u_wetness);
    float spec = pow(max(dot(n, halfVec), 0.0), specPow) * mix(0.16, 0.55, u_wetness);

    vec3 lit = albedo.rgb * (u_ambientColor * dirShade + u_torchColor * (ndotl * lightAtten * u_torchIntensity * dirShade));
    lit += u_torchColor * (spec * lightAtten * u_torchIntensity);

    // 용암/발광 룬 등 자체 발광 피크 부스트
    float warmMask = clamp((albedo.r - albedo.b) * 1.6 - 0.35, 0.0, 1.0);
    lit += albedo.rgb * warmMask * u_emissiveBoost;

    // 바이옴 지수 안개 합성
    vec3 finalRgb = mix(u_fogColor, lit, fogFactor);
    gl_FragColor = vec4(finalRgb, albedo.a * v_color.a);
}
