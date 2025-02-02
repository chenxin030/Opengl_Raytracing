#include "Shader.h"
#include "SSBO.h"
#include "ImGUIManager.h"
#include <GLFW/glfw3.h>

const int WIDTH = 800;
const int HEIGHT = 600;

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

    Shader raytracingShader("E:/vsProject/opengl_rt/shader/raytracingCs.glsl");
    Shader outputShader("E:/vsProject/opengl_rt/shader/outputVs.glsl", "E:/vsProject/opengl_rt/shader/outputFs.glsl");

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

    // ��������
    raytracingShader.use();
    SSBO ssbo;
    ssbo.objects.clear();
    Object obj{
        0,
        glm::vec3(0.0f, 0.0f, -5.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        1.0f,    // radius
        glm::vec3(0.0f), // normal (unused)
        0.0f     // distance (unused)
    };
    ssbo.objects.push_back(obj);
    ssbo.update();

    while (!glfwWindowShouldClose(window)) {

        imguiManager.BeginFrame();
        imguiManager.DrawObjectController(ssbo);
		imguiManager.DrawObjectsList(ssbo);

        ssbo.update();
        raytracingShader.use();
        raytracingShader.setInt("numObjects", ssbo.objects.size());
        raytracingShader.setVec3("cameraPos", glm::vec3(0.0f, 0.0f, 0.0f));
        raytracingShader.setVec3("cameraDir", glm::vec3(0.0f, 0.0f, -1.0f));
        raytracingShader.setVec3("cameraUp", glm::vec3(0.0f, 1.0f, 0.0f));
        raytracingShader.setVec3("cameraRight", glm::vec3(1.0f, 0.0f, 0.0f));

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