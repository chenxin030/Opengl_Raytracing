#version 430 core

// 新增最大递归深度限制
#define MAX_RAY_DEPTH 3 

struct Ray {
    vec3 origin;
    vec3 direction;
    float energy;    // 光线能量（用于衰减）
    int depth;       // 递归深度
};

struct Material {
    int type;
    vec3 albedo;
    float metallic;
    float roughness; 
    float ior;
    float transparency;
    float specular;
};

struct Object {
    int type;        // 0=球体, 1=平面
    vec3 position;
    Material material;
    float radius;   // 球体
    vec3 normal;    // 平面
    float distance; // 平面
};

struct Light {
    int type;            // 0=点光源, 1=定向光
    vec3 position;      
    vec3 direction;     
    vec3 color;       
    float intensity;    // 光线能量（用于衰减）
    int depth;       // 递归深度
};

layout(local_size_x = 16, local_size_y = 16) in;
layout(rgba32f, binding = 0) uniform image2D outputImage;

layout(std430, binding = 0) buffer Objects {
    Object objects[];
};
layout(std430, binding = 1) buffer Lights {
    Light lights[];
};

uniform int numObjects;

uniform vec3 cameraPos;
uniform vec3 cameraDir;
uniform vec3 cameraUp;
uniform vec3 cameraRight;
uniform float fov;

uniform samplerCube skybox;
uniform bool useSkybox;
// raytracingCs.glsl

vec3 computeLighting(vec3 point, vec3 normal, Material mat, vec3 viewDir) {
    vec3 totalLight = vec3(0.0);
    
    for(int i = 0; i < lights.length(); i++) {
        Light light = lights[i];
        vec3 lightDir;
        float attenuation = 1.0;
        
        // 计算光照方向和衰减
        if(light.type == 0) { // 点光源
            lightDir = light.position - point;
            float distance = length(lightDir);
            attenuation = 1.0 / (1.0 + 0.1 * distance + 0.01 * distance * distance);
            lightDir = normalize(lightDir);
        } else { // 定向光
            lightDir = normalize(-light.direction);
        }
        
        // 漫反射项
        float NdotL = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = mat.albedo * light.color * NdotL;
        
        // 镜面反射项（根据材质类型）
        vec3 specular = vec3(0.0);
        if(mat.type == 0) {
            // GGX高光模型（简化版）
            vec3 H = normalize(lightDir + viewDir);
            float NdotH = max(dot(normal, H), 0.0);
            float alpha = mat.roughness * mat.roughness;
            float alpha2 = alpha * alpha;
            float denom = (NdotH * NdotH) * (alpha2 - 1.0) + 1.0;
            float D = alpha2 / (3.14159265 * denom * denom);
            specular = D * mat.metallic * light.color;
        } 
        else if(mat.type == 1) {
            // 菲涅尔反射（Schlick近似）
            float cosTheta = dot(normal, -viewDir);
            float F0 = pow((1.0 - mat.ior) / (1.0 + mat.ior), 2.0);
            float F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
            
            // 折射计算（仅当透明度>0）
            if(mat.transparency > 0.01) {
                vec3 refractDir = refract(-viewDir, normal, 1.0/mat.ior);
                // ...执行折射光线追踪...
            }
            specular = vec3(F) * light.color;
        }
        else { // MATERIAL_PLASTIC
            // Blinn-Phong高光
            vec3 H = normalize(lightDir + viewDir);
            float NdotH = max(dot(normal, H), 0.0);
            specular = light.color * pow(NdotH, 32.0) * mat.specular;
        }
        
        // 能量守恒：漫反射与镜面反射的权重分配
        float kD = (1.0 - mat.metallic) * (1.0 - mat.transparency);
        totalLight += (kD * diffuse + specular) * attenuation * light.intensity;
    }
    
    return totalLight;
}
bool intersectSphere(Ray ray, Object obj, out float t) {
    vec3 oc = ray.origin - obj.position;
    float a = dot(ray.direction, ray.direction);
    float b = 2.0 * dot(oc, ray.direction);
    float c = dot(oc, oc) - obj.radius * obj.radius;
    float discriminant = b * b - 4.0 * a * c;

    if (discriminant < 0.0) {
        return false;
    } else {
        t = (-b - sqrt(discriminant)) / (2.0 * a);
        return t > 0.0;
    }
}

