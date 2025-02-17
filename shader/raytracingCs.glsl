#version 430 core

// 新增最大递归深度限制
#define MAX_RAY_DEPTH 1 
const float PI = 3.14159265359;

struct AABB {
    vec3 min;
    vec3 max;
};

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
    float radius;   // 球体
    vec3 normal;    // 平面
    vec2 size;      // 平面
    Material material;
    AABB bounds;
};

struct Light {
    int type;            // 0=点光源, 1=定向光, 2=区域光
    vec3 position;      
    vec3 direction;     
    vec3 color;       
    float intensity;    
    float radius;       // 区域光源半径
    int samples;        // 采样数（建议4-16）

    float shadowSoftness;                                           // 阴影柔化强度
    int shadowType;                                                 // 0=无 1=PCF 2=PCSS
    int pcfSamples;                                                 // PCF采样数
    float lightSize;                                                // PCSS光源尺寸
    float angularRadius;
};

layout(local_size_x = 32, local_size_y = 32) in;
layout(rgba32f, binding = 0) uniform image2D outputImage;
layout(rgba32f, binding = 1) uniform image2D gPosition;
layout(rgba16f, binding = 2) uniform image2D gNormal;

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

uniform float maxRayDistance = 114514.0; // 最大射线距离限制

bool intersectAABB(Ray ray, AABB aabb, out float tMin, out float tMax) {
    vec3 invDir = 1.0 / ray.direction;
    vec3 t0 = (aabb.min - ray.origin) * invDir;
    vec3 t1 = (aabb.max - ray.origin) * invDir;
    
    vec3 tSmaller = min(t0, t1);
    vec3 tLarger = max(t0, t1);
    
    tMin = max(max(tSmaller.x, tSmaller.y), tSmaller.z);
    tMax = min(min(tLarger.x, tLarger.y), tLarger.z);
    
    return tMax >= tMin && tMin < maxRayDistance && tMax > 0.0;
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
        t = dot(obj.position - ray.origin, obj.normal) / denom;
        if (t < 0.0) return false;

        vec3 hitPoint = ray.origin + ray.direction * t;
        
        // 修正坐标系构建逻辑
        vec3 right, forward;
        if (abs(obj.normal.y) > 0.9) { 
            // 法线接近Y轴（如地面/天花板）
            right = normalize(cross(obj.normal, vec3(0,0,1)));
            forward = normalize(cross(right, obj.normal));
        } else { 
            // 法线接近X/Z轴（如墙面）
            right = normalize(cross(obj.normal, vec3(0,1,0)));
            forward = normalize(cross(right, obj.normal));
        }

        // 计算局部偏移
        vec3 localOffset = hitPoint - obj.position;
        float x = dot(localOffset, right);
        float z = dot(localOffset, forward);

        // 检查尺寸范围（size.x=宽度，size.y=高度）
        if (abs(x) > obj.size.x/2.0 || abs(z) > obj.size.y/2.0) {
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
        // 先检测AABB
        float boxTMin, boxTMax;
        if(!intersectAABB(ray, obj.bounds, boxTMin, boxTMax)) continue;
        
        // 再检测具体形状
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
void generateCameraRay(out Ray ray, vec2 jitter) {
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imageSize = imageSize(outputImage);
    
    // 计算屏幕坐标（-1到1范围）
    // 添加抖动偏移
    vec2 uv = (vec2(pixelCoords) + 0.5 + jitter) / vec2(imageSize);
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

float haltonSequence(int index, int base) {
    float result = 0.0;
    float f = 1.0 / base;
    int i = index;
    while(i > 0) {
        result += f * (i % base);
        i = i / base;
        f = f / base;
    }
    return result;
}

// 修改区域光源采样函数
vec3 sampleAreaLight(Light light, vec3 point, vec2 rand, out float pdf) {
    // 极坐标采样（余弦加权）
    float theta = 2.0 * PI * rand.x;
    float r = sqrt(rand.y);// 均匀分布
    
    // 构建光源坐标系
    vec3 normal = normalize(-light.direction); // 修正方向
    vec3 tangent = normalize(cross(normal, vec3(0,1,0)));
    if(length(tangent) < 0.1) 
        tangent = normalize(cross(normal, vec3(1,0,0)));
    vec3 bitangent = normalize(cross(normal, tangent));
    
    // 生成采样点
    vec2 disk = r * vec2(cos(theta), sin(theta));
    vec3 samplePos = light.position + 
                    light.radius * (disk.x * tangent + disk.y * bitangent);
    
    // 计算概率密度函数(PDF)（面积倒数）
    float area = light.radius * light.radius * PI; // 正确面积计算
    pdf = 1.0 / (area * light.samples);

    return samplePos;
}


// 通用PCF实现
float pcfShadow(vec3 point, vec3 normal, Light light, vec3 lightDir, float lightDistance) {
    float shadow = 0.0;
    float filterSize = light.shadowSoftness * 0.005;
    vec3 tangent, bitangent;

    // 构建切线空间
    if(light.type == 1) { // 定向光
        tangent = normalize(cross(lightDir, vec3(0,1,0)));
        bitangent = cross(lightDir, tangent);
        filterSize = light.angularRadius * lightDistance;
    } else { // 点光源/区域光
        tangent = normalize(cross(lightDir, vec3(0,1,0)));
        bitangent = cross(lightDir, tangent);
    }

    for(int i = 0; i < light.pcfSamples; i++) {
        vec2 rand = vec2(haltonSequence(i, 2) * 2.0 - 1.0);
        vec3 jitteredDir;

        if(light.type == 1) { // 定向光
            jitteredDir = lightDir + 
                rand.x * tangent * filterSize +
                rand.y * bitangent * filterSize;
        } else { // 点光源/区域光
            jitteredDir = normalize(lightDir + 
                rand.x * tangent * filterSize +
                rand.y * bitangent * filterSize);
        }

        Ray shadowRay;
        shadowRay.origin = point + normal * 0.001;
        shadowRay.direction = jitteredDir;
        shadowRay.depth = 0;

        Material tempMat;
        vec3 tempNormal;
        float t;
        bool isOccluded = intersectObjects(shadowRay, tempMat, tempNormal, t);

        if(light.type == 0 || light.type == 2) { // 点/区域光源需要距离判断
            isOccluded = isOccluded && (t < lightDistance);
        }

        shadow += isOccluded ? 0.0 : 1.0;
    }
    return shadow / light.pcfSamples;
}

// 通用PCSS实现
float pcssShadow(vec3 point, vec3 normal, Light light, vec3 lightDir, float lightDistance) {
    // 步骤1：遮挡物深度估计
    float avgBlockerDepth = 0.0;
    int blockerCount = 0;
    float searchSize = light.lightSize * 0.1;

    for(int i = 0; i < 16; i++) {
        vec2 rand = vec2(haltonSequence(i, 3) * 2.0 - 1.0);
        vec3 sampleDir = lightDir + rand.x * searchSize + rand.y * searchSize;

        Ray shadowRay;
        shadowRay.origin = point + normal * 0.001;
        shadowRay.direction = normalize(sampleDir);
        shadowRay.depth = 0;

        Material tempMat;
        vec3 tempNormal;
        float t;
        bool isOccluded = intersectObjects(shadowRay, tempMat, tempNormal, t);

        if(light.type != 1) { // 非定向光需要距离判断
            isOccluded = isOccluded && (t < lightDistance);
        }

        if(isOccluded) {
            avgBlockerDepth += t;
            blockerCount++;
        }
    }

    if(blockerCount == 0) return 1.0;
    avgBlockerDepth /= blockerCount;

    // 步骤2：计算半影尺寸
    float penumbraSize = (avgBlockerDepth - lightDistance) * light.lightSize;
    penumbraSize *= light.shadowSoftness;

    // 步骤3：动态PCF
    return pcfShadow(point, normal, light, lightDir, lightDistance);
}

// 统一阴影计算函数
float calculateShadow(vec3 point, vec3 normal, Light light) {
    if(light.shadowType == 0) return 1.0;
    
    float shadow = 0.0;
    vec3 lightDir;
    float lightDistance;

    // 计算基础光照参数
    if(light.type == 0) { // 点光源
        lightDir = light.position - point;
        lightDistance = length(lightDir);
        lightDir = normalize(lightDir);
    } else if(light.type == 1) { // 定向光
        lightDir = normalize(-light.direction);
        lightDistance = 1e6;
    } else { // 区域光
        lightDir = normalize(light.position - point);
        lightDistance = length(light.position - point);
    }

    // 阴影类型处理
    if(light.shadowType == 1) { // PCF
        shadow = pcfShadow(point, normal, light, lightDir, lightDistance);
    } else if(light.shadowType == 2) { // PCSS
        shadow = pcssShadow(point, normal, light, lightDir, lightDistance);
    }

    return shadow;
}

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
            float lightCos = max(dot(lightDir, lightNormal), 0.0); // 移除负号
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

    vec2 jitter = vec2(
            random(pixelCoords + vec2(1, 0)),
            random(pixelCoords + vec2(0, 1))
        ) - 0.5;

    Ray ray;
    generateCameraRay(ray, jitter);
    
    vec3 finalColor = vec3(0.0);
    vec3 throughput = vec3(1.0);
    
    vec3 P;
    vec3 V;
    vec3 N;

    for(int depth = 0; depth < MAX_RAY_DEPTH; ++depth) {
        Material mat;
        float t;
        
        if(!intersectObjects(ray, mat, N, t)) {
            if(useSkybox) finalColor += throughput * texture(skybox, ray.direction).rgb;
            else finalColor += throughput * vec3(0.0);
            break;
        }
        
        P = ray.origin + ray.direction * t;
        V = normalize(-ray.direction);
        
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
    imageStore(gPosition, pixelCoords, vec4(P, 1.0));
    imageStore(gNormal, pixelCoords, vec4(N, 1.0));
}