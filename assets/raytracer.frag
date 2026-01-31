#version 450 core
out vec4 FragColor;
in vec2 TexCoords;

uniform vec3 cameraPos;
uniform vec3 cameraFront;
uniform vec3 cameraUp;
uniform vec2 resolution;

struct Ray {
    vec3 origin;
    vec3 direction;
};

struct Sphere {
    vec3 pos;
    float radius;
    vec3 color;
    float pad; // temp alignment so the Sphere reaches 32 bytes
};

layout(std430, binding = 0) buffer Spheres { //for receiving spheres
    Sphere spheres[];
};

uniform int sphereCount;

struct Collision {
    bool didHit;
    float distance;
    vec3 hitPoint;
    vec3 normal;
    vec3 color;
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
    closest.distance = 1e30; // very large distance
    closest.didHit = false;

    for(int i = 0; i < sphereCount; i++){
        Sphere s = spheres[i];
        Collision current = raySphere(ray, s.pos, s.radius);

        if(current.didHit && current.distance < closest.distance){
            closest = current;
            closest.color = s.color;
        }
    }

    return closest;
}

void main()
{
    vec2 ndc = TexCoords; // [0,1]
    ndc.x *= resolution.x / resolution.y; // aspect correction

    vec3 forward = normalize(cameraFront);
    vec3 right   = normalize(cross(forward, cameraUp));
    vec3 up      = cross(right, forward);

    Ray ray;
    ray.origin = cameraPos;
    ray.direction = normalize(forward + ndc.x*right + ndc.y*up);

    // FragColor = vec4(sphere(ray, vec3(0,0,-5), 1.0).didHit,float(sphere(ray, vec3(0,0,-5), 1.0).didHit) * .75f,float(sphere(ray, vec3(0,0,-5), 1.0).didHit) * .5f,1);

    FragColor = vec4(calculateRayCollision(ray).color, 1);
}
