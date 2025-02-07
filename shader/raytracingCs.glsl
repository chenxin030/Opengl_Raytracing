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
uniform float fov = 60.0;          // 垂直视场角（角度制）
uniform float focalLength = 1.0;   // 焦距

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

// 原理1：光线生成
void generateCameraRay(out Ray ray) {
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imageSize = imageSize(outputImage);
    
    // 计算屏幕坐标（-1到1范围）
    vec2 uv = (vec2(pixelCoords) + 0.5) / vec2(imageSize);
    uv = uv * 2.0 - 1.0;
    
    // 根据FOV计算实际屏幕坐标
    float aspect = float(imageSize.x) / float(imageSize.y);
    float tanFov = tan(radians(fov) * 0.5);
    uv.x *= aspect * tanFov * focalLength;
    uv.y *= tanFov * focalLength;
    
    ray.origin = cameraPos;
    ray.direction = normalize(cameraDir + uv.x * cameraRight + uv.y * cameraUp);
    ray.energy = 1.0;
    ray.depth = 0;
}

// Schlick近似法
float fresnelSchlick(float cosTheta, float ior) {
    float r0 = pow((1.0 - ior) / (1.0 + ior), 2.0);
    return r0 + (1.0 - r0) * pow(1.0 - cosTheta, 5.0); // = R：反射的占比
}

// 原理2：基于物理的材质（PBR）
vec3 computePBR(Material mat, vec3 N, vec3 V, vec3 L, vec3 H, vec3 radiance) {
    // 粗糙度重映射
    float alpha = pow(mat.roughness, 2.0);
    
    // 法线分布函数（GGX/Trowbridge-Reitz）
    float NDF = alpha * alpha / (PI * pow(pow(max(dot(N, H), 0.0), 2.0) 
        * (alpha * alpha - 1.0) + 1.0, 2.0));
    
    // 几何遮蔽函数（Schlick-GGX）
    float k = pow(mat.roughness + 1.0, 2.0) / 8.0;
    float G = max(dot(N, V), 0.0) / (max(dot(N, V), 0.0) * (1.0 - k) + k);
    G *= max(dot(N, L), 0.0) / (max(dot(N, L), 0.0) * (1.0 - k) + k);
    
    // 菲涅尔方程（Schlick近似）
    vec3 F0 = mix(vec3(0.04), mat.albedo, mat.metallic);
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(H, V), 0.0), 5.0);
    
    // 组合BRDF
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    vec3 specular = numerator / max(denominator, 0.001);
    
    // 漫反射（能量守恒）
    vec3 kD = (1.0 - F) * (1.0 - mat.metallic);
    vec3 diffuse = kD * mat.albedo / PI;
    
    return (diffuse + specular) * radiance * max(dot(N, L), 0.0);
}

// 原理3：折射处理（使用Snell定律）
vec3 calculateRefraction(Ray ray, vec3 hitPoint, vec3 N, Material mat, inout float energy) {
    bool entering = dot(ray.direction, N) < 0.0;
    float eta = entering ? (1.0 / mat.ior) : mat.ior;
    vec3 normal = entering ? N : -N;
    
    // 计算折射方向
    vec3 refractDir = refract(normalize(ray.direction), normal, eta);
    if (dot(refractDir, refractDir) < 0.001) { // 全内反射
        refractDir = reflect(ray.direction, normal);
    }
    
    // 更新光线能量
    energy *= (1.0 - fresnelSchlick(max(dot(-ray.direction, normal), 0.0), mat.ior));
    return refractDir;
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

// [修改后的computeLighting函数]
vec3 computeLighting(vec3 P, vec3 N, Material mat, vec3 V) {
    vec3 Lo = vec3(0.0);
    
    for(int i = 0; i < numLights; ++i) {
        Light light = lights[i];
        vec3 lightDir;
        // 衰减
        float attenuation = 1.0;
        float lightDistance = 0.0;
        
        // 计算基础光照参数
        if(light.type == 0) { // 点光源
            lightDir = light.position - P;
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
            float distance = length(light.position - P);
            attenuation = 1.0 / (distance * distance);
        
            // 计算光源法线方向贡献
            vec3 lightNormal = normalize(light.direction);
            float lightCos = max(dot(-lightDir, lightNormal), 0.0);
            attenuation *= lightCos;
        }
        
        // 计算阴影因子
        float shadowFactor = calculateShadow(P, N, light);
        
        // 使用PBR计算光照
        vec3 L = normalize(lightDir);
        vec3 H = normalize(V + L);
        vec3 radiance = light.color * attenuation * light.intensity;
        
        Lo += computePBR(mat, N, V, L, H, radiance) * shadowFactor;
    }
    return Lo;
}

void main() {
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);

    Ray ray;
    generateCameraRay(ray);
    
    vec3 finalColor = vec3(0.0);
    vec3 throughput = vec3(1.0);
    
    for(int depth = 0; depth < MAX_RAY_DEPTH; ++depth) {
        Material mat;
        vec3 N;
        float t;
        
        if(!intersectObjects(ray, mat, N, t)) {
            if(useSkybox) finalColor += throughput * texture(skybox, ray.direction).rgb;
            break;
        }
        
        vec3 P = ray.origin + ray.direction * t;
        vec3 V = normalize(-ray.direction);
        
        // 计算表面光照
        vec3 Lo = computeLighting(P, N, mat, V);
        finalColor += throughput * Lo;
        
        // 计算反射/折射
        float F = fresnelSchlick(max(dot(V, N), 0.0), mat.ior);
        
        // 俄罗斯轮盘赌终止条件
        if(depth > 2) {
            float continueProb = min(max(throughput.x, max(throughput.y, throughput.z)) * 0.95, 0.95);
            if(random(gl_GlobalInvocationID.xy + depth) > continueProb) break;
            throughput /= continueProb;
        }
        
        // 选择反射或折射
        if(mat.transparency > 0.0) {
            ray.direction = calculateRefraction(ray, P, N, mat, ray.energy);
            ray.origin = P - N * 0.001;
            throughput *= mat.albedo * (1.0 - F) * mat.transparency;
        } else {
            ray.direction = reflect(ray.direction, N);
            ray.origin = P + N * 0.001;
            throughput *= mat.albedo * F;
        }
        
        ray.energy *= 0.8; // 能量衰减
    }
    
    imageStore(outputImage, pixelCoords, vec4(finalColor, 1.0));
}