// Material.h
#pragma once
#include <glm/glm.hpp>

enum MaterialType {
    MATERIAL_METALLIC,
    MATERIAL_DIELECTRIC,
    MATERIAL_PLASTIC
};

struct Material {
    alignas(4) MaterialType type = MATERIAL_PLASTIC;    // 4�ֽڶ��룬ռ4�ֽ�
    alignas(16) glm::vec3 albedo = glm::vec3(1.0f);     // 16�ֽڶ��룬ռ12�ֽ�
    alignas(4) float metallic = 0.0f;                   // 4�ֽڶ��룬ռ4�ֽ�
    alignas(4) float roughness = 0.5f;                  // 4�ֽڶ��룬ռ4�ֽ�
    alignas(4) float diffuseStrength;                   // ������ǿ�ȣ�0=�������䣩
    alignas(4) float ior = 1.0f;                        // 4�ֽڶ��룬ռ4�ֽ�
    alignas(4) float transparency = 0.0f;               // 4�ֽڶ��룬ռ4�ֽ�
    alignas(4) float specular = 0.5f;                   // 4�ֽڶ��룬ռ4�ֽ�
    alignas(4) float subsurfaceScatter = 0.0f;          // �α���ɢ��ǿ�ȣ�0-1��
    alignas(16) glm::vec3 subsurfaceColor = glm::vec3(1.0f); // ɢ����ɫ
    alignas(4) float scatterDistance = 0.1f;            // ɢ��������
};

