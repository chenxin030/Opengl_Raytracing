// AO.cpp
#include "AO.h"
#include <random>
#include <imgui.h>

AOManager::AOManager(int width, int height)
    : screenWidth(width), screenHeight(height) {
    Init();
}

AOManager::~AOManager() {
    glDeleteTextures(1, &ssaoColorBuffer);
    glDeleteTextures(1, &ssaoBlurBuffer);
    glDeleteFramebuffers(1, &ssaoFBO);
    glDeleteFramebuffers(1, &ssaoBlurFBO);
}

void AOManager::Init() {
    InitSSAO();
    InitRayTraced();
}

void AOManager::InitSSAO() {
    // 生成采样核
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
    std::default_random_engine generator;
    for (unsigned int i = 0; i < 64; ++i) {
        glm::vec3 sample(
            randomFloats(generator) * 2.0 - 1.0,
            randomFloats(generator) * 2.0 - 1.0,
            randomFloats(generator)
        );
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = (float)i / 64.0f;
        scale = 0.1f + (scale * scale) * 0.9f;
        sample *= scale;
        ssaoKernel.push_back(sample);
    }

    // 生成噪声纹理
    std::vector<glm::vec3> ssaoNoise;
    for (unsigned int i = 0; i < 16; i++) {
        glm::vec3 noise(
            randomFloats(generator) * 2.0 - 1.0,
            randomFloats(generator) * 2.0 - 1.0,
            0.0f);
        ssaoNoise.push_back(noise);
    }

    glGenTextures(1, &noiseTexture);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // 创建FBO和纹理
    glGenFramebuffers(1, &ssaoFBO);
    glGenFramebuffers(1, &ssaoBlurFBO);

    // ...类似Bloom的FBO初始化代码

}

void AOManager::InitRayTraced() {
    rtAOShader = Shader("shader/raytracingCsAO.glsl");
}

void AOManager::Render(GLuint positionTex, GLuint normalTex, const glm::mat4& view, const glm::mat4& projection) {
    if (currentType == SSAO) {
        RenderSSAO(positionTex, normalTex, view, projection);
    }
    else {
        RenderRayTraced();
    }
}

void AOManager::RenderSSAO(GLuint positionTex, GLuint normalTex, const glm::mat4& view, const glm::mat4& projection) {
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    glClear(GL_COLOR_BUFFER_BIT);

    ssaoShader.use();
    ssaoShader.setMat4("view", view);
    ssaoShader.setMat4("projection", projection);
    ssaoShader.setInt("gPosition", 0);
    ssaoShader.setInt("gNormal", 1);
    ssaoShader.setInt("texNoise", 2);

    // 绑定纹理...
    RenderQuad();

    // 模糊处理...
}

void AOManager::RenderRayTraced() {
    rtAOShader.use();
    rtAOShader.setInt("numObjects", /*传递物体数量*/);
    rtAOShader.setFloat("aoRadius", rtRadius);
    rtAOShader.setInt("aoSamples", rtSamples);

    glDispatchCompute(/*...*/);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void AOManager::DrawUI() {
    ImGui::Begin("AO Settings", &showSettings);

    // 类型选择
    const char* aoTypes[] = { "SSAO", "Ray Traced AO" };
    ImGui::Combo("AO Type", (int*)&currentType, aoTypes, IM_ARRAYSIZE(aoTypes));

    // 通用参数
    ImGui::SliderFloat("AO Strength", &aoStrength, 0.0f, 2.0f);

    // 类型特定参数
    if (currentType == SSAO) {
        ImGui::Text("SSAO Parameters");
        // 添加SSAO参数...
    }
    else {
        ImGui::Text("Ray Traced AO Parameters");
        ImGui::SliderInt("Samples", &rtSamples, 1, 64);
        ImGui::SliderFloat("Radius", &rtRadius, 0.1f, 2.0f);
    }

    ImGui::End();
}