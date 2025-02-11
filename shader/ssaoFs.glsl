#version 430 core
out float FragColor;
in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise;

uniform vec3 samples[64];
uniform mat4 projection;
uniform mat4 view;

// 屏幕尺寸参数
const vec2 noiseScale = vec2(800.0/4.0, 800.0/4.0); // 根据实际分辨率调整

void main() {
    // 获取输入数据
    vec3 fragPos = texture(gPosition, TexCoords).xyz;
    vec3 normal = normalize(texture(gNormal, TexCoords).rgb);
    vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).xyz);
    
    // 创建TBN矩阵
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    
    // 计算环境遮蔽
    float occlusion = 0.0;
    for(int i = 0; i < 64; ++i) {
        // 获取样本位置
        vec3 samplePos = TBN * samples[i];
        samplePos = fragPos + samplePos * 0.5; // 调整半径
        
        // 投影到屏幕空间
        vec4 offset = vec4(samplePos, 1.0);
        offset = projection * view * offset;
        offset.xy /= offset.w;
        offset.xy = offset.xy * 0.5 + 0.5;
        
        // 获取样本深度
        float sampleDepth = texture(gPosition, offset.xy).z;
        
        // 范围检查+累积
        float rangeCheck = smoothstep(0.0, 1.0, 0.5 / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + 0.025 ? 1.0 : 0.0) * rangeCheck;
    }
    FragColor = 1.0 - (occlusion / 64.0);
}