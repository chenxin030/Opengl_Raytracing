#include "Application.h"
#include <iostream>
#include "global.h"

void ForwardShadingPipline::Init()
{
    InitFWEW();
    InitShdaer();
    InitOutputTex();
    imguiManager.m_Window = window;
    imguiManager.Init();
    ssbo.Init();
    lightSSBO.Init();
    InitBloom();
    InitTAA();
    InitAO();
    gProfiler.Init();
}

void ForwardShadingPipline::InitFWEW()
{
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "OpenGL Ray Tracing", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    glewInit();

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
}

void ForwardShadingPipline::InitShdaer()
{
    raytracingShader.Init("shader/raytracingCs.glsl");
    outputShader.Init("shader/outputVs.glsl", "shader/outputFs.glsl");
}

void ForwardShadingPipline::InitOutputTex()
{
    glGenTextures(1, &outputTex);
    glBindTexture(GL_TEXTURE_2D, outputTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindImageTexture(0, outputTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
}

void ForwardShadingPipline::InitBloom()
{
    glGenFramebuffers(2, bloomFBOs);
    glGenTextures(2, bloomTextures);
    // ����������ȡ��ģ���õ�����
    for (int i = 0; i < 2; i++) {
        glBindTexture(GL_TEXTURE_2D, bloomTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, bloomFBOs[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomTextures[i], 0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    extractShader.Init("shader/outputVs.glsl", "shader/brightness_extractFs.glsl");
    blurShader.Init("shader/outputVs.glsl", "shader/gaussian_blurFs.glsl");
    bloomCombineShader.Init("shader/outputVs.glsl", "shader/bloom_combineFs.glsl");
}

void ForwardShadingPipline::InitTAA()
{
    glGenTextures(2, historyTex);
    for (int i = 0; i < 2; i++) {
        glBindTexture(GL_TEXTURE_2D, historyTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glGenFramebuffers(1, &taaFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, taaFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, historyTex[0], 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    taaShader.Init("shader/outputVs.glsl", "shader/taaFs.glsl");
}

void ForwardShadingPipline::InitAO()
{
    aoManager = new AOManager(WIDTH, HEIGHT);
    aoManager->Init();
    imguiManager.aoManager = aoManager;
    // �������λ�����
    glGenTextures(1, &gPositionTex);
    glBindTexture(GL_TEXTURE_2D, gPositionTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glGenTextures(1, &gNormalTex);
    glBindTexture(GL_TEXTURE_2D, gNormalTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void ForwardShadingPipline::Run()
{
    while (!glfwWindowShouldClose(window)) {

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        glViewport(0, 0, fbWidth, fbHeight);

        static int frameCount = 0;
        int currentHistory = frameCount % 2;

        imguiManager.BeginFrame();
        imguiManager.HandleCameraMovement(camera, deltaTime);
        imguiManager.DrawFPS();
        imguiManager.DrawObjectsList(ssbo);
        imguiManager.DrawLightController(lightSSBO);
        imguiManager.DrawCameraControls(camera);
        imguiManager.LoadSave(ssbo, lightSSBO);
        imguiManager.DrawTAASettings();
        imguiManager.ChooseSkybox();
        aoManager->DrawUI();

        raytracingShader.use();
        raytracingShader.setInt("numObjects", ssbo.objects.size());
        raytracingShader.setInt("numLights", lightSSBO.lights.size());
        raytracingShader.setVec3("cameraPos", camera.Position);
        raytracingShader.setVec3("cameraDir", camera.Front);
        raytracingShader.setVec3("cameraUp", camera.Up);
        raytracingShader.setVec3("cameraRight", camera.Right);
        raytracingShader.setFloat("fov", camera.FOV);

        // ����պ�
        if (imguiManager.IsSkyboxEnabled()) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, imguiManager.GetCurrentSkyboxTexture());
        }
        raytracingShader.setBool("useSkybox", imguiManager.IsSkyboxEnabled());

        gProfiler.BeginFrame();
        gProfiler.BeginGPUSection(PerformanceProfiler::Stage::RayTracing);

        glDispatchCompute(
            (WIDTH + 15) / 16,  // ����ȡ��
            (HEIGHT + 15) / 16,
            1
        );
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        gProfiler.EndGPUSection(PerformanceProfiler::Stage::RayTracing);

        // AO
        aoManager->Render(gPositionTex, gNormalTex,
            camera.GetViewMatrix(),
            camera.GetProjectionMatrix((float)WIDTH / HEIGHT));

        // ------------------------- Bloom���� -------------------------
        // ����1: ������ȡ
        gProfiler.BeginGPUSection(PerformanceProfiler::Stage::BloomExtract);

        glBindFramebuffer(GL_FRAMEBUFFER, bloomFBOs[0]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        extractShader.use();
        extractShader.setFloat("threshold", 1.0f); // ����������ֵ
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, outputTex);
        RenderQuad(); // ��Ⱦȫ���ı���

        gProfiler.EndGPUSection(PerformanceProfiler::Stage::BloomExtract);

        // ����2: ��˹ģ�����������ˮƽ�ʹ�ֱģ������������Խ��Ч��Խƽ����
        gProfiler.BeginGPUSection(PerformanceProfiler::Stage::BloomBlur);

        bool horizontal = true;
        blurShader.use();
        for (int i = 0; i < 10; i++) { // ģ����������������10�Σ�
            glBindFramebuffer(GL_FRAMEBUFFER, bloomFBOs[horizontal]);
            blurShader.setBool("horizontal", horizontal);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, bloomTextures[!horizontal]);
            RenderQuad();
            horizontal = !horizontal;
        }

        gProfiler.EndGPUSection(PerformanceProfiler::Stage::BloomBlur);

        // ����3: �ϲ�BloomЧ�����������
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        bloomCombineShader.use();
        bloomCombineShader.setFloat("bloomStrength", 0.5f); // ����Bloomǿ��
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, outputTex); // ԭʼ����
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, bloomTextures[!horizontal]); // ģ����ĸ߹�
        RenderQuad();

        // TAA
        gProfiler.BeginGPUSection(PerformanceProfiler::Stage::TAA);

        if (imguiManager.IsTAAEnabled()) {
            glBindFramebuffer(GL_FRAMEBUFFER, taaFBO);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, historyTex[currentHistory], 0);

            taaShader.use();
            taaShader.setFloat("uBlendFactor", imguiManager.GetTAABlendFactor());
            taaShader.setInt("uCurrentFrame", 0);
            taaShader.setInt("uHistory", 1);
            taaShader.setFloat("uJitterX", haltonSequence(frameCount % 8, 2) * 0.5 / WIDTH);
            taaShader.setFloat("uJitterY", haltonSequence(frameCount % 8, 3) * 0.5 / HEIGHT);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, outputTex); // ��ǰ֡
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, historyTex[1 - currentHistory]); // ��һ֡

            RenderQuad();

            // ������ʷ����
            frameCount++;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glBindTexture(GL_TEXTURE_2D, historyTex[currentHistory]);
            glGenerateMipmap(GL_TEXTURE_2D); // ��ѡ������Mipmap������֡����

        }
        gProfiler.EndGPUSection(PerformanceProfiler::Stage::TAA);

        gProfiler.EndFrame(deltaTime * 1000.0f);
        gProfiler.DrawImGuiPanel();

        imguiManager.EndFrame();

        glfwSwapBuffers(window);
        glfwPollEvents();

    }
}
