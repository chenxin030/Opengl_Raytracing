#version 430 core
in vec2 TexCoords;
out float FragColor;

uniform sampler2D ssaoInput;  // SSAO原始纹理
uniform bool horizontal;      // 模糊方向标志

// 高斯核权重 (5x5)
const float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec2 tex_offset = 1.0 / textureSize(ssaoInput, 0); // 获取纹理尺寸
    float result = texture(ssaoInput, TexCoords).r * weight[0];
    
    if(horizontal) {
        // 水平模糊
        for(int i = 1; i < 5; ++i) {
            result += texture(ssaoInput, TexCoords + vec2(tex_offset.x * i, 0.0)).r * weight[i];
            result += texture(ssaoInput, TexCoords - vec2(tex_offset.x * i, 0.0)).r * weight[i];
        }
    } else {
        // 垂直模糊
        for(int i = 1; i < 5; ++i) {
            result += texture(ssaoInput, TexCoords + vec2(0.0, tex_offset.y * i)).r * weight[i];
            result += texture(ssaoInput, TexCoords - vec2(0.0, tex_offset.y * i)).r * weight[i];
        }
    }
    FragColor = result;
}