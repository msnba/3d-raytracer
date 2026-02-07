#version 430 core

//-- Data --
out vec4 FragColor;
in vec2 TexCoords;

uniform vec2 resolution;
uniform vec3 cameraPos;
uniform vec3 cameraFront;
uniform vec3 cameraUp;

// frame accumulation
uniform sampler2D previousFrame;
uniform uint frameIndex;

// -- Structs --

struct Material {
    vec3 color;
    float smoothness;
    vec4 emission; //rgb + strength
};

struct Ray {
    vec3 origin;
    vec3 direction;
};

struct Collision {
    bool didHit;
    float distance;
    vec3 hitPoint;
    vec3 normal;
    Material material;
};

struct Triangle {
    vec4 a, b, c;
    vec4 n0, n1, n2;
};

struct Mesh {
    uvec4 data; // firstTriangle, triangleCount, materialIdx, pad
    vec4 minBounds;
    vec4 maxBounds;
};

struct Sphere {
    vec3 pos;
    float radius;
    Material material;
};

// -- SSBOs --

struct SceneData {
    vec2 rayBehavior; // maxbounce, numraysperpixel
};

layout(std430, binding = 0) buffer Spheres {
    Sphere spheres[];
};

layout(std430, binding = 1) buffer Materials {
    Material materials[];
};

layout(std430, binding = 2) buffer Triangles {
    Triangle triangles[];
};

layout(std430, binding = 3) buffer Meshes {
    Mesh meshes[];
};

layout(std140, binding = 4) buffer Data {
    SceneData sceneData;
};

// -- Functions --

Collision raySphere(Ray ray, Sphere s){
    Collision collision;
    collision.didHit = false;

    vec3 oc = s.pos - ray.origin;

    float closestApproach = dot(oc, ray.direction);
    if(closestApproach < 0) return collision; //sphere behind ray

    float distRay2 = dot(oc, oc) - closestApproach * closestApproach;
    float r2 = s.radius * s.radius;

    if(distRay2 > r2) return collision; //if distance to ray greater to radius, miss

    collision.distance = closestApproach - sqrt(r2 - distRay2); //closest - distance from closest to sphere surface
    collision.didHit = true;
    collision.hitPoint = ray.origin + ray.direction * collision.distance;
    collision.normal = (collision.hitPoint - s.pos) / s.radius; // cheaper normal
    collision.material = s.material;
    return collision;
}

bool rayAABB(Ray ray, vec3 minB, vec3 maxB){
    vec3 invDir = 1.0 / ray.direction; //precompute cuz f division

    vec3 t0 = (minB - ray.origin) * invDir;
    vec3 t1 = (maxB - ray.origin) * invDir;

    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);

    return min(min(tmax.x, tmax.y), tmax.z) >= max(max(max(tmin.x, tmin.y), tmin.z), 0.0); // this looks scary but just computes if max is bigger than min dw
}

// Moller-Trumbore algorithm
// https://en.wikipedia.org/wiki/Moller-Trumbore_intersection_algorithm
// adapted from https://stackoverflow.com/a/42752998
Collision rayTriangle(Ray ray, Triangle tri, Material mat){
    Collision c;
    c.didHit = false;

    vec3 edge1 = tri.b.xyz - tri.a.xyz;
    vec3 edge2 = tri.c.xyz - tri.a.xyz;
    vec3 normalVec = cross(edge1, edge2);
    float det = -dot(ray.direction, normalVec);
    float invdet = 1.0f/det;
    vec3 ao = ray.origin - tri.a.xyz;
    vec3 dao = cross(ao, ray.direction);
    float u = dot(edge2, dao) * invdet;
    float v = -dot(edge1, dao) * invdet;
    float dist = dot(ao, normalVec) * invdet;
    float w = 1 - u - v;

    c.didHit = det>=1E-6 && dist >= 0 && u >= 0 && v >= 0 && w >= 0;
    c.hitPoint = ray.origin + ray.direction * dist;
    c.normal = normalize(normalVec);
    c.distance = dist;
    c.material = mat;

    return c;
}

