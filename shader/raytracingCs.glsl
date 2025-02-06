#version 430 core

// 新增最大递归深度限制
#define MAX_RAY_DEPTH 3 
const float PI = 3.14159265359;

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
    vec2 size;      // 平面
};

struct Light {
    int type;            // 0=点光源, 1=定向光, 2=区域光
    vec3 position;      
    vec3 direction;     
    vec3 color;       
    float intensity;    
    float radius;       // 区域光源半径
    int samples;        // 采样数（建议4-16）
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
uniform int numLights;

uniform vec3 cameraPos;
uniform vec3 cameraDir;
uniform vec3 cameraUp;
uniform vec3 cameraRight;
uniform float fov;

uniform samplerCube skybox;
uniform bool useSkybox;

uniform float maxRayDistance = 100.0; // 最大射线距离限制

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
        // 计算交点
        t = dot(obj.position - ray.origin, obj.normal) / denom;
        if(t < 0.0) return false;
        
        vec3 hitPoint = ray.origin + ray.direction * t;
        
        // 构建安全的局部坐标系
        vec3 forward = normalize(cross(obj.normal, vec3(0,0,1)));
        if(length(forward) < 0.1) forward = normalize(cross(obj.normal, vec3(0,1,0)));
        vec3 right = normalize(cross(forward, obj.normal));
        
        // 计算局部坐标
        vec3 localOffset = hitPoint - obj.position;
        float x = dot(localOffset, right);
        float y = dot(localOffset, forward);
        
        // 检查尺寸范围（使用obj.size的x和y分别对应宽高）
        if(abs(x) > obj.size.x/2.0 || abs(y) > obj.size.y/2.0) {
            return false;
        }
        
        return true;
    }
    return false;
}

bool intersectObjects(Ray ray, out Material hitMaterial, out vec3 hitNormal, out float t) {
    float minT = maxRayDistance;
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
        
        if(isHit && currentT > 0.0 && currentT < minT) {
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
    t = minT;
    return hit;
}

// 随机数生成器（用于采样分布）
float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233)))*43758.5453123);
}

// 修改区域光源采样函数
vec3 sampleAreaLight(Light light, vec3 point, vec2 rand, out float pdf) {
    // 极坐标采样（余弦加权）
    float theta = 2.0 * PI * rand.x;
    float r = sqrt(rand.y);
    vec2 disk = r * vec2(cos(theta), sin(theta));
    
    // 构建光源坐标系
    vec3 normal = normalize(light.direction);
    vec3 tangent = normalize(cross(normal, vec3(0,1,0)));
    if(length(tangent) < 0.1) tangent = cross(normal, vec3(1,0,0));
    vec3 bitangent = cross(normal, tangent);
    
    // 生成采样点
    vec3 samplePos = light.position + 
                    light.radius * (disk.x * tangent + disk.y * bitangent);
    
    // 计算概率密度函数(PDF)
    // 修复后（正确面积计算）
    float area = PI * light.radius * light.radius;
    pdf = 1.0 / (area * light.samples);
    return samplePos;
}

float calculateShadow(vec3 point, vec3 normal, Light light) {
    float shadow = 0.0;
    int validSamples = 0;
    
    if(light.type == 0) { // 点光源
        vec3 lightDir = light.position - point;
        float lightDistance = length(lightDir);
        lightDir = normalize(lightDir);
        
        Ray shadowRay;
        shadowRay.origin = point + normal * 0.001;
        shadowRay.direction = lightDir;
        shadowRay.depth = 0;
        
        Material tempMat;
        vec3 tempNormal;
        float t;
        if(intersectObjects(shadowRay, tempMat, tempNormal, t) && t < lightDistance) {
            return 0.5; // 完全阴影
        }
        return 1.0;
    }
    else if(light.type == 1) { // 定向光
        vec3 lightDir = normalize(-light.direction);
        
        Ray shadowRay;
        shadowRay.origin = point + normal * 0.001;
        shadowRay.direction = lightDir;
        shadowRay.depth = 0;
        
        Material tempMat;
        vec3 tempNormal;
        float t;
        if(intersectObjects(shadowRay, tempMat, tempNormal, t)) {
            return 0.5; // 完全阴影
        }
        return 1.0;
    }
    else if(light.type == 2) { // 区域光（重要性采样）
        float totalWeight = 0.0;
        float shadow = 0.0;
        
        for(int i = 0; i < light.samples; i++) {
            vec2 rand = vec2(random(point.xy + i), random(point.yz + i));
            float pdf;
            vec3 samplePos = sampleAreaLight(light, point, rand, pdf);
            
            vec3 lightDir = samplePos - point;
            float lightDistance = length(lightDir);
            lightDir = normalize(lightDir);
            
            // 计算可见性
            Ray shadowRay;
            shadowRay.origin = point + normal * 0.001;
            shadowRay.direction = lightDir;
            shadowRay.depth = 0;
            
            Material tempMat;
            vec3 tempNormal;
            float t;
            if(!intersectObjects(shadowRay, tempMat, tempNormal, t) || t > lightDistance) {
                // 计算重要性权重
                float cosTheta = max(dot(lightDir, normalize(light.direction)), 0.0);
                float weight = cosTheta / (pdf * light.samples);
                shadow += weight;
            }
            totalWeight += 1.0 / (pdf * light.samples);
        }
        
        // 修复后（正确蒙特卡洛积分）
        return shadow / float(light.samples);
    }
    return 1.0;
}

