#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0)in struct FragmentIn
{
    vec4 vertexWorldPos;
    vec4 vertexNormal;
    vec4 vertexUV;
}
i;


layout(location = 3)flat in vec4 cameraPosition;

layout(location = 0)out vec4 color;

layout(set = 0, binding = 1)uniform sampler2D textures[8];

void main()
{
    vec3 l = normalize(cameraPosition.xyz - i.vertexWorldPos.xyz);
    float nDotL = max(dot(i.vertexNormal.xyz, l), 0.0f);
    color = nDotL * texture(textures[floatBitsToUint(cameraPosition.w)], i.vertexUV.xy);
}
