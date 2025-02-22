#version 430
uniform sampler2D uCurrentFrame;
uniform sampler2D uHistory;
uniform float uBlendFactor;
uniform float uJitterX;
uniform float uJitterY;

uniform sampler2D gNormal;

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
    // ��ǰ֡��ɫ��������ƫ�ƣ�
    vec2 jitteredUV = TexCoords + vec2(uJitterX, uJitterY);
    vec3 current = texture(uCurrentFrame, jitteredUV).rgb;
    
    // ��ʷ��ɫ
    vec3 history = texture(uHistory, TexCoords).rgb;
    
    // ����������ɫ��Χ������Ӱ��
    vec3 minColor = current, maxColor = current;
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            vec3 neighbor = texelFetch(uCurrentFrame, ivec2(gl_FragCoord) + ivec2(x,y), 0).rgb;
            minColor = min(minColor, neighbor);
            maxColor = max(maxColor, neighbor);
        }
    }
    
    // ǯ����ʷ��ɫ������Χ
    history = clipAABB(history, minColor, maxColor);

    // ���߼�飬��ǿʱ���˲�
    float blendFactor = 0.0;
    vec3 prevNormal = texture(gNormal, TexCoords).rgb;
    vec3 currNormal = texture(gNormal, jitteredUV).rgb;
    if (dot(prevNormal, currNormal) < 0.9) {
        blendFactor = uBlendFactor * 0.2; // ���߱仯��ʱ������ʷȨ��
    }

    // ��ϵ�ǰ֡����ʷ֡
    vec3 result = mix(history, current, blendFactor);
    fragColor = vec4(result, 1.0);
}