Collision calculateRayCollision(Ray ray)
{
    Collision closest;
    closest.didHit = false;
    closest.distance = 1e30; // very large distance as a default

    for(int i = 0; i < spheres.length(); i++){
        Sphere s = spheres[i];
        Collision current = raySphere(ray, s);

        if(current.didHit && current.distance < closest.distance){
            closest = current;
            closest.material = s.material;
        }
    }

    for(int i=0; i < meshes.length(); i++){
        Mesh mesh = meshes[i];

        if(!rayAABB(ray, mesh.minBounds.xyz, mesh.maxBounds.xyz)) continue;

        for(uint j = mesh.data.x; j < mesh.data.x + mesh.data.y; j++){
            Triangle tri = triangles[j];
            Material mat = materials[mesh.data.z];

            Collision current = rayTriangle(ray, tri, mat);
            if(current.didHit && current.distance < closest.distance){
                closest = current;
            }
        }
    }
    return closest;
}

float randomFloat(inout uint rng){
    rng = rng * 747796405u + 2891336453u;
    uint result = ((rng >> ((rng >> 28u) + 4u)) ^ rng) * 277803737u;
    result = (result >> 22u) ^ result;

    return float(result) / 4294967295.0;
}

// PCG rng
float randomNormalDistribution(inout uint rng){
    float u = randomFloat(rng);

    // normal dist
    float theta = 2 * 3.1415926 * u;
    float rho = sqrt(-2 * log(u));
    return rho * cos(theta);
}

vec3 cosineHemisphereDirection(vec3 normal, inout uint rng){ // removes diffuse bias
    float u1 = randomFloat(rng);
    float u2 = randomFloat(rng);

    float r = sqrt(u1);
    float theta = 2.0 * 3.1415926 * u2;

    vec3 tangent = normalize(abs(normal.x) > 0.1 ? cross(vec3(0,1,0), normal) : cross(vec3(1,0,0), normal));

    return normalize(
        r * cos(theta) * tangent + 
        r * sin(theta) * cross(normal, tangent) +
        sqrt(1.0 - u1) * normal
    );
}

vec3 ambient(Ray ray){
    // this can be over-complicated later
    return vec3(0.00);
}

vec3 trace(Ray ray, inout uint rng){
    vec3 incomingLight = vec3(0);
    vec3 rayColor = vec3(1.0f);

    for(int i=0; i <= sceneData.rayBehavior.x; i++){
        Collision collision = calculateRayCollision(ray);
        if(!collision.didHit){
            incomingLight += ambient(ray);
            break;
        }
        ray.origin = collision.hitPoint + collision.normal * 0.0005;

        vec3 diffuseDir = cosineHemisphereDirection(collision.normal, rng);
        vec3 specularDir = reflect(ray.direction, collision.normal);

        ray.direction = normalize(mix(diffuseDir, specularDir, collision.material.smoothness));
        incomingLight += collision.material.emission.rgb * collision.material.emission.a * rayColor;

        rayColor *= collision.material.color.rgb;

        // russian roulette
        if (i > 3){
            float p = max(rayColor.r, max(rayColor.g, rayColor.b));
            p = clamp(p, 0.05, 0.95);

            if(randomFloat(rng) > p) break;

            rayColor /= p;
        }
    }

    return incomingLight;
}

uint hash(uint x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
};

void main()
{
    vec2 uv = TexCoords; // [0,1]
    vec2 screen = uv - 0.5;
    screen.x *= resolution.x / resolution.y; // aspect correction
    uvec2 numPixels = uvec2(resolution);
    uvec2 pixelCoord = uvec2(uv * vec2(numPixels));

    
    // uint rng = pixelCoord.y * numPixels.x + pixelCoord.x + frameIndex * 9781u;
    // uint rng = pixelCoord.y * numPixels.x + pixelCoord.x;
    uint rng = hash((pixelCoord.y * numPixels.x + pixelCoord.x) ^ hash(frameIndex)); //slap a little hashing on it and call it a day

    vec3 forward = normalize(cameraFront);
    vec3 right   = normalize(cross(forward, cameraUp));
    vec3 up      = cross(right, forward);

    Ray ray;
    ray.origin = cameraPos;
    vec3 focusPoint =
        cameraPos +
        forward +
        screen.x * right +
        screen.y * up;

    
    ray.direction = normalize(focusPoint - ray.origin);

    vec3 totalLight = vec3(0);
    for(int i = 0; i < sceneData.rayBehavior.y; i++){
        rng = uint(randomFloat(rng) * 4294967295.0);
        totalLight += trace(ray, rng);
    }
    totalLight /= sceneData.rayBehavior.y; //average

    float weight = 1.0/(frameIndex+1);

    FragColor = vec4(texture(previousFrame, uv).rgb * (1 - weight) + totalLight * weight, 1.0f);
}
