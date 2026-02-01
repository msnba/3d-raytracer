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

// frame accumulation
uniform sampler2D previousFrame;
uniform uint frameIndex;

struct Ray {
    vec3 origin;
    vec3 direction;
};

struct Material {
    vec3 color;
    float smoothness;
    vec4 emission; //rgb + strength
};

struct Triangle {
    vec4 a, b, c;
    vec4 n0, n1, n2;
};

struct Mesh {
    uint firstTriangle;
    uint triangleCount;
    uint materialIdx;
};

struct Sphere {
    vec3 pos;
    float radius;
    Material material;
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

// Moller-Trumbore algorithm
// https://en.wikipedia.org/wiki/Moller-Trumbore_intersection_algorithm
// adapted by https://stackoverflow.com/a/42752998
Collision rayTriangle(Ray ray, Triangle tri, Material mat){
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

    Collision c;
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

    for(int i = 0; i < sphereCount; i++){
        Sphere s = spheres[i];
        Collision current = raySphere(ray, s.pos, s.radius);

        if(current.didHit && current.distance < closest.distance){
            closest = current;
            closest.material = s.material;
        }
    }

    for(int i=0; i < meshes.length(); i++){
        Mesh mesh = meshes[i];
        for(uint j = mesh.firstTriangle; j < mesh.firstTriangle + mesh.triangleCount; j++){
            Triangle tri = triangles[j];
            Material mat = materials[mesh.materialIdx];

            Collision current = rayTriangle(ray, tri, mat);
            if(current.didHit && current.distance < closest.distance){
                closest = current;
            }
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

        // vec3 diffuseDir = randomHemisphereDirection(collision.normal, rng);
        vec3 diffuseDir = normalize(randomHemisphereDirection(collision.normal, rng));
        vec3 specularDir = reflect(ray.direction, collision.normal);
        ray.direction = mix(diffuseDir, specularDir, collision.material.smoothness);

        incomingLight += collision.material.emission.rgb * collision.material.emission.a * rayColor;

        // rayColor *= collision.material.color.rgb * dot(collision.normal, ray.direction) * 1.5; //lambert's cosine law
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
    uint rng = pixelCoord.y * numPixels.x + pixelCoord.x + frameIndex * 9781u;
    // uint rng = pixelCoord.y * numPixels.x + pixelCoord.x;

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

    float effectiveFrame = min(float(frameIndex), 150.0); //puts a ceiling on the amount of accumulated frames
    // effectiveFrame = frameIndex;

    vec3 prev = texture(previousFrame, uv).rgb;
    float weight = 1.0/(effectiveFrame+1);

    FragColor = vec4(mix(prev, totalLight, weight), 1.0f);
}
