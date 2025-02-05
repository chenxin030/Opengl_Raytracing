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

    // ��������
    SSBO ssbo;

    // �����������壨��ࣩ
    {
        Object glassSphere;
        glassSphere.type = ObjectType::SPHERE; // ����
        glassSphere.position = glm::vec3(-2.0f, 0.0f, -5.0f);
        glassSphere.radius = 1.0f;

        // �������ʲ���
        glassSphere.material.type = MATERIAL_DIELECTRIC;
        glassSphere.material.albedo = glm::vec3(0.9f, 0.9f, 0.9f); // ��΢ɫ��
        glassSphere.material.ior = 1.5f;        // ����������
        glassSphere.material.transparency = 0.95f; // ��͸����
        glassSphere.material.roughness = 0.05f;  // ����⻬

        ssbo.objects.push_back(glassSphere);
        imguiManager.m_UIObjects.push_back({ "Glass Sphere", glassSphere });
    }

    // �����������壨�м䣩
    {
        Object metalSphere;
        metalSphere.type = ObjectType::SPHERE; // ����
        metalSphere.position = glm::vec3(0.0f, 0.0f, -5.0f);
        metalSphere.radius = 1.0f;

        // �������ʲ���
        metalSphere.material.type = MATERIAL_METALLIC;
        metalSphere.material.albedo = glm::vec3(0.8f, 0.8f, 0.8f); // ����ɫ
        metalSphere.material.metallic = 1.0f;     // ��ȫ����
        metalSphere.material.roughness = 0.1f;    // ��΢�ֲڶ�
        metalSphere.material.specular = 0.0f;     // ������ʹ�øò���

        ssbo.objects.push_back(metalSphere);
        imguiManager.m_UIObjects.push_back({ "metal Sphere", metalSphere });
    }

    // ���ϲ������壨�Ҳࣩ
    {
        Object plasticSphere;
        plasticSphere.type = ObjectType::SPHERE; // ����
        plasticSphere.position = glm::vec3(2.0f, 0.0f, -5.0f);
        plasticSphere.radius = 1.0f;

        // ���ϲ��ʲ���
        plasticSphere.material.type = MATERIAL_PLASTIC;
        plasticSphere.material.albedo = glm::vec3(0.2f, 0.5f, 0.8f); // ��ɫ����
        plasticSphere.material.specular = 0.6f;    // �еȾ��淴��
        plasticSphere.material.roughness = 0.5f;   // ��ֲڱ���
        plasticSphere.material.transparency = 0.0f; // ��͸��

        ssbo.objects.push_back(plasticSphere);
        imguiManager.m_UIObjects.push_back({ "plastic Sphere", plasticSphere });
    }
	// �ذ�
	{
		Object floor;
		floor.type = ObjectType::PLANE; // ƽ��
		floor.position = glm::vec3(0.0f, -1.0f, -5.0f);
		floor.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        floor.size = glm::vec2(1.0f, 1.0f);

		floor.material.type = MATERIAL_METALLIC;
		floor.material.albedo = glm::vec3(0.8f, 0.8f, 0.8f); // ��ɫ
		floor.material.specular = 0.0f;    // �޾��淴��
		floor.material.roughness = 0.0f;   // ��ȫ�⻬
		ssbo.objects.push_back(floor);
		imguiManager.m_UIObjects.push_back({ "Floor", floor });
	}

    // ���Ĭ�Ϲ�Դ
    LightSSBO lightSSBO;
    Light defaultLight;
    defaultLight.type = LightType::DIRECTIONAL;
    defaultLight.position = glm::vec3(0.0f, 3.0f, 0.0f);
    defaultLight.direction = glm::vec3(1.0f, -1.0f, -1.f);
    defaultLight.color = glm::vec3(1.0f);
    defaultLight.intensity = 1.0f;
    defaultLight.radius = 0.0f;
    lightSSBO.lights.push_back(defaultLight);

    Light areaLight;
    areaLight.type = LightType::AREA;
    areaLight.position = glm::vec3(0, 3, 0);
    areaLight.direction = glm::vec3(0, -1, 0); // ��Դ����
    areaLight.color = glm::vec3(1.0);
    areaLight.intensity = 5.0;
    areaLight.radius = 0.5f;    // ������Ӱ��Ͷ�
    areaLight.samples = 8;     // �������̶�
    lightSSBO.lights.push_back(areaLight);
    lightSSBO.update();

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
        imguiManager.ChooseSkybox();
        
        raytracingShader.use();
        raytracingShader.setInt("numObjects", ssbo.objects.size());
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