bool intersectPlane(Ray ray, Object obj, out float t) {
    float denom = dot(obj.normal, ray.direction);
    if (abs(denom) > 1e-6) {
        vec3 p0 = obj.normal * obj.distance; // 使用存储的距离值
        t = dot(p0 - ray.origin, obj.normal) / denom;
        return t >= 0.0;
    }
    return false;
}

bool intersectObjects(Ray ray, out Material hitMaterial, out vec3 hitNormal, out float t) {
    float minT = 1e6;
    bool hit = false;
    
    for(int i = 0; i < numObjects; i++) {
        Object obj = objects[i];
        float currentT;
        bool isHit = false;
        
        // 球体相交检测
        if(obj.type == 0) {
            isHit = intersectSphere(ray, obj, currentT);
        } 
        // 平面相交检测
        else if(obj.type == 1) {
            isHit = intersectPlane(ray, obj, currentT);
        }
        
        if(isHit && currentT < minT) {
            minT = currentT;
            hit = true;
            t = currentT;
            
            // 获取材质属性
            hitMaterial = obj.material;
            
            // 计算法线
            if(obj.type == 0) {
                hitNormal = normalize(ray.origin + ray.direction * t - obj.position);
            } else {
                hitNormal = obj.normal;
            }
        }
    }
    return hit;
}

// 新增菲涅尔反射率计算
float fresnelSchlick(float cosTheta, float ior) {
    float r0 = pow((1.0 - ior) / (1.0 + ior), 2.0);
    return r0 + (1.0 - r0) * pow(1.0 - cosTheta, 5.0);
}

void main() {
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imageSize = imageSize(outputImage);
    
    // 计算UV坐标
    vec2 uv = (vec2(pixelCoords) + 0.5) / vec2(imageSize); // 中心采样
    uv = uv * 2.0 - 1.0;
    
    // 初始化光线
    Ray ray;
    ray.origin = cameraPos;
    ray.direction = normalize(cameraDir + uv.x * cameraRight + uv.y * cameraUp);
    ray.energy = 1.0;
    ray.depth = 0;
    
    vec3 finalColor = vec3(0.0);
    vec3 accumulatedColor = vec3(1.0); // 颜色累积乘数
    
    // 光线迭代循环（基于递归深度）
    while(ray.depth < MAX_RAY_DEPTH && ray.energy > 0.01) {
        Material hitMaterial;
        vec3 hitNormal;
        float hitT;
        
        if(intersectObjects(ray, hitMaterial, hitNormal, hitT)) {
            vec3 hitPoint = ray.origin + ray.direction * hitT;
            
            // 计算视角方向
            vec3 viewDir = -ray.direction;
            
            // 菲涅尔反射率计算
            float cosTheta = dot(-viewDir, hitNormal);
            float fresnel = fresnelSchlick(cosTheta, hitMaterial.ior);
            
            // 表面颜色计算（包含光照）
            vec3 surfaceColor = computeLighting(
                hitPoint, 
                hitNormal, 
                hitMaterial, 
                viewDir
            ) * hitMaterial.albedo;
            
            // 混合到最终颜色
            finalColor += accumulatedColor * surfaceColor * (1.0 - hitMaterial.transparency);
            
            // 准备下一次光线追踪
            if(hitMaterial.transparency > 0.01) {
                // 折射路径
                vec3 refractDir = refract(
                    viewDir, 
                    hitNormal, 
                    hitMaterial.ior
                );
                
                Ray refractRay = Ray(
                    hitPoint - 0.001 * hitNormal,
                    refractDir,
                    ray.energy * (1.0 - fresnel) * hitMaterial.transparency,
                    ray.depth + 1
                );
                
                // 更新累积颜色
                accumulatedColor *= hitMaterial.albedo * (1.0 - fresnel);
                ray = refractRay;
            } else {
                // 反射路径
                vec3 reflectDir = reflect(viewDir, hitNormal);
                Ray reflectRay = Ray(
                    hitPoint + 0.001 * hitNormal,
                    reflectDir,
                    ray.energy * fresnel,
                    ray.depth + 1
                );
                
                accumulatedColor *= hitMaterial.albedo * fresnel;
                ray = reflectRay;
            }
        } else {
            // 处理天空盒
            if(useSkybox) {
                finalColor += accumulatedColor * texture(skybox, ray.direction).rgb;
            }
            break;
        }
    }
    
    imageStore(outputImage, pixelCoords, vec4(finalColor, 1.0));
}