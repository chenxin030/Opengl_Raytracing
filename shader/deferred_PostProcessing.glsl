#version 430 core

uniform sampler2D lightingResult; // ���ս׶����
uniform sampler2D bloomBlur;      // Bloomģ�����
uniform float bloomStrength = 0.5;

// TAA���
uniform sampler2D historyTexture;
uniform vec2 jitterOffset;
uniform float blendFactor;

out vec4 FragColor;

vec3 clipAABB(vec3 color, vec3 minColor, vec3 maxColor) {
    vec3 center = 0.5 * (maxColor + minColor);
    vec3 extents = 0.5 * (maxColor - minColor);
    vec3 clip = color - center;
    clip = clamp(clip, -extents, extents);
    return center + clip;
}

void main() {
    vec2 uv = gl_FragCoord.xy / textureSize(lightingResult, 0);
    
    // Bloom���
    vec3 color = texture(lightingResult, uv).rgb;
    vec3 bloom = texture(bloomBlur, uv).rgb;
    color += bloom * bloomStrength;
    
    // TAA����
    vec3 history = texture(historyTexture, uv + jitterOffset).rgb;
    vec3 current = color;
    
    // ��ɫ�ü�����
    vec3 minColor = current, maxColor = current;
    for(int i = -1; i <= 1; ++i) {
        for(int j = -1; j <= 1; ++j) {
            vec3 c = texelFetch(lightingResult, ivec2(gl_FragCoord) + ivec2(i,j), 0).rgb;
            minColor = min(minColor, c);
            maxColor = max(maxColor, c);
        }
    }
    history = clipAABB(history, minColor, maxColor);
    color = mix(history, current, blendFactor);
    
    FragColor = vec4(color, 1.0);
}