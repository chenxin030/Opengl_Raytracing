// AO.h
#pragma once
#include <glm/glm.hpp>
#include "Shader.h"
#include <vector>

class AOManager {
public:
    AOManager(int width, int height);
    ~AOManager();

    void Init();
    void Resize(int width, int height);
    void Render(GLuint positionTex, GLuint normalTex, const glm::mat4& view, const glm::mat4& projection);
    void DrawUI();

    // 参数
    int screenWidth, screenHeight;
    bool enableAO = true;
    float aoStrength = 1.0f;
    bool showSettings = true;

private:
    void InitSSAO();
    void RenderSSAO(GLuint positionTex, GLuint normalTex, const glm::mat4& view, const glm::mat4& projection);

    // SSAO相关
    GLuint ssaoFBO, ssaoBlurFBO;
    GLuint ssaoColorBuffer, ssaoBlurBuffer;
    GLuint noiseTexture;
    std::vector<glm::vec3> ssaoKernel;
    Shader ssaoShader;
    Shader ssaoBlurShader;
};