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
    camera.FOV -= (float)yoffset * camera.ZoomSpeed;
    if (camera.FOV < 1.0f) camera.FOV = 1.0f;
    if (camera.FOV > 90.0f) camera.FOV = 90.0f;
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

    // 创建场景
    SSBO ssbo;

    // 玻璃材质球体（左侧）
    {
        Object glassSphere;
        glassSphere.type = ObjectType::SPHERE; // 球体
        glassSphere.position = glm::vec3(-2.0f, 0.0f, -5.0f);
        glassSphere.radius = 1.0f;

        // 玻璃材质参数
        glassSphere.material.type = MATERIAL_DIELECTRIC;
        glassSphere.material.albedo = glm::vec3(0.9f, 0.9f, 0.9f); // 轻微色调
        glassSphere.material.ior = 1.5f;        // 玻璃折射率
        glassSphere.material.transparency = 0.95f; // 高透明度
        glassSphere.material.roughness = 0.05f;  // 表面光滑

        ssbo.objects.push_back(glassSphere);
        imguiManager.m_UIObjects.push_back({ "Glass Sphere", glassSphere });
    }

    // 金属材质球体（中间）
    {
        Object metalSphere;
        metalSphere.type = ObjectType::SPHERE; // 球体
        metalSphere.position = glm::vec3(0.0f, 0.0f, -5.0f);
        metalSphere.radius = 1.0f;

        // 金属材质参数
        metalSphere.material.type = MATERIAL_METALLIC;
        metalSphere.material.albedo = glm::vec3(0.8f, 0.8f, 0.8f); // 基础色
        metalSphere.material.metallic = 1.0f;     // 完全金属
        metalSphere.material.roughness = 0.1f;    // 轻微粗糙度
        metalSphere.material.specular = 0.0f;     // 金属不使用该参数

        ssbo.objects.push_back(metalSphere);
        imguiManager.m_UIObjects.push_back({ "metal Sphere", metalSphere });
    }

    // 塑料材质球体（右侧）
    {
        Object plasticSphere;
        plasticSphere.type = ObjectType::SPHERE; // 球体
        plasticSphere.position = glm::vec3(2.0f, 0.0f, -5.0f);
        plasticSphere.radius = 1.0f;

        // 塑料材质参数
        plasticSphere.material.type = MATERIAL_PLASTIC;
        plasticSphere.material.albedo = glm::vec3(0.2f, 0.5f, 0.8f); // 蓝色塑料
        plasticSphere.material.specular = 0.6f;    // 中等镜面反射
        plasticSphere.material.roughness = 0.5f;   // 半粗糙表面
        plasticSphere.material.transparency = 0.0f; // 不透明

        ssbo.objects.push_back(plasticSphere);
        imguiManager.m_UIObjects.push_back({ "plastic Sphere", plasticSphere });
    }
    LightSSBO lightSSBO;
    // 添加默认光源
    Light defaultLight;
    defaultLight.type = LightType::POINT;
    defaultLight.position = glm::vec3(0.0f, 3.0f, 0.0f);
    defaultLight.direction = glm::vec3(0.f, 0.f, -1.f);
    defaultLight.color = glm::vec3(1.0f);
    defaultLight.intensity = 1.0f;
    defaultLight.radius = 10.0f;
    lightSSBO.lights.push_back(defaultLight);
    lightSSBO.update();

	glm::vec2 resolution = glm::vec2(WIDTH, HEIGHT);

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
        imguiManager.ChooseSkybox();
        
        raytracingShader.use();
        raytracingShader.setInt("numObjects", ssbo.objects.size());
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

        glClear(GL_COLOR_BUFFER_BIT);

        outputShader.use();
		outputShader.setVec2("resolution", resolution);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, outputTex);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        imguiManager.EndFrame();

        glfwSwapBuffers(window);
        glfwPollEvents();

    }

    glDeleteTextures(1, &outputTex);

    glfwTerminate();
    return 0;
}