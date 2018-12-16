#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0)in struct FragmentIn
{
    vec4 vertexWorldPos;
    vec4 vertexNormal;
    vec4 vertexUV;
}
i;

layout(location = 3)flat in int textureUnit;

layout(location = 0)out vec4 color;
layout(location = 1)out vec4 position;
layout(location = 2)out vec2 normal;

layout(set = 0, binding = 1)uniform sampler2D textures[8];

void main()
{
    vec3 n = normalize(i.vertexNormal.xyz);
    position = vec4(i.vertexWorldPos.xyz, n.y);
    normal = n.xz;
    color = texture(textures[textureUnit], i.vertexUV.xy);
}
