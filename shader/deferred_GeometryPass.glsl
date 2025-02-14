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

layout(local_size_x = 32, local_size_y = 32) in;
layout(rgba32f, binding = 0) uniform image2D gPosition;
layout(rgba16f, binding = 1) uniform image2D gNormal;
layout(rgba16f, binding = 2) uniform image2D gMaterail;

layout(std430, binding = 0) buffer Objects {
    Object objects[];
};

uniform int numObjects;
uniform int numLights;

uniform vec3 cameraPos;
uniform vec3 cameraDir;
uniform vec3 cameraUp;
uniform vec3 cameraRight;
uniform float fov = 60.0;          // 垂直视场角（角度制）
uniform float focalLength = 1.0;   // 焦距

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
    ivec2 imageSize = imageSize(gPosition);
    
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

// 随机数生成器（用于采样分布）
float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233)))*43758.5453123);
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
            break;
        }
        
        P = ray.origin + ray.direction * t;
        V = normalize(-ray.direction);
        
        // 俄罗斯轮盘赌终止条件
        if(depth > 2) {
            float continueProb = min(max(throughput.x, max(throughput.y, throughput.z)) * 0.95, 0.95);
            if(random(gl_GlobalInvocationID.xy + depth) > continueProb) break;
            throughput /= continueProb;
        }
        
        ray.energy *= 0.8; // 能量衰减
    }
    
    imageStore(gPosition, pixelCoords, vec4(P, 1.0));
    imageStore(gNormal, pixelCoords, vec4(N, 1.0));
    
}