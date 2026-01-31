#version 450 core
out vec4 FragColor;
in vec2 TexCoords;

uniform vec3 cameraPos;
uniform vec3 cameraFront;
uniform vec3 cameraUp;
uniform vec2 resolution;
uniform uint sphereCount;
uniform uint maxBounce;
uniform uint numRaysPerPixel;

struct Ray {
    vec3 origin;
    vec3 direction;
};

struct Material {
    vec4 color;
    vec4 emission; //rgb + strength
};

struct Sphere {
    vec3 pos;
    float radius;
    Material material;
};

layout(std430, binding = 0) buffer Spheres { //for receiving spheres
    Sphere spheres[];
};



struct Collision {
    bool didHit;
    float distance;
    vec3 hitPoint;
    vec3 normal;
    Material material;
};

Collision raySphere(Ray ray, vec3 center, float radius){
    Collision collision;
    collision.didHit = false;

    vec3 oc = ray.origin - center;
    float a = dot(ray.direction, ray.direction);
    float b = 2.0 * dot(oc, ray.direction);
    float c = dot(oc, oc) - radius * radius;
    float discriminant = b*b - 4.0 * a*c;

    if(discriminant >= 0.0){
        float t = (-b - sqrt(discriminant)) / (2.0 * a);
        if(t >= 0.0){
            collision.didHit = true;
            collision.distance = t;
            collision.hitPoint = ray.origin + ray.direction * t;
            collision.normal = normalize(collision.hitPoint - center);
        }
    }
    return collision;
}

Collision calculateRayCollision(Ray ray)
{
    Collision closest;
    closest.didHit = false;
    closest.distance = 1e30; // very large distance as a default

    for(int i = 0; i < sphereCount; i++){
        Sphere s = spheres[i];
        Collision current = raySphere(ray, s.pos, s.radius);

        if(current.didHit && current.distance < closest.distance){
            closest = current;
            closest.material = s.material;
        }
    }

    return closest;
}

// PCG rng
float randomNormalDistribution(inout uint rng){
    rng = rng * 747796405u + 2891336453u;
    uint result = ((rng >> ((rng >> 28u) + 4u)) ^ rng) * 277803737u;
    result = (result >> 22u) ^ result;

    float u = float(result) / 4294967295.0;

    // normal dist
    float theta = 2 * 3.1415926 * u;
    float rho = sqrt(-2 * log(u));
    return rho * cos(theta);
}

//randomize ray bounce from an object, locked to the normal's hemisphere
vec3 randomHemisphereDirection(vec3 normal, inout uint rng){
    vec3 dir = normalize(vec3(randomNormalDistribution(rng), randomNormalDistribution(rng), randomNormalDistribution(rng)));
    return dir * sign(dot(normal, dir)); //2d dot product
}

vec3 trace(Ray ray, inout uint rng){
    vec3 incomingLight = vec3(0);
    vec3 rayColor = vec3(1.0f);

    for(int i=0;i <= maxBounce; i++){
        Collision collision = calculateRayCollision(ray);
        if(!collision.didHit){break;}
        ray.origin = collision.hitPoint;
        ray.direction = randomHemisphereDirection(collision.normal, rng);

        incomingLight += collision.material.emission.rgb * collision.material.emission.a * rayColor;
        rayColor *= collision.material.color.rgb;
    }

    return incomingLight;
}

void main()
{
    vec2 uv = TexCoords; // [0,1]
    vec2 screen = uv - 0.5;
    screen.x *= resolution.x / resolution.y; // aspect correction
    uvec2 numPixels = uvec2(resolution);
    uvec2 pixelCoord = uvec2(uv * vec2(numPixels));
    uint rng = pixelCoord.y * numPixels.x + pixelCoord.x;

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

    for(int i = 0; i < numRaysPerPixel; i++){
        totalLight += trace(ray, rng);
    }

    totalLight /= numRaysPerPixel; //average

    FragColor = vec4(totalLight, 1);
}
