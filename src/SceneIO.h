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

static ObjectType IntToObjectType(const std::string& str) {
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

static void WriteObjectParams(std::ofstream& file, const Object& obj) {
	file << " " << obj.position.x << " " << obj.position.y << " " << obj.position.z
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

static void WriteLightParams(std::ofstream& file, const Light& light) {
	file << " " << light.position.x << " " << light.position.y << " " << light.position.z
		<< " " << light.direction.x << " " << light.direction.y << " " << light.direction.z
		<< " " << light.color.x << " " << light.color.y << " " << light.color.z
		<< " " << light.intensity
		<< " " << light.radius
		<< " " << light.samples;
}

class SceneIO {
public:
    static bool Load(const std::string& path, SSBO& ssbo, LightSSBO& lightSSBO) {
        std::ifstream file(path);
        if (!file.is_open()) return false;

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string type;
            iss >> type;

            if (type == "OBJECT") ParseObject(iss, ssbo);
            else if (type == "LIGHT") ParseLight(iss, lightSSBO);
        }
        return true;
    }

    static bool Save(const std::string& path, SSBO& ssbo, LightSSBO& lightSSBO) {
        std::ofstream file(path);
        if (!file.is_open()) return false;

        // 写入物体数据
        for (const auto& obj : ssbo.objects) {
            file << "OBJECT " << ObjectTypeToString(obj.type);
            WriteObjectParams(file, obj);
            file << "\n";
        }

        // 写入光源数据
        for (const auto& light : lightSSBO.lights) {
            file << "LIGHT " << LightTypeToString(light.type);
            WriteLightParams(file, light);
            file << "\n";
        }
        return true;
    }

private:
    static void ParseObject(std::istringstream& iss, SSBO& ssbo) {
        Object obj;
        std::string typeStr;
        iss >> typeStr >> obj.position.x >> obj.position.y >> obj.position.z;
        obj.type = IntToObjectType(typeStr);

        iss >> obj.radius
            >> obj.normal.x >> obj.normal.y >> obj.normal.z
            >> obj.size.x >> obj.size.y;

        int matType;
        iss >> matType;
        obj.material.type = static_cast<MaterialType>(matType);
		iss >> obj.material.albedo.x >> obj.material.albedo.y >> obj.material.albedo.z
			>> obj.material.metallic
			>> obj.material.roughness
			>> obj.material.ior
			>> obj.material.transparency
			>> obj.material.specular;
        ssbo.objects.push_back(obj);
    }

    static void ParseLight(std::istringstream& iss, LightSSBO& lightSSBO) {
        Light light;
        std::string typeStr;
        iss >> typeStr;
        light.type = StringToLightType(typeStr);

		iss >> light.position.x >> light.position.y >> light.position.z
			>> light.direction.x >> light.direction.y >> light.direction.z
			>> light.color.x >> light.color.y >> light.color.z
			>> light.intensity
			>> light.radius
			>> light.samples;
        lightSSBO.lights.push_back(light);
    }
};