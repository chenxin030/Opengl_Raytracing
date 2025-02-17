#version 430
uniform sampler2D uCurrentFrame;
uniform sampler2D uHistory;
uniform float uBlendFactor;
uniform float uJitterX;
uniform float uJitterY;

in vec2 TexCoords;
out vec4 fragColor;

vec3 clipAABB(vec3 color, vec3 minColor, vec3 maxColor) {
    vec3 center = 0.5 * (maxColor + minColor);
    vec3 extents = 0.5 * (maxColor - minColor);
    vec3 clip = color - center;
    clip = clamp(clip, -extents, extents);
    return center + clip;
}

void main() {
    // 当前帧颜色（带抖动偏移）
    vec2 jitteredUV = TexCoords + vec2(uJitterX, uJitterY);
    vec3 current = texture(uCurrentFrame, jitteredUV).rgb;
    
    // 历史颜色
    vec3 history = texture(uHistory, TexCoords).rgb;
    
    // 计算邻域颜色范围（抗重影）
    vec3 minColor = current, maxColor = current;
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            vec3 neighbor = texelFetch(uCurrentFrame, ivec2(gl_FragCoord) + ivec2(x,y), 0).rgb;
            minColor = min(minColor, neighbor);
            maxColor = max(maxColor, neighbor);
        }
    }
    
    // 钳制历史颜色到邻域范围
    history = clipAABB(history, minColor, maxColor);
    
    // 混合当前帧与历史帧
    vec3 result = mix(history, current, uBlendFactor);
    fragColor = vec4(result, 1.0);
}