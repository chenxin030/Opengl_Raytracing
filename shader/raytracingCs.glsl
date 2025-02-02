#version 430 core
layout(local_size_x = 16, local_size_y = 16) in;
layout(rgba32f, binding = 0) uniform image2D outputImage;

uniform vec3 cameraPos;
uniform vec3 cameraDir;
uniform vec3 cameraUp;
uniform vec3 cameraRight;
uniform float fov;

struct Ray {
    vec3 origin;
    vec3 direction;
};

struct Sphere {
    vec3 center;
    float radius;
    vec3 color;
};

struct Plane {
    vec3 normal;
    float distance;
    vec3 color;
};

const int numSpheres = 2;
const int numPlanes = 1;
Sphere spheres[numSpheres];
Plane planes[numPlanes];

void initScene() {
    spheres[0] = Sphere(vec3(0.0, 0.0, -5.0), 1.0, vec3(1.0, 0.0, 0.0));
    spheres[1] = Sphere(vec3(2.0, 0.0, -5.0), 1.0, vec3(0.0, 0.0, 1.0));
    planes[0] = Plane(vec3(0.0, 1.0, 0.0), -1.0, vec3(0.5, 0.5, 0.5));
}

bool intersectSphere(Ray ray, Sphere sphere, out float t) {
    vec3 oc = ray.origin - sphere.center;
    float a = dot(ray.direction, ray.direction);
    float b = 2.0 * dot(oc, ray.direction);
    float c = dot(oc, oc) - sphere.radius * sphere.radius;
    float discriminant = b * b - 4.0 * a * c;

    if (discriminant < 0.0) {
        return false;
    } else {
        t = (-b - sqrt(discriminant)) / (2.0 * a);
        return t > 0.0;
    }
}

bool intersectPlane(Ray ray, Plane plane, out float t) {
    float denom = dot(plane.normal, ray.direction);
    if (abs(denom) > 1e-6) {
        vec3 p0 = plane.normal * plane.distance;
        t = dot(p0 - ray.origin, plane.normal) / denom;
        return t >= 0.0;
    }
    return false;
}

bool intersectScene(Ray ray, out vec3 hitColor, out vec3 hitNormal, out float hitT) {
    float minT = 1e6;
    bool hit = false;

    for (int i = 0; i < numSpheres; i++) {
        float t;
        if (intersectSphere(ray, spheres[i], t) && t < minT) {
            minT = t;
            hitColor = spheres[i].color;
            hitNormal = normalize(ray.origin + ray.direction * t - spheres[i].center);
            hit = true;
        }
    }

    for (int i = 0; i < numPlanes; i++) {
        float t;
        if (intersectPlane(ray, planes[i], t) && t < minT) {
            minT = t;
            hitColor = planes[i].color;
            hitNormal = planes[i].normal;
            hit = true;
        }
    }

    hitT = minT;
    return hit;
}

vec3 computeLighting(vec3 point, vec3 normal, vec3 color) {
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 viewDir = normalize(cameraPos - point);
    vec3 reflectDir = reflect(-lightDir, normal);

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * color;
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = spec * vec3(1.0);

    return diffuse + specular;  // 直接返回光照结果
}

void main() {
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imageSize = imageSize(outputImage);

    vec2 uv = vec2(pixelCoords) / vec2(imageSize);
    uv = uv * 2.0 - 1.0;

    initScene();

    Ray ray;
    ray.origin = cameraPos;
    ray.direction = normalize(cameraDir + uv.x * cameraRight + uv.y * cameraUp);

    vec3 hitColor, hitNormal;
    float hitT;
    if (intersectScene(ray, hitColor, hitNormal, hitT)) {
        vec3 hitPoint = ray.origin + ray.direction * hitT;
        vec3 lighting = computeLighting(hitPoint, hitNormal, hitColor);
        imageStore(outputImage, pixelCoords, vec4(lighting, 1.0));
    } else {
        imageStore(outputImage, pixelCoords, vec4(0.0, 0.0, 0.0, 1.0));
    }
}