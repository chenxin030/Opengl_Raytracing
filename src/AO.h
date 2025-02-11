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

    // UI����
    void DrawUI();
    AOType GetCurrentType() const { return currentType; }

private:
    void InitSSAO();
    void InitRayTraced();
    void RenderSSAO(GLuint positionTex, GLuint normalTex, const glm::mat4& view, const glm::mat4& projection);
    void RenderRayTraced();

    // SSAO��Դ
    GLuint ssaoFBO, ssaoBlurFBO;
    GLuint ssaoColorBuffer, ssaoBlurBuffer;
    GLuint noiseTexture;
    std::vector<glm::vec3> ssaoKernel;
    Shader ssaoShader;
    Shader ssaoBlurShader;

    // ����׷��AO����
    Shader rtAOShader;
    int rtSamples = 16;
    float rtRadius = 0.5f;

    // ͨ�ò���
    int screenWidth, screenHeight;
    AOType currentType = SSAO;
    bool showSettings = true;
    float aoStrength = 1.0f;
};