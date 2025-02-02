#version 430 core
layout(local_size_x = 16, local_size_y = 16) in;
layout(rgba32f, binding = 0) uniform image2D outputImage;

uniform vec3 cameraPos;
uniform vec3 cameraDir;
uniform vec3 cameraUp;
uniform vec3 cameraRight;
uniform float fov;

void main()
{
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imageSize = imageSize(outputImage);

    vec2 uv = vec2(pixelCoords) / vec2(imageSize);
    uv = uv * 2.0 - 1.0;

    vec3 rayDir = normalize(cameraDir + uv.x * cameraRight + uv.y * cameraUp);

    // 这里可以添加光线追踪的逻辑，比如与场景中的物体相交、计算光照等
    vec3 color = vec3(uv, 0.5); // 简单的UV颜色

    imageStore(outputImage, pixelCoords, vec4(color, 1.0));
}