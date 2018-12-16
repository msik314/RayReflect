#version 450
#extension GL_ARB_separate_shader_objects : enable

#define EPSILON 0.01

struct StorageVertex
{
    vec4 positionU;
    vec4 normalV;
    ivec4 textureUnit;
};

struct Triangle
{
    int verts[3];
    float dist;
};

layout(constant_id = 0)const uint numObjs = 1;
layout(constant_id = 1)const uint numVerts = 3;
layout(constant_id = 2)const uint numTris = 1;

layout(input_attachment_index = 0, set = 0, binding = 0)uniform subpassInput inColor;
layout(input_attachment_index = 1, set = 0, binding = 1)uniform subpassInput posNY;
layout(input_attachment_index = 2, set = 0, binding = 2)uniform subpassInput nXZ;

layout(set = 0, binding = 3)uniform CamPos
{
    vec3 cameraPosition;
};

layout(set = 0, binding = 4)uniform sampler2D textures[8];

layout(location = 0)out vec4 color;

layout(std430, set = 1, binding = 0)buffer VertexBuffer
{
    StorageVertex verts[numVerts];
};

layout(std430, set = 1, binding = 1)buffer IndexBuffer
{
    Triangle tris[numTris];
};

vec3 comb(vec3 a, vec3 b, vec3 c, vec3 m)
{
    return a * m.x + b * m.y + c * m.z;
}

float comb(float a, float b, float c, vec3 m)
{
    return a * m.x + b * m.y + c * m.z;
}

bool intersect(vec3 origin, vec3 direction, StorageVertex tri[3], out float r, out vec3 bary)
{
    vec3 e1 = tri[1].positionU.xyz - tri[0].positionU.xyz; 
    vec3 e2 = tri[2].positionU.xyz - tri[0].positionU.xyz;
    vec3 p = cross(direction, e2);
    float det = dot(e1, p);
    if(abs(det) < EPSILON)
    {
        bary = vec3(0, 0, 0);
        return false;
    }
    
    float invDet = 1/det;
    
    vec3 t = origin - tri[0].positionU.xyz;
    vec3 q = cross(t, e1);
    
    bary.y = dot(t, p) * invDet;
    bary.z = dot(direction, q) * invDet;
    bary.x = 1 - bary.y - bary.z;
    
    r = dot(e2, q) * invDet;
    
    return r > EPSILON && bary.x >= 0 && bary.y >= 0 && bary.z >= 0;
}

void main()
{
    vec4 pos = subpassLoad(posNY);
    vec2 norm = subpassLoad(nXZ).xy;
    vec4 loadedColor = subpassLoad(inColor);
    vec3 worldPos = pos.xyz;
    vec3 worldNorm = vec3(norm.x, pos.w, norm.y);
    vec3 l = normalize(cameraPosition - worldPos);
    
    
    vec4 castColor = loadedColor;
    
    StorageVertex triVerts[3];
    
    if(loadedColor.xyz == vec3(0) && worldPos == vec3(0) && worldNorm == vec3(0))
    {
        discard;
        return;
    }
    
    vec3 rayDir = normalize(worldPos - cameraPosition);
    if(dot(rayDir, worldNorm) > 0)
    {
        color = vec4(max(dot(worldNorm, l), 0.0f) * loadedColor.xyz, 1);
        return;
    }
    
    vec3 refDir = reflect(rayDir, worldNorm);
    
    vec3 bary;
    vec3 minBary;
    float minR = 100000;
    float r;
    int idx = -1;
    vec3 avgNormal;
    
    for(int i = 0; i < numTris; ++i)
    {
        triVerts[0] = verts[tris[i].verts[0]];
        triVerts[1] = verts[tris[i].verts[1]];
        triVerts[2] = verts[tris[i].verts[2]];
        
        if(intersect(worldPos, refDir, triVerts, r, bary) && r < minR)
        {
            avgNormal = comb(triVerts[0].normalV.xyz, triVerts[1].normalV.xyz, triVerts[2].normalV.xyz, vec3(0.333));
            if(dot(avgNormal, refDir) < 0)
            {
                idx = i;
                minR = r;
                minBary = bary;
            }
        }
    }
    
    int hitTex;
    vec2 texCoord;
    vec2 texUV;
    vec3 hitNormal;
    float nDotL;
    
    if(idx > 0)
    {
        triVerts[0] = verts[tris[idx].verts[0]];
        triVerts[1] = verts[tris[idx].verts[1]];
        triVerts[2] = verts[tris[idx].verts[2]];
        hitTex = triVerts[0].textureUnit.x;
        texUV.x = comb(triVerts[0].positionU.w, triVerts[1].positionU.w, triVerts[2].positionU.w, minBary);
        texUV.y = comb(triVerts[0].normalV.w, triVerts[1].normalV.w, triVerts[2].normalV.w, minBary);
        hitNormal = normalize(comb(triVerts[0].normalV.xyz, triVerts[1].normalV.xyz, triVerts[2].normalV.xyz, bary));
        nDotL = max(dot(hitNormal, l), 0);
        nDotL = max(nDotL, -dot(hitNormal, refDir));
        castColor = nDotL * vec4(texture(textures[hitTex], texUV).xyz, 1);
    }
    nDotL = max(dot(worldNorm, l), 0.0f);
    color = nDotL * mix(loadedColor, castColor, 0.5);
    color.w = 1;
}
