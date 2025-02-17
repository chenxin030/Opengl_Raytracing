#version 430 core

const float PI = 3.14159265359;

// ��������
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gMaterial;  // metallic(r), roughness(g), transparency(b), ior(a)
uniform sampler2D gAlbedo;    // ��������������

// ��Դ���ݣ���raytracingCs.glslһ�£�
struct Light {
    int type;            // 0=���Դ, 1=�����, 2=�����
    vec3 position;      
    vec3 direction;     
    vec3 color;       
    float intensity;    
    float radius;       
    int samples;        
    float shadowSoftness;
    int shadowType;     
    int pcfSamples;     
    float lightSize;    
    float angularRadius;
};
layout(std430, binding = 1) buffer Lights {
    Light lights[];
};
uniform int numLights;

// �������պ�
uniform vec3 cameraPos;
uniform samplerCube skybox;
uniform bool useSkybox;

// Shadow Maps��ʾ��������ʹ�ö������Ӱ��ͼ��
uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;

out vec4 FragColor;

// ����PBR���㺯��
vec3 computePBR(float metallic, float roughness, vec3 albedo, vec3 N, vec3 V, vec3 L, vec3 H, vec3 radiance) {
    float alpha = roughness * roughness;
    
    // ���߷ֲ�������GGX��
    float NdotH = max(dot(N, H), 0.0);
    float NDF = alpha * alpha / (PI * pow(NdotH * NdotH * (alpha * alpha - 1.0) + 1.0, 2.0));
    
    // ���κ�����Smith-Schlick��
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float G = max(dot(N, V), 0.0) / (max(dot(N, V), 0.0) * (1.0 - k) + k);
    G *= max(dot(N, L), 0.0) / (max(dot(N, L), 0.0) * (1.0 - k) + k);
    
    // �������Schlick���ƣ�
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(H, V), 0.0), 5.0);
    
    // ��������߹�
    vec3 kD = (1.0 - F) * (1.0 - metallic);
    vec3 diffuse = kD * albedo / PI;
    vec3 specular = (NDF * G * F) / max(4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0), 0.001);
    
    return (diffuse + specular) * radiance * max(dot(N, L), 0.0);
}

// �����PCF��Ӱ����
float CalcDirLightShadow(vec3 fragPos, vec3 normal) {
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    float currentDepth = projCoords.z;
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (currentDepth - 0.005) > pcfDepth ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}

void main() {
    ivec2 coord = ivec2(gl_FragCoord.xy);
    
    // ��G-Buffer��ȡ����
    vec3 worldPos = texelFetch(gPosition, coord, 0).rgb;
    vec3 N = normalize(texelFetch(gNormal, coord, 0).rgb);
    vec3 albedo = texelFetch(gAlbedo, coord, 0).rgb;
    vec4 matData = texelFetch(gMaterial, coord, 0);
    
    float metallic = matData.r;
    float roughness = matData.g;
    
    vec3 V = normalize(cameraPos - worldPos);
    vec3 Lo = vec3(0.0);

    // �������й�Դ
    for(int i = 0; i < numLights; ++i) {
        Light light = lights[i];
        vec3 L = vec3(0.0);
        vec3 radiance = vec3(0.0);
        float attenuation = 1.0;
        
        // ������ղ���
        if(light.type == 0) { // ���Դ
            vec3 lightVec = light.position - worldPos;
            float distance = length(lightVec);
            L = normalize(lightVec);
            attenuation = 1.0 / (1.0 + 0.1 * distance + 0.01 * distance * distance);
            radiance = light.color * light.intensity * attenuation;
        } else if(light.type == 1) { // �����
            L = normalize(-light.direction);
            radiance = light.color * light.intensity;
            // ��Ӱ����
            float shadow = CalcDirLightShadow(worldPos, N);
            radiance *= (1.0 - shadow);
        }
        
        // PBR����
        vec3 H = normalize(V + L);
        Lo += computePBR(metallic, roughness, albedo, N, V, L, H, radiance);
    }

    // ��պз��䣨���Խ������棩
    if(useSkybox && metallic > 0.1) {
        vec3 R = reflect(-V, N);
        Lo += texture(skybox, R).rgb * metallic;
    }

    // ���HDR��ɫ
    FragColor = vec4(Lo, 1.0);
}