// Schlick近似法
float fresnelSchlick(float cosTheta, float ior) {
    float r0 = pow((1.0 - ior) / (1.0 + ior), 2.0);
    return r0 + (1.0 - r0) * pow(1.0 - cosTheta, 5.0); // = R：反射的占比
}

vec3 computeLighting(vec3 point, vec3 normal, Material mat, vec3 viewDir) {
    vec3 totalLight = vec3(0.0);
    
    for(int i = 0; i < numLights; i++) {
        Light light = lights[i];
        vec3 lightDir;
        // 衰减
        float attenuation = 1.0;
        float lightDistance = 0.0;
        
        // 计算基础光照参数
        if(light.type == 0) { // 点光源
            lightDir = light.position - point;
            lightDistance = length(lightDir);
            attenuation = 1.0 / (1.0 + 0.1 * lightDistance + 0.01 * lightDistance * lightDistance);
            lightDir = normalize(lightDir);
        }
        else if(light.type == 1) { // 定向光
            lightDir = normalize(-light.direction);
            lightDistance = 1e6; // 无限远
        }
        else if(light.type == 2) { // 区域光
            // 使用基于物理的衰减
            float distance = length(light.position - point);
            attenuation = 1.0 / (distance * distance);
        
            // 计算光源法线方向贡献
            vec3 lightNormal = normalize(light.direction);
            float lightCos = max(dot(-lightDir, lightNormal), 0.0);
            attenuation *= lightCos;
        }
        
        // 计算阴影因子
        float shadowFactor = calculateShadow(point, normal, light);
        
        // 漫反射项
        float NdotL = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = mat.albedo * light.color * NdotL;
        
        // 镜面反射项
        vec3 specular = vec3(0.0);
        vec3 H = normalize(lightDir + viewDir);
        float NdotH = max(dot(normal, H), 0.0);
        
        if(mat.type == 0) { // 金属
            float alpha = mat.roughness * mat.roughness;
            float alpha2 = alpha * alpha;
            float denom = NdotH * NdotH * (alpha2 - 1.0) + 1.0;
            float D = alpha2 / (3.14159265 * denom * denom);
            // 修复后（加入菲涅尔项F和几何项G）
            float F = fresnelSchlick(max(dot(H, viewDir), 0.0), mat.ior);
            float G = 1.0 / (1.0 + (mat.roughness + sqrt(mat.roughness)) * NdotL);
            specular = (D * F * G) * mat.metallic * light.color;
        }
        else if(mat.type == 1) { // 电介质
            float F0 = pow((1.0 - mat.ior) / (1.0 + mat.ior), 2.0);
            float F = F0 + (1.0 - F0) * pow(1.0 - max(dot(viewDir, normal), 0.0), 5.0);
            specular = vec3(F) * light.color;
        }
        else if(mat.type == 2) { // 塑料
            specular = light.color * pow(NdotH, 32.0) * mat.specular;
        }
        
        // 能量守恒
        float kD = (1.0 - mat.metallic) * (1.0 - mat.transparency);
        vec3 contribution = (kD * diffuse + specular) * attenuation * light.intensity;
        
        totalLight += contribution * shadowFactor;
    }
    
    return totalLight;
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
            float cosTheta = dot(ray.direction, hitNormal);
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
                // 修复后（判断光线进出材质）
                float eta = (cosTheta > 0.0) ? 1.0/hitMaterial.ior : hitMaterial.ior;
                vec3 refractDir = refract(viewDir, sign(cosTheta)*hitNormal, eta);
                
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