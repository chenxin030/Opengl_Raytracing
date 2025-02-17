#pragma once
#include "Shader.h"
#include "global.h"
#include "ImGUIManager.h"
#include "PerformanceProfiler.h"
#include "SSBO.h"
class DeferredShadingPipeline {
private:
    GLFWwindow* window = nullptr;
    ImGuiManager imguiManager;
    SSBO ssbo;
    LightSSBO lightSSBO;
    // GPU Time Query
    PerformanceProfiler gProfiler;

    Shader geometryPassShader;
    GLuint gBufferFBO;
    GLuint gPositionTex, gNormalTex, gMaterialTex, gAlbedoTex;

    Shader lightingPassShader;

    Shader postProcessShader;

public:
    void Init();
    void InitGBuffer();
    void InitShader();

    void Render();
};