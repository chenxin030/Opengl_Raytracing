#version 430
layout(local_size_x = 16, local_size_y = 16) in;
layout(rgba32f, binding = 0) uniform image2D outputImage;
layout(r32ui, binding = 1) uniform uimage2D aoOutput;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform float radius = 0.5;
uniform int samples = 16;

void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    vec3 pos = texelFetch(gPosition, coord, 0).rgb;
    vec3 normal = texelFetch(gNormal, coord, 0).rgb;
    
    float occlusion = 0.0;
    for(int i = 0; i < samples; ++i) {
        // 生成随机方向
        vec3 dir = normalize(normal + ...); // 需要实现半球随机采样
        // 发射检测光线
        // ...
    }
    imageStore(outputImage, coord, vec4(vec3(1.0 - occlusion/samples), 1.0));
}