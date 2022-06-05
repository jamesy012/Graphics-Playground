#version 450 core
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec2 inUV;

layout(set=0, binding=0) uniform sampler2D sTexture;

layout(location = 0) out vec4 fColor;

void main()
{
    fColor = texture(sTexture, inUV);
}