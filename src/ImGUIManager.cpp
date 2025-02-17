
#include "SSBO.h"
#include "ImGuiManager.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "TextureLoader.h"
#include "SceneIO.h"
#include "AO.h"

namespace fs = std::filesystem;

ImGuiManager::ImGuiManager(GLFWwindow* window) : m_Window(window) {}

void ImGuiManager::Init() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // ����״̬���棬�����´����л����ϴ����н���ʱ��״̬
    io.IniFilename = nullptr;

    SetupStyle();
    ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
    ImGui_ImplOpenGL3_Init("#version 430");

    // ����Ĭ����պ�
    std::string defaultPath = std::string("res/skybox/") + m_SkyboxPaths[0];
    m_CurrentSkyboxTexture = ConvertHDRToCubemap(defaultPath.c_str());

    fs::create_directories("res/Scene");
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

void ImGuiManager::DrawObjectsList(SSBO& ssbo) {
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
    ImGui::Begin("Scene Objects", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);

    static UIObject uiObj{ "New Object" };

    // ��������
    ImGui::InputText("Name", uiObj.name, IM_ARRAYSIZE(uiObj.name));

    // ��������ѡ��
	static int objType = 0;
    ImGui::RadioButton("Sphere", &objType, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Plane", &objType, 1);
    uiObj.obj.type = static_cast<ObjectType>(objType);

    // ��������
    ImGui::InputFloat3("Position", &uiObj.obj.position.x);
    if (objType == 0) {
        ImGui::InputFloat("Radius", &uiObj.obj.radius);
    }
    if (objType == 1) {
        ImGui::InputFloat3("Normal", &uiObj.obj.normal.x);
        ImGui::InputFloat2("Size (W/H)", &uiObj.obj.size.x);
    }

    // �����������
    ImGui::Separator();
    ImGui::Text("Material Settings");

    // ��������ѡ��
	static int matType = 0;
    ImGui::RadioButton("Metallic", &matType, MATERIAL_METALLIC);
    ImGui::SameLine();
    ImGui::RadioButton("Dielectric", &matType, MATERIAL_DIELECTRIC);
    ImGui::SameLine();
    ImGui::RadioButton("Plastic", &matType, MATERIAL_PLASTIC);
	uiObj.obj.material.type = static_cast<MaterialType>(matType);

    // ͨ�ò���
    ImGui::ColorEdit3("Albedo", &uiObj.obj.material.albedo.r);
    ImGui::SliderFloat("Roughness", &uiObj.obj.material.roughness, 0.0f, 1.0f);

    // �����ض�����
    switch (uiObj.obj.material.type) {
    case MATERIAL_METALLIC:
        ImGui::SliderFloat("Metallic ", &uiObj.obj.material.metallic, 0.0f, 1.0f);
        uiObj.obj.material.transparency = 0.0f; // ������͸��
        break;
    case MATERIAL_DIELECTRIC:
        ImGui::SliderFloat("IOR", &uiObj.obj.material.ior, 1.0f, 2.5f);
        ImGui::SliderFloat("Transparency", &uiObj.obj.material.transparency, 0.0f, 1.0f);
        uiObj.obj.material.metallic = 0.0f; // ����ʷǽ���
        break;
    case MATERIAL_PLASTIC:
        ImGui::SliderFloat("Specular", &uiObj.obj.material.specular, 0.0f, 1.0f);
        uiObj.obj.material.transparency = 0.0f; // ���ϲ�͸��
        break;
    }

    // �������
    if (ImGui::Button("Add Object")) {
        ssbo.objects.push_back(uiObj.obj);
        m_UIObjects.push_back(uiObj);
    }

    // �����б�
    ImGui::Separator();
    ImGui::Text("Total Objects: %d", ssbo.objects.size());

    for (int i = 0; i < m_UIObjects.size(); ++i) {
        UIObject& uiObj = m_UIObjects[i];

        ImGui::PushID(i);

        if (ImGui::TreeNodeEx(
            (void*)(intptr_t)i,
            ImGuiTreeNodeFlags_DefaultOpen,
            "%s##%d", uiObj.name, i)) 
        {
            // �༭����
            ImGui::InputText("Name##obj", uiObj.name, IM_ARRAYSIZE(uiObj.name));

            // ͬ��λ��
            ImGui::InputFloat3("Position", &uiObj.obj.position.x);

            // �����������ͬ��
            if (uiObj.obj.type == ObjectType::SPHERE) {
                ImGui::DragFloat("Radius##obj", &uiObj.obj.radius, 0.1f, 0.0f, 100.0f);
            }
            else if(uiObj.obj.type == ObjectType::PLANE) {
                ImGui::InputFloat3("Normal##obj", &uiObj.obj.normal.x);
                ImGui::InputFloat2("Size##obj", &uiObj.obj.size.x);
            }

            // ͬ����������
            ImGui::Separator();
            ImGui::Text("Material Settings");

            // ��������
            int matType = uiObj.obj.material.type;
            if (ImGui::RadioButton("Metallic##obj", &matType, MATERIAL_METALLIC) ||
                ImGui::RadioButton("Dielectric##obj", &matType, MATERIAL_DIELECTRIC) ||
                ImGui::RadioButton("Plastic##obj", &matType, MATERIAL_PLASTIC))
            {
                uiObj.obj.material.type = static_cast<MaterialType>(matType);
            }

            ImGui::ColorEdit3("Albedo##obj", &uiObj.obj.material.albedo.r);
            ImGui::SliderFloat("Roughness##obj", &uiObj.obj.material.roughness, 0.0f, 1.0f);

            // �����ض�����
            switch (uiObj.obj.material.type) {
            case MATERIAL_METALLIC:
                uiObj.obj.material.transparency = 0.0f; // ������͸��
                ImGui::SliderFloat("Metallic ##obj", &uiObj.obj.material.metallic, 0.0f, 1.0f);
                break;
            case MATERIAL_DIELECTRIC:
                uiObj.obj.material.metallic = 0.0f; // ����ʷǽ���
				uiObj.obj.material.specular = 0.0f; // ������޾��淴��
                ImGui::SliderFloat("IOR##obj", &uiObj.obj.material.ior, 1.0f, 2.5f);
                ImGui::SliderFloat("Transparency##obj", &uiObj.obj.material.transparency, 0.0f, 1.0f);
                break;
            case MATERIAL_PLASTIC:
                uiObj.obj.material.transparency = 0.0f; // ���ϲ�͸��
                ImGui::SliderFloat("Specular##obj", &uiObj.obj.material.specular, 0.0f, 1.0f);
                break;
            }

            ImGui::TreePop();
        }

        // ɾ����ť��Ҫͬʱɾ�������б��Ԫ��
        if (ImGui::SmallButton("Delete")) {
            m_UIObjects.erase(m_UIObjects.begin() + i);
            ssbo.objects.erase(ssbo.objects.begin() + i);
            i--;
        }

        ssbo.objects[i] = uiObj.obj;
        ImGui::PopID();
    }

    ssbo.update();
    ImGui::End();
}

void ImGuiManager::DrawLightController(LightSSBO& lightSSBO) {

    ImGui::SetNextWindowPos(ImVec2(10, 40), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
    ImGui::Begin("Light Controller");

	static UILight uiLight{ "New Light" };

	// ��������
    ImGui::InputText("Name", uiLight.name, IM_ARRAYSIZE(uiLight.name));

    // ��Դ����ѡ��
    static int lightType = 0;
    ImGui::RadioButton("Point", &lightType, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Directional", &lightType, 1);
    ImGui::SameLine();
    ImGui::RadioButton("Area", &lightType, 2);
	uiLight.light.type = static_cast<LightType>(lightType);

    // ͨ������
    ImGui::ColorEdit3("Color", &uiLight.light.color.x);
    ImGui::SliderFloat("Intensity", &uiLight.light.intensity, 0.1f, 10.0f);

    // �����ض�����
    if (lightType == 0) {
        ImGui::InputFloat3("Position", &uiLight.light.position.x);
        ImGui::SliderFloat("Radius", &uiLight.light.radius, 1.0f, 20.0f);
    }
    else if(lightType == 1){
        ImGui::InputFloat3("Direction", &uiLight.light.direction.x);
    }
    else if (lightType == 2) {
        ImGui::InputFloat3("Position", &uiLight.light.position.x);
        ImGui::SliderFloat("Radius", &uiLight.light.radius, 1.0f, 20.0f);
        ImGui::InputFloat3("Direction", &uiLight.light.direction.x);
		ImGui::InputInt("Samples", &uiLight.light.samples);
    }

    // �ڹ�Դ������������
    ImGui::Separator();
    ImGui::Text("Shadow Settings");
    ImGui::Combo("Shadow Type", &uiLight.light.shadowType, "None\0PCF\0PCSS\0");

    if (uiLight.light.shadowType != 0) {
        ImGui::SliderInt("PCF Samples", &uiLight.light.pcfSamples, 1, 16);
        ImGui::SliderFloat("Softness", &uiLight.light.shadowSoftness, 0.0f, 2.0f);
    }

    if (uiLight.light.shadowType == 2) { // PCSS
        if (lightType == 0 || lightType == 1) { // ���Դ�����
            ImGui::SliderFloat("Light Size", &uiLight.light.lightSize, 0.1f, 5.0f);
        }
        if (lightType == 1) { // �����
            ImGui::SliderAngle("Angular Radius", &uiLight.light.angularRadius, 0.1f, 5.0f);
        }
    }

    // ��ӹ�Դ��ť
    if (ImGui::Button("Add Light")) {
        lightSSBO.lights.push_back(uiLight.light);
		m_UILights.push_back(uiLight);
    }

    // ��Դ�б�
    ImGui::Separator();
    ImGui::Text("Lights (%d)", lightSSBO.lights.size());

    for (int i = 0; i < m_UILights.size(); ++i) {
		UILight& uiLight = m_UILights[i];

        ImGui::PushID(i);
        if (ImGui::TreeNodeEx(
            (void*)(intptr_t)i, 
            ImGuiTreeNodeFlags_DefaultOpen,
			"%s##%d", uiLight.name, i))
        {
			// �༭����
			ImGui::InputText("Name##light", uiLight.name, IM_ARRAYSIZE(uiLight.name));

            // �༭����
            ImGui::ColorEdit3("Color", &uiLight.light.color.r);
            ImGui::SliderFloat("Intensity", &uiLight.light.intensity, 0.1f, 10.0f);

            if (uiLight.light.type == LightType::POINT) {
                ImGui::InputFloat3("Position", &uiLight.light.position.x);
                ImGui::SliderFloat("Radius", &uiLight.light.radius, 1.0f, 20.0f);
            }
            else if(uiLight.light.type == LightType::DIRECTIONAL) {
                ImGui::InputFloat3("Direction", &uiLight.light.direction.x);
            }
			else if (uiLight.light.type == LightType::AREA) {
                ImGui::InputFloat3("Position", &uiLight.light.position.x);
                ImGui::SliderFloat("Radius", &uiLight.light.radius, 1.0f, 20.0f);
				ImGui::InputFloat3("Direction", &uiLight.light.direction.x);
				ImGui::InputInt("Samples", &uiLight.light.samples);
			}

            // �ڹ�Դ������������
            ImGui::Separator();
            ImGui::Text("Shadow Settings");
            ImGui::Combo("Shadow Type", &uiLight.light.shadowType, "None\0PCF\0PCSS\0");

            if (uiLight.light.shadowType != 0) {
                ImGui::SliderInt("PCF Samples", &uiLight.light.pcfSamples, 1, 16);
                ImGui::SliderFloat("Softness", &uiLight.light.shadowSoftness, 0.0f, 2.0f);
            }

            if (uiLight.light.shadowType == 2) { // PCSS
                if (lightType == 0 || lightType == 1) { // ���Դ�����
                    ImGui::SliderFloat("Light Size", &uiLight.light.lightSize, 0.1f, 5.0f);
                }
                if (lightType == 1) { // �����
                    ImGui::SliderAngle("Angular Radius", &uiLight.light.angularRadius, 0.1f, 5.0f);
                }
            }

            // ɾ����ť
            if (ImGui::SmallButton("Delete")) {
                lightSSBO.lights.erase(lightSSBO.lights.begin() + i);
				m_UILights.erase(m_UILights.begin() + i);
                ImGui::TreePop();
                ImGui::PopID();
                break;
            }

            ImGui::TreePop();
        }
        lightSSBO.lights[i] = uiLight.light;
        ImGui::PopID();
    }

    lightSSBO.update();
    ImGui::End();
}

void ImGuiManager::DrawCameraControls(Camera& camera) {

    ImGui::SetNextWindowPos(ImVec2(10, 70), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
    ImGui::Begin("Camera Controls");

    // λ�ÿ���
    ImGui::Text("Position");
    ImGui::SliderFloat3("##uiObj.obj.position", &camera.Position.x, -10.0f, 10.0f);

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

void ImGuiManager::DrawFPS() {
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

void ImGuiManager::ChooseSkybox() {

    ImGui::SetNextWindowPos(ImVec2(10, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
    ImGui::Begin("Skybox Settings");

    // ����/������պи�ѡ��
    ImGui::Checkbox("Enable Skybox", &m_UseSkybox);

    // ��պ�ѡ�������˵�
    if (m_UseSkybox && ImGui::Combo("Select Skybox", &m_SelectedSkyboxIndex, m_SkyboxNames.data(), static_cast<int>(m_SkyboxNames.size()))) {
        // ��ѡ��ı�ʱ���¼�����պ�
        if (m_CurrentSkyboxTexture != 0) {
            glDeleteTextures(1, &m_CurrentSkyboxTexture);
            m_CurrentSkyboxTexture = 0;
        }
        std::string fullPath = std::string("res/skybox/") + m_SkyboxPaths[m_SelectedSkyboxIndex];
        m_CurrentSkyboxTexture = ConvertHDRToCubemap(fullPath.c_str());
    }

    ImGui::End();
}

void ImGuiManager::LoadSave(SSBO& ssbo, LightSSBO& lightSSBO) {
    ImGui::SetNextWindowPos(ImVec2(10, 130), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
    ImGui::Begin("Load/Save");

    if (ImGui::Button("Load Scene")) {
        m_FileDialog.show = true;
        m_FileDialog.isOpenMode = true;
        m_FileDialog.currentPath = std::filesystem::current_path().string() + "/res/Scene";
        RefreshFileList();
    }

    ImGui::SameLine();

    if (ImGui::Button("Save Scene")) {
        m_FileDialog.show = true;
        m_FileDialog.isOpenMode = false;
        m_FileDialog.currentPath = std::filesystem::current_path().string() + "/res/Scene";
        RefreshFileList();
    }

    if (m_FileDialog.show) {
        DrawFileDialog(ssbo, lightSSBO);
    }

    ImGui::End();
}

void ImGuiManager::DrawTAASettings()
{
    ImGui::SetNextWindowPos(ImVec2(10, 160), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
    ImGui::Begin("Anti-Aliasing (TAA)");

    ImGui::Checkbox("Enable TAA", &m_EnableTAA);
    if (m_EnableTAA) {
        ImGui::SliderFloat("Blend Factor", &m_TAABlendFactor, 0.01f, 0.5f);
    }

    ImGui::End();
}

void ImGuiManager::DrawFileDialog(SSBO& ssbo, LightSSBO& lightSSBO) {
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(m_FileDialog.isOpenMode ? "Load Scene##FileDialog" : "Save Scene##FileDialog", &m_FileDialog.show)) {
        // ��ǰ·����ʾ
        ImGui::Text("Current Path: %s", m_FileDialog.currentPath.c_str());
        if (ImGui::Button("�� Up")) {
            auto parent_path = std::filesystem::path(m_FileDialog.currentPath).parent_path();
            if (!parent_path.empty()) {
                m_FileDialog.currentPath = parent_path.string();
                RefreshFileList();
            }
        }

        // �ļ��б�
        if (ImGui::BeginChild("FileList##Unique", ImVec2(0, 300), true)) {
            for (size_t i = 0; i < m_FileDialog.entries.size(); ++i) {
                const auto& entry = m_FileDialog.entries[i];
                ImGui::PushID(static_cast<int>(i)); // ÿ����ĿΨһID

                const bool isDirectory = entry.is_directory();
                const std::string displayName = entry.path().filename().string();
                const bool isParentDir = (i == 0 && displayName == "..");

                // ��ʾ�ɵ����Ŀ¼/�ļ���
                ImGuiSelectableFlags flags = ImGuiSelectableFlags_AllowDoubleClick;
                if (ImGui::Selectable(
                    isDirectory ? "[D] " : "[F] ",
                    m_FileDialog.selectedFile == entry.path().string(),
                    flags))
                {
                    if (isDirectory) {
                        if (isParentDir) return; // ��Ŀ¼��Ҫ˫��
                        m_FileDialog.selectedFile = ""; // ����Ŀ¼�����·��
                    }
                    else {
                        m_FileDialog.selectedFile = entry.path().string();
                    }
                }

                // ����˫���¼�
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                    if (isDirectory) {
                        if (isParentDir) {
                            m_FileDialog.currentPath = entry.path().string();
                        }
                        else {
                            m_FileDialog.currentPath = entry.path().string();
                        }
                        RefreshFileList();
                        m_FileDialog.selectedFile = "";
                    }
                }

                // ��ʾ�ļ���
                ImGui::SameLine();
                ImGui::Text("%s", displayName.c_str());

                ImGui::PopID();
            }
        }
        ImGui::EndChild();

        // �ļ������루����ģʽ��
        static char fileName[256] = "untitled";
        if (!m_FileDialog.isOpenMode) {
            ImGui::InputText("File Name##Save", fileName, sizeof(fileName));
            std::filesystem::path fullPath = std::filesystem::path(m_FileDialog.currentPath) / fileName;
            if (fullPath.extension() != ".scene") {
                fullPath.replace_extension(".scene");
            }
            m_FileDialog.selectedFile = fullPath.string();
        }

        // ��ʾ��ǰѡ��
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Selected: %s",
            m_FileDialog.selectedFile.empty() ? "None" : m_FileDialog.selectedFile.c_str());

        // ������ť
        ImGui::BeginDisabled(m_FileDialog.isOpenMode && m_FileDialog.selectedFile.empty());
        if (ImGui::Button(m_FileDialog.isOpenMode ? "Open##FDConfirm" : "Save##FDConfirm")) {
            if (m_FileDialog.isOpenMode) {
                // ���س���
                if (std::filesystem::exists(m_FileDialog.selectedFile)) {
                    ssbo.objects.clear();
                    m_UIObjects.clear();
                    lightSSBO.lights.clear();
                    m_UILights.clear();

                    if (SceneIO::Load(m_FileDialog.selectedFile, m_UIObjects, m_UILights)) {
                        // ͬ��UI����
                        for (const auto& uiObj : m_UIObjects) {
                            ssbo.objects.push_back(uiObj.obj);
                        }
						for (const auto& uiLight : m_UILights) {
							lightSSBO.lights.push_back(uiLight.light);
						}
                        ssbo.update();
                        lightSSBO.update();
                    }
                }
            }
            else {
                // ���泡��
                if (!m_FileDialog.selectedFile.empty()) {
                    SceneIO::Save(m_FileDialog.selectedFile, m_UIObjects, m_UILights);
                }
            }
            m_FileDialog.show = false;
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Cancel##FDCancel")) {
            m_FileDialog.show = false;
        }

        ImGui::End();
    }
}

void ImGuiManager::RefreshFileList()
{
    m_FileDialog.entries.clear();

    // ��ӷ����ϼ�Ŀ¼
    if (m_FileDialog.currentPath != std::filesystem::path(m_FileDialog.currentPath).root_path()) {
        m_FileDialog.entries.emplace_back(std::filesystem::path(m_FileDialog.currentPath).parent_path());
    }

    // ������ǰĿ¼
    for (const auto& entry : std::filesystem::directory_iterator(m_FileDialog.currentPath)) {
        if (entry.is_directory() ||
            (entry.is_regular_file() && entry.path().extension() == ".scene")) {
            m_FileDialog.entries.push_back(entry);
        }
    }

    // ����Ŀ¼��ǰ���ļ��ں�
    std::sort(m_FileDialog.entries.begin(), m_FileDialog.entries.end(),
        [](const auto& a, const auto& b) {
        if (a.is_directory() != b.is_directory())
            return a.is_directory();
        return a.path().filename() < b.path().filename();
    });
}

void ImGuiManager::HandleCameraMovement(Camera& camera, float deltaTime)
{
    // ֻ��δ����ImGuiʱ��������
    if (!ImGui::GetIO().WantCaptureKeyboard && !ImGui::GetIO().WantCaptureMouse) {
        // ��ס�Ҽ������ƶ����
        if (glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            // ǰ���ƶ�
            glm::vec3 moveDir(0.0f);
            if (glfwGetKey(m_Window, GLFW_KEY_W) == GLFW_PRESS)
                moveDir += camera.Front;
            if (glfwGetKey(m_Window, GLFW_KEY_S) == GLFW_PRESS)
                moveDir -= camera.Front;
            if (glfwGetKey(m_Window, GLFW_KEY_A) == GLFW_PRESS)
                moveDir -= camera.Right;
            if (glfwGetKey(m_Window, GLFW_KEY_D) == GLFW_PRESS)
                moveDir += camera.Right;
            // �����ƶ�
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