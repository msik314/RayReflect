#version 450
#extension GL_ARB_separate_shader_objects : enable

struct Uniforms
{
    mat4 model;
    mat4 vp;
    vec4 camTex;
};

struct StorageVertex
{
    vec4 positionU;
    vec4 normalV;
    ivec4 textureUnit;
};

layout(location = 0)in vec4 position;
layout(location = 1)in vec4 normal;
layout(location = 2)in vec4 texCoord;

layout(constant_id = 0)const uint numObjs = 1;
layout(constant_id = 1)const uint numVerts = 3;
layout(constant_id = 2)const uint numTris = 1;

layout(location = 0)out struct VertexOut
{
    vec4 vertexWorldPos;
    vec4 vertexNormal;
    vec4 vertexUV;
}
o;

layout(location = 3)flat out int textureUnit;

layout(std140, binding = 0)uniform UniformBuffer
{
    Uniforms data[numObjs];
}
uniformBuffer;

layout(std430, set = 1, binding = 0)buffer VertexBuffer
{
    StorageVertex verts[numVerts];
};

void main()
{
    StorageVertex sv;
    o.vertexNormal = transpose(inverse(uniformBuffer.data[gl_InstanceIndex].model)) * normal;
    o.vertexUV = texCoord;
    textureUnit = floatBitsToInt(uniformBuffer.data[gl_InstanceIndex].camTex.w);
    o.vertexWorldPos = uniformBuffer.data[gl_InstanceIndex].model * vec4(position.xyz, 1.0);
    sv.positionU = vec4(o.vertexWorldPos.xyz, o.vertexUV.x);
    sv.normalV = vec4(o.vertexNormal.xyz, o.vertexUV.y);
    sv.textureUnit = ivec4(textureUnit);
    
    verts[gl_VertexIndex] = sv;
    
    gl_Position = uniformBuffer.data[gl_InstanceIndex].vp * o.vertexWorldPos;
}
