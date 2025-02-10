#include "Shader.h"
#include "SSBO.h"
#include "ImGUIManager.h"
#include <GLFW/glfw3.h>
#include "Camera.h"
#include <imgui.h>
#include "TextureLoader.h"

const int WIDTH = 800;
const int HEIGHT = 800;

Camera camera;
bool firstMouse = true;
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

void RenderQuad() {
    static GLuint quadVAO = 0, quadVBO;
    if (quadVAO == 0) {
        float quadVertices[] = {
            // 位置       // 纹理坐标
            -1.0f,  1.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f,
             1.0f, -1.0f, 1.0f, 0.0f,

            -1.0f,  1.0f, 0.0f, 1.0f,
             1.0f, -1.0f, 1.0f, 0.0f,
             1.0f,  1.0f, 1.0f, 1.0f
        };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = (xpos - lastX) * camera.Sensitivity;
        float yoffset = (lastY - ypos) * camera.Sensitivity;
        lastX = xpos;
        lastY = ypos;

        camera.Yaw += xoffset;
        camera.Pitch += yoffset;

        // 限制俯仰角
        if (camera.Pitch > 89.0f) camera.Pitch = 89.0f;
        if (camera.Pitch < -89.0f) camera.Pitch = -89.0f;

        camera.UpdateVectors();
    }
    else {
        firstMouse = true;
    }
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS){
        camera.FOV -= (float)yoffset * camera.ZoomSpeed;
        if (camera.FOV < 1.0f) camera.FOV = 1.0f;
        if (camera.FOV > 90.0f) camera.FOV = 90.0f;
    }
}

// TAA： Halton序列生成函数（低差异序列）
float haltonSequence(int index, int base) {
	float result = 0.0f;
	float f = 1.0f / base;
	int i = index;
	while (i > 0) {
		result += f * (i % base);
		i = (int)floor(i / base);
		f /= base;
	}
	return result;
}

// GPU Time Query
enum QueryStages {
    RAYTRACE_START,
    RAYTRACE_END,
    BLOOM_EXTRACT_START,
    BLOOM_EXTRACT_END,
    BLOOM_BLUR_START,
    BLOOM_BLUR_END,
    TAA_START,
    TAA_END,
    NUM_QUERIES_PER_FRAME
};
const int QUERY_FRAME_COUNT = 2; // 双缓冲
GLuint queryPool[QUERY_FRAME_COUNT][NUM_QUERIES_PER_FRAME];
int currentQueryIndex = 0;
std::vector<float> frameTimeHistory(60, 0.0f);

GPUTimingData gpuTiming[QUERY_FRAME_COUNT];

void ProcessQueries() {
    int readIndex = (currentQueryIndex - 1 + QUERY_FRAME_COUNT) % QUERY_FRAME_COUNT;

    GLint available;
    glGetQueryObjectiv(queryPool[readIndex][TAA_END], GL_QUERY_RESULT_AVAILABLE, &available);

    if (available) {
        GLuint64 start, end;

        // 光线追踪
        glGetQueryObjectui64v(queryPool[readIndex][RAYTRACE_START], GL_QUERY_RESULT, &start);
        glGetQueryObjectui64v(queryPool[readIndex][RAYTRACE_END], GL_QUERY_RESULT, &end);
        gpuTiming[readIndex].raytracingTime = (end - start) / 1e6;

        // Bloom提取
        glGetQueryObjectui64v(queryPool[readIndex][BLOOM_EXTRACT_START], GL_QUERY_RESULT, &start);
        glGetQueryObjectui64v(queryPool[readIndex][BLOOM_EXTRACT_END], GL_QUERY_RESULT, &end);
        gpuTiming[readIndex].bloomExtractTime = (end - start) / 1e6;

        // Bloom模糊
        glGetQueryObjectui64v(queryPool[readIndex][BLOOM_BLUR_START], GL_QUERY_RESULT, &start);
        glGetQueryObjectui64v(queryPool[readIndex][BLOOM_BLUR_END], GL_QUERY_RESULT, &end);
        gpuTiming[readIndex].bloomBlurTime = (end - start) / 1e6;

        // TAA
        glGetQueryObjectui64v(queryPool[readIndex][TAA_START], GL_QUERY_RESULT, &start);
        glGetQueryObjectui64v(queryPool[readIndex][TAA_END], GL_QUERY_RESULT, &end);
        gpuTiming[readIndex].taaTime = (end - start) / 1e6;

        // 更新历史
        frameTimeHistory.erase(frameTimeHistory.begin());
        frameTimeHistory.push_back(gpuTiming[readIndex].raytracingTime);

        gpuTiming[readIndex].available = true;
    }
}

