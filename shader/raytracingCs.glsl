#version 430 core

struct Ray {
    vec3 origin;
    vec3 direction;
};

struct Object {
    int type;        // 0=球体, 1=平面
    vec3 position;
    vec3 color;
    float radius;   // 球体
    vec3 normal;    // 平面
    float distance; // 平面
};

layout(local_size_x = 16, local_size_y = 16) in;
layout(rgba32f, binding = 0) uniform image2D outputImage;
layout(std430, binding = 0) buffer Objects {
    Object objects[];
};

uniform vec3 cameraPos;
uniform vec3 cameraDir;
uniform vec3 cameraUp;
uniform vec3 cameraRight;
uniform float fov;
uniform int numObjects;
uniform vec3 BackGroundColor;

vec3 computeLighting(vec3 point, vec3 normal, vec3 color) {
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 viewDir = normalize(cameraPos - point);
    vec3 reflectDir = reflect(-lightDir, normal);

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * color;

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = spec * vec3(1.0);

    return diffuse + specular; 
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

bool intersectObjects(Ray ray, out vec3 hitColor, out vec3 hitNormal, out float t) {
    float minT = 1e6;
    bool hit = false;
    for (int i = 0; i < numObjects; i++) {
        Object obj = objects[i];
        float currentT;
        if ((obj.type == 0 && intersectSphere(ray, obj, currentT)) ||
            (obj.type == 1 && intersectPlane(ray, obj, currentT))) {
            if (currentT < minT) {
                minT = currentT;
                hit = true;
                t = currentT;
                hitColor = obj.color;
                hitNormal = normalize((obj.type == 0) ? (ray.origin + ray.direction * t - obj.position) : obj.normal);
            }
        }
    }
    return hit;
}

void main() {
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imageSize = imageSize(outputImage);

    vec2 uv = vec2(pixelCoords) / vec2(imageSize);
    uv = uv * 2.0 - 1.0;

    Ray ray;
    ray.origin = cameraPos;
    ray.direction = normalize(cameraDir + uv.x * cameraRight + uv.y * cameraUp);

    vec3 hitColor, hitNormal;
    float hitT;

    if (intersectObjects(ray, hitColor, hitNormal, hitT)) {
        vec3 hitPoint = ray.origin + ray.direction * hitT;
        vec3 lighting = computeLighting(hitPoint, hitNormal, hitColor);
        imageStore(outputImage, pixelCoords, vec4(lighting, 1.0));
    } else {
        imageStore(outputImage, pixelCoords, vec4(BackGroundColor, 1.0));
    }
}