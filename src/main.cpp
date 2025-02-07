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

// 全屏四边形的顶点数据
const float quadVertices[] = {
    // 位置          // 纹理坐标
    -1.0f,  1.0f,  0.0f, 1.0f,
    -1.0f, -1.0f,  0.0f, 0.0f,
     1.0f, -1.0f,  1.0f, 0.0f,

    -1.0f,  1.0f,  0.0f, 1.0f,
     1.0f, -1.0f,  1.0f, 0.0f,
     1.0f,  1.0f,  1.0f, 1.0f
};

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

// 鼠标回调函数
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

// 滚轮回调函数
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS){
        camera.FOV -= (float)yoffset * camera.ZoomSpeed;
        if (camera.FOV < 1.0f) camera.FOV = 1.0f;
        if (camera.FOV > 90.0f) camera.FOV = 90.0f;
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

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

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

    Shader extractShader("shader/outputVs.glsl", "shader/brightness_extractFs.glsl");
    Shader blurShader("shader/outputVs.glsl", "shader/gaussian_blurFs.glsl");
    Shader bloomCombineShader("shader/outputVs.glsl", "shader/bloom_combineFs.glsl");

    while (!glfwWindowShouldClose(window)) {

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // 获取实际帧缓冲尺寸（物理像素）
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

        // 设置视口匹配实际尺寸（有这3行才能放满整个窗口）
        glViewport(0, 0, fbWidth, fbHeight);

        imguiManager.BeginFrame();
		imguiManager.HandleCameraMovement(camera, deltaTime);
        imguiManager.DrawFPS();
        imguiManager.DrawObjectsList(ssbo);
        imguiManager.DrawLightController(lightSSBO);
        imguiManager.DrawCameraControls(camera);
        imguiManager.DrawSceneIO(ssbo, lightSSBO);
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

        glDispatchCompute(
            (WIDTH + 15) / 16,  // 向上取整
            (HEIGHT + 15) / 16,
            1
        );
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        // ------------------------- Bloom处理 -------------------------
        // 步骤1: 亮度提取
        glBindFramebuffer(GL_FRAMEBUFFER, bloomFBOs[0]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        extractShader.use();
        extractShader.setFloat("threshold", 1.0f); // 设置亮度阈值
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, outputTex);
        RenderQuad(); // 渲染全屏四边形

        // 步骤2: 高斯模糊（交替进行水平和垂直模糊，迭代次数越多效果越平滑）
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

        imguiManager.EndFrame();

        glfwSwapBuffers(window);
        glfwPollEvents();

    }

    glDeleteTextures(1, &outputTex);

    glfwTerminate();
    return 0;
}