int main() {

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "OpenGL Ray Tracing", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    Shader raytracingShader("shader/raytracingCs.glsl");
    Shader outputShader("shader/outputVs.glsl", "shader/outputFs.glsl");

    GLuint outputTex;
    glGenTextures(1, &outputTex);
    glBindTexture(GL_TEXTURE_2D, outputTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindImageTexture(0, outputTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    ImGuiManager imguiManager(window);

    SSBO ssbo;
    LightSSBO lightSSBO;

    // bloom
    GLuint bloomFBOs[2], bloomTextures[2];
    glGenFramebuffers(2, bloomFBOs);
    glGenTextures(2, bloomTextures);

    // 创建亮度提取和模糊用的纹理
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

    // 检查帧缓冲完整性
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Bloom Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // TAA： 添加历史缓冲区
    GLuint historyTex[2]; // 双缓冲
    GLuint taaFBO;

    // 初始化部分（在创建Bloom FBO后添加）
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

    Shader extractShader("shader/outputVs.glsl", "shader/brightness_extractFs.glsl");
    Shader blurShader("shader/outputVs.glsl", "shader/gaussian_blurFs.glsl");
    Shader bloomCombineShader("shader/outputVs.glsl", "shader/bloom_combineFs.glsl");
    Shader taaShader("shader/outputVs.glsl", "shader/taaFs.glsl");

    // GPU Time Query
    glGenQueries(QUERY_FRAME_COUNT * NUM_QUERIES_PER_FRAME, &queryPool[0][0]);

    while (!glfwWindowShouldClose(window)) {

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

		// GPU Time Query
        int activeIndex = currentQueryIndex % QUERY_FRAME_COUNT;

        // 获取实际帧缓冲尺寸（物理像素）
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

        // 设置视口匹配实际尺寸（有这3行才能放满整个窗口）
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
        
        raytracingShader.use();
        raytracingShader.setInt("numObjects", ssbo.objects.size());
        raytracingShader.setInt("numLights", lightSSBO.lights.size());
        raytracingShader.setVec3("cameraPos", camera.Position);
        raytracingShader.setVec3("cameraDir", camera.Front);
        raytracingShader.setVec3("cameraUp", camera.Up);
        raytracingShader.setVec3("cameraRight", camera.Right);
        raytracingShader.setFloat("fov", camera.FOV);

        // 绑定天空盒
        if (imguiManager.IsSkyboxEnabled()) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, imguiManager.GetCurrentSkyboxTexture());
        }
        raytracingShader.setBool("useSkybox", imguiManager.IsSkyboxEnabled());

        glQueryCounter(queryPool[activeIndex][RAYTRACE_START], GL_TIMESTAMP);
        glDispatchCompute(
            (WIDTH + 15) / 16,  // 向上取整
            (HEIGHT + 15) / 16,
            1
        );
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        glQueryCounter(queryPool[activeIndex][RAYTRACE_END], GL_TIMESTAMP);

        // ------------------------- Bloom处理 -------------------------
        // 步骤1: 亮度提取
        glQueryCounter(queryPool[activeIndex][BLOOM_EXTRACT_START], GL_TIMESTAMP);
        glBindFramebuffer(GL_FRAMEBUFFER, bloomFBOs[0]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        extractShader.use();
        extractShader.setFloat("threshold", 1.0f); // 设置亮度阈值
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, outputTex);
        RenderQuad(); // 渲染全屏四边形
        glQueryCounter(queryPool[activeIndex][BLOOM_EXTRACT_END], GL_TIMESTAMP);

        // 步骤2: 高斯模糊（交替进行水平和垂直模糊，迭代次数越多效果越平滑）
        glQueryCounter(queryPool[activeIndex][BLOOM_BLUR_START], GL_TIMESTAMP);
        bool horizontal = true;
        blurShader.use();
        for (int i = 0; i < 10; i++) { // 模糊迭代次数（例如10次）
            glBindFramebuffer(GL_FRAMEBUFFER, bloomFBOs[horizontal]);
            blurShader.setBool("horizontal", horizontal);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, bloomTextures[!horizontal]);
            RenderQuad();
            horizontal = !horizontal;
        }
        glQueryCounter(queryPool[activeIndex][BLOOM_BLUR_END], GL_TIMESTAMP);

        // 步骤3: 合并Bloom效果到最终输出
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        bloomCombineShader.use();
        bloomCombineShader.setFloat("bloomStrength", 0.5f); // 调整Bloom强度
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, outputTex); // 原始场景
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, bloomTextures[!horizontal]); // 模糊后的高光
        RenderQuad();
        glQueryCounter(queryPool[activeIndex][3], GL_TIMESTAMP);

        // TAA
        if (imguiManager.IsTAAEnabled()) {
            glQueryCounter(queryPool[activeIndex][TAA_START], GL_TIMESTAMP);

            glBindFramebuffer(GL_FRAMEBUFFER, taaFBO);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, historyTex[currentHistory], 0);

            taaShader.use();
            taaShader.setFloat("uBlendFactor", imguiManager.GetTAABlendFactor());
            taaShader.setInt("uCurrentFrame", 0);
            taaShader.setInt("uHistory", 1);
            taaShader.setFloat("uJitterX", haltonSequence(frameCount % 8, 2) * 0.5 / WIDTH);
            taaShader.setFloat("uJitterY", haltonSequence(frameCount % 8, 3) * 0.5 / HEIGHT);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, outputTex); // 当前帧
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, historyTex[1 - currentHistory]); // 上一帧

            RenderQuad();

            // 交换历史缓冲
            frameCount++;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glBindTexture(GL_TEXTURE_2D, historyTex[currentHistory]);
            glGenerateMipmap(GL_TEXTURE_2D); // 可选：生成Mipmap用于下帧采样

            glQueryCounter(queryPool[activeIndex][TAA_END], GL_TIMESTAMP);
        }

        ProcessQueries();
        currentQueryIndex++;
        int activeIdx = (currentQueryIndex - 1) % QUERY_FRAME_COUNT;
        imguiManager.DrawPerformanceStats(
            deltaTime * 1000.0f,
            gpuTiming[activeIdx],
            frameTimeHistory
        );

        imguiManager.EndFrame();

        glfwSwapBuffers(window);
        glfwPollEvents();

    }

    glDeleteTextures(1, &outputTex);

    glfwTerminate();
    return 0;
}