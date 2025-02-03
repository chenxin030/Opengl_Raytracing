
#include "SSBO.h"
#include "ImGuiManager.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "TextureLoader.h"

ImGuiManager::ImGuiManager(GLFWwindow* window) : m_Window(window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	// 禁用状态保存，否则下次运行会是上次运行结束时的状态
    io.IniFilename = nullptr; // 添加此行

    SetupStyle();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430");

    // 加载默认天空盒
    std::string defaultPath = std::string("res/skybox/") + m_SkyboxPaths[0];
    m_CurrentSkyboxTexture = ConvertHDRToCubemap(defaultPath.c_str());
}

ImGuiManager::~ImGuiManager() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (m_CurrentSkyboxTexture != 0) {
        glDeleteTextures(1, &m_CurrentSkyboxTexture);
    }
}

void ImGuiManager::BeginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManager::EndFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiManager::DrawObjectsList(SSBO& ssbo)
{
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
    ImGui::Begin("Scene Objects");

    static int objType = 0;
    static float pos[3] = { 0.0f, 0.0f, -5.0f };
    static float color[3] = { 1.0f, 1.0f, 1.0f };
    static float radius = 1.0f;
    static float normal[3] = { 0.0f, 1.0f, 0.0f };

    // 物体类型选择
    ImGui::RadioButton("Sphere", &objType, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Plane", &objType, 1);

    // 通用属性
    ImGui::InputFloat3("Position", pos);
    ImGui::ColorEdit3("Color", color);

    // 类型特定属性
    if (objType == 0) {
        ImGui::InputFloat("Radius", &radius);
    }

    static float distance = 0.0f;
    if (objType == 1) {
        ImGui::InputFloat3("Normal", normal);
        ImGui::InputFloat("Distance", &distance);
    }

    // 添加物体按钮
    if (ImGui::Button("Add Object")) {
        Object newObj;
        newObj.type = objType;
        newObj.position = glm::vec3(pos[0], pos[1], pos[2]);
        newObj.color = glm::vec3(color[0], color[1], color[2]);

        if (objType == 0) {
            newObj.radius = radius;
        }
        else if (objType == 1) {
            newObj.normal = glm::normalize(glm::vec3(normal[0], normal[1], normal[2]));
            newObj.distance = distance;
        }
        ssbo.objects.push_back(newObj);
    }

    // 显示物体总数
    ImGui::Text("Total Objects: %d", ssbo.objects.size());
    ImGui::Separator();

    // 遍历所有物体
    for (int i = 0; i < ssbo.objects.size(); ++i) {
        Object& obj = ssbo.objects[i];

        // 生成唯一标识符
        ImGui::PushID(i);

        // 创建可折叠的树节点
        bool nodeOpen = ImGui::TreeNodeEx(
            (void*)(intptr_t)i,
            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed,
            "Object %d: %s", i, (obj.type == 0) ? "Sphere" : "Plane"
        );

        if (nodeOpen) {
            // 显示通用属性
            ImGui::Text("Position");
            ImGui::InputFloat3("##pos", &obj.position.x);

            ImGui::Text("Color");
            ImGui::ColorEdit3("##col", &obj.color.x);

            // 显示类型特定属性
            if (obj.type == 0) { // 球体
                ImGui::Text("Radius");
                ImGui::InputFloat("##radius", &obj.radius);
            }
            else { // 平面
                ImGui::Text("Normal");
                ImGui::InputFloat3("##normal", &obj.normal.x);
                ImGui::Text("Distance");
                ImGui::InputFloat("##distance", &obj.distance);
            }

            ImGui::TreePop();
        }

        bool deleteClicked = false;
        if (ImGui::SmallButton("Delete")) {
            // 先关闭节点再删除
            if (nodeOpen) {
                ImGui::PopID();
            }
            ssbo.objects.erase(ssbo.objects.begin() + i);
            i--; // 调整索引，避免跳过元素
            continue;
        }

        ImGui::PopID();
    }
    ssbo.update();

    ImGui::End();
}

void ImGuiManager::DrawCameraControls(Camera& camera) {

    ImGui::SetNextWindowPos(ImVec2(10, 70), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
    ImGui::Begin("Camera Controls");

    // 位置控制
    ImGui::Text("Position");
    ImGui::SliderFloat3("##pos", &camera.Position.x, -10.0f, 10.0f);

    // 方向显示
    ImGui::Text("Direction: (%.2f, %.2f, %.2f)",
        camera.Front.x, camera.Front.y, camera.Front.z);

    // FOV控制
    ImGui::Text("FOV");
    ImGui::SliderFloat("##fov", &camera.FOV, 1.0f, 90.0f);

    // 摄像头移速
    ImGui::SliderFloat("Move Speed", &camera.MoveSpeed, 1.0f, 20.0f);

    ImGui::End();
}

void ImGuiManager::DrawLightController(LightSSBO& lightSSBO) {

    ImGui::SetNextWindowPos(ImVec2(10, 40), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
    ImGui::Begin("Light Controller");

    static int lightType = 0;
    static float pos[3] = { 0.0f, 3.0f, 0.0f };
    static float color[3] = { 1.0f, 1.0f, 1.0f };
    static float intensity = 1.0f;
    static float radius = 5.0f;

    // 光源类型选择
    ImGui::RadioButton("Point", &lightType, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Directional", &lightType, 1);

    // 通用属性
    ImGui::ColorEdit3("Color", color);
    ImGui::SliderFloat("Intensity", &intensity, 0.1f, 10.0f);

    // 类型特定属性
    static float dir[3];
    if (lightType == 0) {
        ImGui::InputFloat3("Position", pos);
        ImGui::SliderFloat("Radius", &radius, 1.0f, 20.0f);
    }
    else {
        dir[0] = 0.0f; dir[1] = -1.0f; dir[2] = 0.0f;
        ImGui::InputFloat3("Direction", dir);
    }

    // 添加光源按钮
    if (ImGui::Button("Add Light")) {
        Light newLight;
        newLight.type = (lightType == 0) ? LightType::POINT : LightType::DIRECTIONAL;
        newLight.color = glm::vec3(color[0], color[1], color[2]);
        newLight.intensity = intensity;

        if (lightType == 0) {
            newLight.position = glm::vec3(pos[0], pos[1], pos[2]);
            newLight.radius = radius;
        }
        else {
            newLight.direction = glm::vec3(dir[0], dir[1], dir[2]);
        }

        lightSSBO.lights.push_back(newLight);
    }

    // 光源列表
    ImGui::Separator();
    ImGui::Text("Lights (%d)", lightSSBO.lights.size());

    for (int i = 0; i < lightSSBO.lights.size(); ++i) {
        ImGui::PushID(i);
        Light& light = lightSSBO.lights[i];

        if (ImGui::TreeNodeEx((void*)(intptr_t)i, ImGuiTreeNodeFlags_DefaultOpen,
            "Light %d: %s", i, (light.type == LightType::POINT) ? "Point" : "Directional"))
        {
            // 编辑属性
            ImGui::ColorEdit3("Color", &light.color.r);
            ImGui::SliderFloat("Intensity", &light.intensity, 0.1f, 10.0f);

            if (light.type == LightType::POINT) {
                ImGui::InputFloat3("Position", &light.position.x);
                ImGui::SliderFloat("Radius", &light.radius, 1.0f, 20.0f);
            }
            else {
                ImGui::InputFloat3("Direction", &light.direction.x);
            }

            // 删除按钮
            if (ImGui::SmallButton("Delete")) {
                lightSSBO.lights.erase(lightSSBO.lights.begin() + i);
                ImGui::TreePop();
                ImGui::PopID();
                break;
            }

            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    lightSSBO.update();
    ImGui::End();
}

void ImGuiManager::DrawFPS() {
    // 设置窗口属性：无标题栏、透明背景、固定位置
    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoMove;

    // 设置窗口位置在右上角
    const float PAD = 10.0f;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_pos = viewport->WorkPos;
    ImVec2 work_size = viewport->WorkSize;
    ImVec2 window_pos = ImVec2(work_pos.x + work_size.x - PAD, work_pos.y + PAD);
    ImVec2 window_pos_pivot = ImVec2(1.0f, 0.0f);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);

    // 绘制窗口
    ImGui::SetNextWindowBgAlpha(0.0f); // 完全透明背景
    if (ImGui::Begin("FPS Overlay", nullptr, flags)) {
        float fps = ImGui::GetIO().Framerate; // 直接获取帧率
        // 动态颜色设置
        ImVec4 color;
        if (fps < 30.0f) {
            color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);  // 红色
        }
        else if (fps <= 60.0f) {
            color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);  // 黄色
        }
        else {
            color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);  // 绿色
        }

        // 带颜色文本显示
        ImGui::TextColored(color, "FPS: ");
        ImGui::SameLine();
        ImGui::TextColored(color, "%.1f", fps);
    }
    ImGui::End();
}

void ImGuiManager::ChooseSkybox() {

    ImGui::SetNextWindowPos(ImVec2(10, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
    ImGui::Begin("Skybox Settings");

    // 启用/禁用天空盒复选框
    ImGui::Checkbox("Enable Skybox", &m_UseSkybox);

    // 天空盒选择下拉菜单
    if (ImGui::Combo("Select Skybox", &m_SelectedSkyboxIndex, m_SkyboxNames.data(), static_cast<int>(m_SkyboxNames.size()))) {
        // 当选择改变时重新加载天空盒
        if (m_CurrentSkyboxTexture != 0) {
            glDeleteTextures(1, &m_CurrentSkyboxTexture);
            m_CurrentSkyboxTexture = 0;
        }
        std::string fullPath = std::string("res/skybox/") + m_SkyboxPaths[m_SelectedSkyboxIndex];
        m_CurrentSkyboxTexture = ConvertHDRToCubemap(fullPath.c_str());
    }

    ImGui::End();
}

void ImGuiManager::HandleCameraMovement(Camera& camera, float deltaTime)
{
    // 只在未激活ImGui时处理输入
    if (!ImGui::GetIO().WantCaptureKeyboard && !ImGui::GetIO().WantCaptureMouse) {
        // 按住右键才能移动鼠标
        if (glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            // 前后移动
            glm::vec3 moveDir(0.0f);
            if (glfwGetKey(m_Window, GLFW_KEY_W) == GLFW_PRESS)
                moveDir += camera.Front;
            if (glfwGetKey(m_Window, GLFW_KEY_S) == GLFW_PRESS)
                moveDir -= camera.Front;
            if (glfwGetKey(m_Window, GLFW_KEY_A) == GLFW_PRESS)
                moveDir -= camera.Right;
            if (glfwGetKey(m_Window, GLFW_KEY_D) == GLFW_PRESS)
                moveDir += camera.Right;
            // 上下移动
            if (glfwGetKey(m_Window, GLFW_KEY_Q) == GLFW_PRESS)
                moveDir += camera.Up;
            if (glfwGetKey(m_Window, GLFW_KEY_E) == GLFW_PRESS)
                moveDir -= camera.Up;

            if (glm::length(moveDir) > 0.0f) {
                camera.ProcessMovement(glm::normalize(moveDir), deltaTime);
            }
        }
    }
}

void ImGuiManager::SetupStyle() {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;
}