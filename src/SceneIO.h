#pragma once
#include "SSBO.h"
#include "LightSSBO.h"
#include <fstream>
#include <sstream>
#include <glm/glm.hpp>

static std::string ObjectTypeToString(ObjectType type) {
    switch (type) {
    case ObjectType::SPHERE: return "SPHERE";
    case ObjectType::PLANE: return "PLANE";
    default: return "UNKNOWN";
    }
}

static std::string LightTypeToString(LightType type) {
    switch (type) {
    case LightType::DIRECTIONAL: return "DIRECTIONAL";
    case LightType::AREA: return "AREA";
    default: return "POINT";
    }
}

static std::string MaterialTypeToString(MaterialType type) {
    switch (type) {
    case MATERIAL_METALLIC: return "METALLIC";
    case MATERIAL_DIELECTRIC: return "DIELECTRIC";
    default: return "PLASTIC";
    }
}

static ObjectType StringToObjectType(const std::string& str) {
    if (str == "SPHERE") return ObjectType::SPHERE;
    if (str == "PLANE") return ObjectType::PLANE;
    return ObjectType::SPHERE; // 默认值
}

static LightType StringToLightType(const std::string& str) {
    if (str == "DIRECTIONAL") return LightType::DIRECTIONAL;
    if (str == "AREA") return LightType::AREA;
    return LightType::POINT;
}

static MaterialType StringToMaterialType(const std::string& str) {
    if (str == "METALLIC") return MATERIAL_METALLIC;
    if (str == "DIELECTRIC") return MATERIAL_DIELECTRIC;
    return MATERIAL_PLASTIC;
}

static void WriteObjectParams(std::ofstream& file, const Object& obj, const std::string& name) {
    file << " " << name
        << " " << obj.position.x << " " << obj.position.y << " " << obj.position.z
        << " " << obj.radius
        << " " << obj.normal.x << " " << obj.normal.y << " " << obj.normal.z
        << " " << obj.size.x << " " << obj.size.y
        << " " << static_cast<int>(obj.material.type)
        << " " << obj.material.albedo.x << " " << obj.material.albedo.y << " " << obj.material.albedo.z
        << " " << obj.material.metallic
        << " " << obj.material.roughness
        << " " << obj.material.ior
		<< " " << obj.material.transparency
		<< " " << obj.material.specular;
}

static void WriteLightParams(std::ofstream& file, const Light& light, const std::string& name) {
	file << " " << name
        << " " << light.position.x << " " << light.position.y << " " << light.position.z
		<< " " << light.direction.x << " " << light.direction.y << " " << light.direction.z
		<< " " << light.color.x << " " << light.color.y << " " << light.color.z
		<< " " << light.intensity
		<< " " << light.radius
		<< " " << light.samples;
}

void GenerateAABBForObject(Object& obj) {
    if (obj.type == ObjectType::SPHERE) {
        // 球体AABB：中心±半径
        obj.bounds.min = obj.position - glm::vec3(obj.radius);
        obj.bounds.max = obj.position + glm::vec3(obj.radius);
    }
    else if (obj.type == ObjectType::PLANE) {
        // 平面AABB：根据法线方向生成
        glm::vec3 right, forward;

        if (abs(obj.normal.y) > 0.9) { // Y轴法线（地面/天花板）
            right = glm::vec3(1, 0, 0);
            forward = glm::vec3(0, 0, 1);
        }
        else { // X/Z轴法线（墙面）
            right = glm::normalize(glm::cross(obj.normal, glm::vec3(0, 1, 0)));
            forward = glm::normalize(glm::cross(right, obj.normal));
        }

        glm::vec3 halfSizeX = right * (obj.size.x / 2.0f);
        glm::vec3 halfSizeY = forward * (obj.size.y / 2.0f);

        obj.bounds.min = obj.position - halfSizeX - halfSizeY;
        obj.bounds.max = obj.position + halfSizeX + halfSizeY;

        // 沿法线方向扩展1cm避免平面厚度为0
        obj.bounds.min += obj.normal * 0.01f;
        obj.bounds.max += obj.normal * 0.01f;
    }
}

class SceneIO {
public:
    static bool Load(const std::string& path, std::vector<UIObject>& uiObjs, std::vector<UILight>& uiLights) {
        std::ifstream file(path);
        if (!file.is_open()) return false;

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string type;
            iss >> type;

            if (type == "OBJECT") ParseObject(iss, uiObjs);
            else if (type == "LIGHT") ParseLight(iss, uiLights);
        }
        return true;
    }

    static bool Save(const std::string& path, const std::vector<UIObject>& uiObjects, const std::vector<UILight>& uiLights) {
        std::ofstream file(path);
        if (!file.is_open()) return false;

        // 写入物体数据
        for (const auto& uiObj : uiObjects) {
            file << "OBJECT " << ObjectTypeToString(uiObj.obj.type);
            WriteObjectParams(file, uiObj.obj, uiObj.name);
            file << "\n";
        }

        // 写入光源数据
        for (const auto& uiLight : uiLights) {
            file << "LIGHT " << LightTypeToString(uiLight.light.type);
            WriteLightParams(file, uiLight.light, uiLight.name);
            file << "\n";
        }
        return true;
    }

private:
    static void ParseObject(std::istringstream& iss, std::vector<UIObject>& uiObjects) {
        UIObject uiObj;
        std::string typeStr, name;
        iss >> typeStr >> name;
        iss >> uiObj.obj.position.x >> uiObj.obj.position.y >> uiObj.obj.position.z;
        uiObj.obj.type = StringToObjectType(typeStr);

        iss >> uiObj.obj.radius
            >> uiObj.obj.normal.x >> uiObj.obj.normal.y >> uiObj.obj.normal.z
            >> uiObj.obj.size.x >> uiObj.obj.size.y;

        int matType;
        iss >> matType;
        uiObj.obj.material.type = static_cast<MaterialType>(matType);
		iss >> uiObj.obj.material.albedo.x >> uiObj.obj.material.albedo.y >> uiObj.obj.material.albedo.z
			>> uiObj.obj.material.metallic
			>> uiObj.obj.material.roughness
			>> uiObj.obj.material.ior
			>> uiObj.obj.material.transparency
			>> uiObj.obj.material.specular;
        snprintf(uiObj.name, sizeof(uiObj.name), "%s", name.c_str());

        GenerateAABBForObject(uiObj.obj); // 新增

        uiObjects.push_back(uiObj);
    }

    static void ParseLight(std::istringstream& iss, std::vector<UILight>& uiLights) {
        UILight uiLight;
        std::string typeStr, name;
        iss >> typeStr >> name;
        uiLight.light.type = StringToLightType(typeStr);

		iss >> uiLight.light.position.x >> uiLight.light.position.y >> uiLight.light.position.z
			>> uiLight.light.direction.x >> uiLight.light.direction.y >> uiLight.light.direction.z
			>> uiLight.light.color.x >> uiLight.light.color.y >> uiLight.light.color.z
			>> uiLight.light.intensity
			>> uiLight.light.radius
			>> uiLight.light.samples;
        snprintf(uiLight.name, sizeof(uiLight.name), "%s", name.c_str());
        uiLights.push_back(uiLight);
    }
};