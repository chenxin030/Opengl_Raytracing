
#include "SSBO.h"
#include "ImGuiManager.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

ImGuiManager::ImGuiManager(GLFWwindow* window) : m_Window(window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    SetupStyle();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430");
}

ImGuiManager::~ImGuiManager() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
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
    ImGui::Begin("Scene Objects");

    static int objType = 0;
    static float pos[3] = { 0.0f, 0.0f, -5.0f };
    static float color[3] = { 1.0f, 1.0f, 1.0f };
    static float radius = 1.0f;
    static float normal[3] = { 0.0f, 1.0f, 0.0f };

    // ��������ѡ��
    ImGui::RadioButton("Sphere", &objType, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Plane", &objType, 1);

    // ͨ������
    ImGui::InputFloat3("Position", pos);
    ImGui::ColorEdit3("Color", color);

    // �����ض�����
    if (objType == 0) {
        ImGui::InputFloat("Radius", &radius);
    }

    static float distance = 0.0f;
    if (objType == 1) {
        ImGui::InputFloat3("Normal", normal);
        ImGui::InputFloat("Distance", &distance);
    }

    // ������尴ť
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
        ssbo.update();
    }

    // ��ʾ��������
    ImGui::Text("Total Objects: %d", ssbo.objects.size());
    ImGui::Separator();

    // ������������
    for (int i = 0; i < ssbo.objects.size(); ++i) {
        Object& obj = ssbo.objects[i];

        // ����Ψһ��ʶ��
        ImGui::PushID(i);

        // �������۵������ڵ�
        bool nodeOpen = ImGui::TreeNodeEx(
            (void*)(intptr_t)i,
            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed,
            "Object %d: %s", i, (obj.type == 0) ? "Sphere" : "Plane"
        );

        if (nodeOpen) {
            // ��ʾͨ������
            ImGui::Text("Position");
            ImGui::InputFloat3("##pos", &obj.position.x);

            ImGui::Text("Color");
            ImGui::ColorEdit3("##col", &obj.color.x);

            // ��ʾ�����ض�����
            if (obj.type == 0) { // ����
                ImGui::Text("Radius");
                ImGui::InputFloat("##radius", &obj.radius);
            }
            else { // ƽ��
                ImGui::Text("Normal");
                ImGui::InputFloat3("##normal", &obj.normal.x);
                ImGui::Text("Distance");
                ImGui::InputFloat("##distance", &obj.distance);
            }

            ImGui::TreePop();
        }

        bool deleteClicked = false;
        if (ImGui::SmallButton("Delete")) {
            // �ȹرսڵ���ɾ��
            if (nodeOpen) {
                ImGui::PopID();
            }
            ssbo.objects.erase(ssbo.objects.begin() + i);
            ssbo.update();
            i--; // ������������������Ԫ��
            continue;
        }

        ImGui::PopID();
    }

    ImGui::End();
}

void ImGuiManager::DrawCameraControls(Camera& camera) {
    ImGui::Begin("Camera Controls");

    // λ�ÿ���
    ImGui::Text("Position");
    ImGui::SliderFloat3("##pos", &camera.Position.x, -10.0f, 10.0f);

    // ������ʾ
    ImGui::Text("Direction: (%.2f, %.2f, %.2f)",
        camera.Front.x, camera.Front.y, camera.Front.z);

    // FOV����
    ImGui::Text("FOV");
    ImGui::SliderFloat("##fov", &camera.FOV, 1.0f, 90.0f);

    // ����ͷ����
    ImGui::SliderFloat("Move Speed", &camera.MoveSpeed, 1.0f, 20.0f);

    ImGui::End();
}

// ImGuiManager.cpp
void ImGuiManager::DrawGlobalMessage() {
    // ���ô������ԣ��ޱ�������͸���������̶�λ��
    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoMove;

    // ���ô���λ�������Ͻ�
    const float PAD = 10.0f;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_pos = viewport->WorkPos;
    ImVec2 work_size = viewport->WorkSize;
    ImVec2 window_pos = ImVec2(work_pos.x + work_size.x - PAD, work_pos.y + PAD);
    ImVec2 window_pos_pivot = ImVec2(1.0f, 0.0f);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);

    // ���ƴ���
    ImGui::SetNextWindowBgAlpha(0.0f); // ��ȫ͸������
    if (ImGui::Begin("FPS Overlay", nullptr, flags)) {
        float fps = ImGui::GetIO().Framerate; // ֱ�ӻ�ȡ֡��
        // ��̬��ɫ����
        ImVec4 color;
        if (fps < 30.0f) {
            color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);  // ��ɫ
        }
        else if (fps <= 60.0f) {
            color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);  // ��ɫ
        }
        else {
            color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);  // ��ɫ
        }

        // ����ɫ�ı���ʾ
        ImGui::TextColored(color, "FPS: ");
        ImGui::SameLine();
        ImGui::TextColored(color, "%.1f", fps);
    }
    ImGui::End();
}

void ImGuiManager::HandleCameraMovement(Camera& camera, float deltaTime)
{
    // ֻ��δ����ImGuiʱ��������
    if (!ImGui::GetIO().WantCaptureKeyboard && !ImGui::GetIO().WantCaptureMouse) {
        // ����Ҽ����
        if (glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            // WASD�ƶ��߼�
            glm::vec3 moveDir(0.0f);
            if (glfwGetKey(m_Window, GLFW_KEY_W) == GLFW_PRESS)
                moveDir += camera.Front;
            if (glfwGetKey(m_Window, GLFW_KEY_S) == GLFW_PRESS)
                moveDir -= camera.Front;
            if (glfwGetKey(m_Window, GLFW_KEY_A) == GLFW_PRESS)
                moveDir -= camera.Right;
            if (glfwGetKey(m_Window, GLFW_KEY_D) == GLFW_PRESS)
                moveDir += camera.Right;
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