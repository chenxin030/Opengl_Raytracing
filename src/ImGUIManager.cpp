
#include "SSBO.h"
#include "ImGuiManager.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

ImGuiManager::ImGuiManager(GLFWwindow* window) {
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

void ImGuiManager::DrawObjectController(SSBO& ssbo) {
    ImGui::Begin("Object Controller");

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

    ImGui::End();
}

void ImGuiManager::DrawObjectsList(SSBO& ssbo)
{
    ImGui::Begin("Scene Objects");

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




void ImGuiManager::SetupStyle() {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;
}