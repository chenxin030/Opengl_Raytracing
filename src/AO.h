// AO.h
#pragma once
#include <glm/glm.hpp>
#include "Shader.h"
#include <vector>

class AOManager {
public:
    enum AOType { SSAO, RAY_TRACED };

    AOManager(int width, int height);
    ~AOManager();

    void Init();
    void Resize(int width, int height);
    void Render(GLuint positionTex, GLuint normalTex, const glm::mat4& view, const glm::mat4& projection);

    // UI控制
    void DrawUI();
    AOType GetCurrentType() const { return currentType; }

private:
    void InitSSAO();
    void InitRayTraced();
    void RenderSSAO(GLuint positionTex, GLuint normalTex, const glm::mat4& view, const glm::mat4& projection);
    void RenderRayTraced();

    // SSAO资源
    GLuint ssaoFBO, ssaoBlurFBO;
    GLuint ssaoColorBuffer, ssaoBlurBuffer;
    GLuint noiseTexture;
    std::vector<glm::vec3> ssaoKernel;
    Shader ssaoShader;
    Shader ssaoBlurShader;

    // 光线追踪AO参数
    Shader rtAOShader;
    int rtSamples = 16;
    float rtRadius = 0.5f;

    // 通用参数
    int screenWidth, screenHeight;
    AOType currentType = SSAO;
    bool showSettings = true;
    float aoStrength = 1.0f;
};