#version 430 core
out float FragColor;
in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise;

uniform vec3 samples[64];
uniform mat4 projection;
uniform mat4 view;

// ��Ļ�ߴ����
const vec2 noiseScale = vec2(800.0/4.0, 800.0/4.0); // ����ʵ�ʷֱ��ʵ���

void main() {
    // ��ȡ��������
    vec3 fragPos = texture(gPosition, TexCoords).xyz;
    vec3 normal = normalize(texture(gNormal, TexCoords).rgb);
    vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).xyz);
    
    // ����TBN����
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    
    // ���㻷���ڱ�
    float occlusion = 0.0;
    for(int i = 0; i < 64; ++i) {
        // ��ȡ����λ��
        vec3 samplePos = TBN * samples[i];
        samplePos = fragPos + samplePos * 0.5; // �����뾶
        
        // ͶӰ����Ļ�ռ�
        vec4 offset = vec4(samplePos, 1.0);
        offset = projection * view * offset;
        offset.xy /= offset.w;
        offset.xy = offset.xy * 0.5 + 0.5;
        
        // ��ȡ�������
        float sampleDepth = texture(gPosition, offset.xy).z;
        
        // ��Χ���+�ۻ�
        float rangeCheck = smoothstep(0.0, 1.0, 0.5 / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + 0.025 ? 1.0 : 0.0) * rangeCheck;
    }
    FragColor = 1.0 - (occlusion / 64.0);
}