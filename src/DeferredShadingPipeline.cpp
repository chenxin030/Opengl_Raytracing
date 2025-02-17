#include "DeferredShadingPipeline.h"

void DeferredShadingPipeline::Init() {
    InitGBuffer();
}

void DeferredShadingPipeline::InitGBuffer() {
    // ����G-Buffer
    glGenFramebuffers(1, &gBufferFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);

    // λ������RGBA32F�߾��ȣ�
    glGenTextures(1, &gPositionTex);
    glBindTexture(GL_TEXTURE_2D, gPositionTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // ��������RGBA16F��
    glGenTextures(1, &gNormalTex);
    glBindTexture(GL_TEXTURE_2D, gNormalTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // ��������RGBA16F��
    glGenTextures(1, &gMaterialTex);
    glBindTexture(GL_TEXTURE_2D, gMaterialTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // ����Albedo����RGBA8��
    glGenTextures(1, &gAlbedoTex);
    glBindTexture(GL_TEXTURE_2D, gAlbedoTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // �󶨸���
    GLuint attachments[4] = {
        GL_COLOR_ATTACHMENT0, // gPosition
        GL_COLOR_ATTACHMENT1, // gNormal
        GL_COLOR_ATTACHMENT2, // gMaterial
        GL_COLOR_ATTACHMENT3  // gAlbedo
    };
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPositionTex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormalTex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gMaterialTex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gAlbedoTex, 0);
    glDrawBuffers(4, attachments);
}

void DeferredShadingPipeline::InitShader()
{
    geometryPassShader.Init("shader/deferred_GeometryPass.glsl");
    lightingPassShader.Init("shader/deferred_LightingPass.glsl");
    postProcessShader.Init("shader/deferred_PostProcessing.glsl");
}

void DeferredShadingPipeline::Render() {
    // ����Pass
    glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);
    geometryPassShader.use();
    glDispatchCompute(
        (WIDTH + 15) / 16,  // ����ȡ��
        (HEIGHT + 15) / 16,
        1
    );

    // ����Pass
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    lightingPassShader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gPositionTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gNormalTex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gMaterialTex);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gAlbedoTex);

    RenderQuad();

    // ���ڴ���
    postProcessShader.use();
    // ...��Bloom/TAA�������...
    RenderQuad();
}