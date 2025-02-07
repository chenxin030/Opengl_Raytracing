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
            // λ��       // ��������
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
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS){
        camera.FOV -= (float)yoffset * camera.ZoomSpeed;
        if (camera.FOV < 1.0f) camera.FOV = 1.0f;
        if (camera.FOV > 90.0f) camera.FOV = 90.0f;
    }
}

// TAA
// Halton�������ɺ������Ͳ������У�
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

    // ���֡����������
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Bloom Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // TAA
    // �����ʷ������
    GLuint historyTex[2]; // ˫����
    GLuint taaFBO;

    // ��ʼ�����֣��ڴ���Bloom FBO����ӣ�
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

    while (!glfwWindowShouldClose(window)) {

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // ��ȡʵ��֡����ߴ磨�������أ�
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

        // �����ӿ�ƥ��ʵ�ʳߴ磨����3�в��ܷ����������ڣ�
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

        // ------------------------- Bloom���� -------------------------
        // ����1: ������ȡ
        glBindFramebuffer(GL_FRAMEBUFFER, bloomFBOs[0]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        extractShader.use();
        extractShader.setFloat("threshold", 1.0f); // ����������ֵ
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, outputTex);
        RenderQuad(); // ��Ⱦȫ���ı���

        // ����2: ��˹ģ�����������ˮƽ�ʹ�ֱģ������������Խ��Ч��Խƽ����
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


        imguiManager.EndFrame();

        glfwSwapBuffers(window);
        glfwPollEvents();

    }

    glDeleteTextures(1, &outputTex);

    glfwTerminate();
    return 0;
}