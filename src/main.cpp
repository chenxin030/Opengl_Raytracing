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

// ȫ���ı��εĶ�������
const float quadVertices[] = {
    // λ��          // ��������
    -1.0f,  1.0f,  0.0f, 1.0f,
    -1.0f, -1.0f,  0.0f, 0.0f,
     1.0f, -1.0f,  1.0f, 0.0f,

    -1.0f,  1.0f,  0.0f, 1.0f,
     1.0f, -1.0f,  1.0f, 0.0f,
     1.0f,  1.0f,  1.0f, 1.0f
};

// ���ص�����
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

        // ���Ƹ�����
        if (camera.Pitch > 89.0f) camera.Pitch = 89.0f;
        if (camera.Pitch < -89.0f) camera.Pitch = -89.0f;

        camera.UpdateVectors();
    }
    else {
        firstMouse = true;
    }
}

// ���ֻص�����
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

    SSBO ssbo;
    LightSSBO lightSSBO;

    while (!glfwWindowShouldClose(window)) {

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // ��ȡʵ��֡����ߴ磨�������أ�
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

        // �����ӿ�ƥ��ʵ�ʳߴ磨����3�в��ܷ����������ڣ�
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

        // ����պ�
        if (imguiManager.IsSkyboxEnabled()) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, imguiManager.GetCurrentSkyboxTexture());
        }
        raytracingShader.setBool("useSkybox", imguiManager.IsSkyboxEnabled());

        glDispatchCompute(
            (WIDTH + 15) / 16,  // ����ȡ��
            (HEIGHT + 15) / 16,
            1
        );
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        glClear(GL_COLOR_BUFFER_BIT);

        outputShader.use();
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