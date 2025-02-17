#version 430 core

uniform sampler2D lightingResult; // 光照阶段输出
uniform sampler2D bloomBlur;      // Bloom模糊结果
uniform float bloomStrength = 0.5;

// TAA相关
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
    
    // Bloom混合
    vec3 color = texture(lightingResult, uv).rgb;
    vec3 bloom = texture(bloomBlur, uv).rgb;
    color += bloom * bloomStrength;
    
    // TAA处理
    vec3 history = texture(historyTexture, uv + jitterOffset).rgb;
    vec3 current = color;
    
    // 颜色裁剪与混合
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