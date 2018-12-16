#version 450
#extension GL_ARB_separate_shader_objects : enable

struct Uniforms
{
    mat4 model;
    mat4 vp;
    vec4 cameraPosition;
};

layout(location = 0)in vec4 position;
layout(location = 1)in vec4 normal;
layout(location = 2)in vec4 texCoord;

layout(location = 0)out struct VertexOut
{
    vec4 vertexWorldPos;
    vec4 vertexNormal;
    vec4 vertexUV;
}
o;

layout(location = 3)flat out vec4 cameraPosition;

layout(std140, binding = 0)uniform UniformBuffer
{
    Uniforms data[2];
}
uniformBuffer;

void main()
{
    o.vertexNormal = transpose(inverse(uniformBuffer.data[gl_InstanceIndex].model)) * normal;
    o.vertexUV = texCoord;
    cameraPosition = uniformBuffer.data[gl_InstanceIndex].cameraPosition;
    o.vertexWorldPos = uniformBuffer.data[gl_InstanceIndex].model * vec4(position.xyz, 1.0);
    gl_Position = uniformBuffer.data[gl_InstanceIndex].vp * o.vertexWorldPos;
}
