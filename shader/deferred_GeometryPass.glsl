#version 430 core

// �������ݹ��������
#define MAX_RAY_DEPTH 1 
const float PI = 3.14159265359;

struct AABB {
    vec3 min;
    vec3 max;
};

struct Ray {
    vec3 origin;
    vec3 direction;
    float energy;    // ��������������˥����
    int depth;       // �ݹ����
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
    int type;        // 0=����, 1=ƽ��
    vec3 position;
    float radius;   // ����
    vec3 normal;    // ƽ��
    vec2 size;      // ƽ��
    Material material;
    AABB bounds;
};

layout(local_size_x = 32, local_size_y = 32) in;
layout(rgba32f, binding = 0) uniform image2D gPosition;
layout(rgba16f, binding = 1) uniform image2D gNormal;
layout(rgba16f, binding = 2) uniform image2D gMaterial;
layout(rgba8, binding = 3) uniform image2D gAlbedo;

layout(std430, binding = 0) buffer Objects {
    Object objects[];
};

uniform int numObjects;
uniform int numLights;

uniform vec3 cameraPos;
uniform vec3 cameraDir;
uniform vec3 cameraUp;
uniform vec3 cameraRight;
uniform float fov = 60.0;          // ��ֱ�ӳ��ǣ��Ƕ��ƣ�
uniform float focalLength = 1.0;   // ����

uniform float maxRayDistance = 114514.0; // ������߾�������

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
        
        // ��������ϵ�����߼�
        vec3 right, forward;
        if (abs(obj.normal.y) > 0.9) { 
            // ���߽ӽ�Y�ᣨ�����/�컨�壩
            right = normalize(cross(obj.normal, vec3(0,0,1)));
            forward = normalize(cross(right, obj.normal));
        } else { 
            // ���߽ӽ�X/Z�ᣨ��ǽ�棩
            right = normalize(cross(obj.normal, vec3(0,1,0)));
            forward = normalize(cross(right, obj.normal));
        }

        // ����ֲ�ƫ��
        vec3 localOffset = hitPoint - obj.position;
        float x = dot(localOffset, right);
        float z = dot(localOffset, forward);

        // ���ߴ緶Χ��size.x=��ȣ�size.y=�߶ȣ�
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
        // �ȼ��AABB
        float boxTMin, boxTMax;
        if(!intersectAABB(ray, obj.bounds, boxTMin, boxTMax)) continue;
        
        // �ټ�������״
        float currentT;
        bool isHit = false;

        // �����ཻ���
        if(obj.type == 0) {
            isHit = intersectSphere(ray, obj, currentT);
        } 
        // ƽ���ཻ���
        else if(obj.type == 1) {
            isHit = intersectPlane(ray, obj, currentT);
        }
        
        if(isHit && currentT > 0.0 && currentT < minT) {
            minT = currentT;
            hit = true;
            t = currentT;
            
            // ��ȡ��������
            hitMaterial = obj.material;
            
            // ���㷨��
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

// ��������
void generateCameraRay(out Ray ray, vec2 jitter) {
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imageSize = imageSize(gPosition);
    
    // ������Ļ���꣨-1��1��Χ��
    // ��Ӷ���ƫ��
    vec2 uv = (vec2(pixelCoords) + 0.5 + jitter) / vec2(imageSize);
    uv = uv * 2.0 - 1.0;
    
    // ����FOV����ʵ����Ļ����
    float aspect = float(imageSize.x) / float(imageSize.y);
    float tanFov = tan(radians(fov) * 0.5);
    uv.x *= aspect * tanFov * focalLength;
    uv.y *= tanFov * focalLength;
    
    ray.origin = cameraPos;
    ray.direction = normalize(cameraDir + uv.x * cameraRight + uv.y * cameraUp);
    ray.energy = 1.0;
    ray.depth = 0;
}

// ����������������ڲ����ֲ���
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
    
    Material finalMat;
    vec3 finalP, finalN;
    bool hasHit = false;

    for(int depth = 0; depth < MAX_RAY_DEPTH; ++depth) {
        Material mat;
        float t;
        vec3 N;
        
        if(intersectObjects(ray, mat, N, t)) {
            finalMat = mat;
            finalP = ray.origin + ray.direction * t;
            finalN = N;
            hasHit = true;
            break; // ����¼�״�����
        }
        
        // ����˹���̶���ֹ����
        if(depth > 2) {
            float continueProb = min(max(throughput.x, max(throughput.y, throughput.z)) * 0.95, 0.95);
            if(random(gl_GlobalInvocationID.xy + depth) > continueProb) break;
            throughput /= continueProb;
        }
        
        ray.energy *= 0.8; // ����˥��
    }
    
    if(hasHit) {
        imageStore(gPosition, pixelCoords, vec4(finalP, 1.0));
        imageStore(gNormal, pixelCoords, vec4(finalN, 1.0));
        imageStore(gMaterial, pixelCoords, vec4(
            finalMat.metallic,
            finalMat.roughness,
            finalMat.transparency,
            finalMat.ior
        ));
        imageStore(gAlbedo, pixelCoords, vec4(finalMat.albedo, 1.0)); // д��Albedo
    } else {
        imageStore(gPosition, pixelCoords, vec4(0));
        imageStore(gNormal, pixelCoords, vec4(0));
        imageStore(gMaterial, pixelCoords, vec4(0));
        imageStore(gAlbedo, pixelCoords, vec4(0));
    }
    
}