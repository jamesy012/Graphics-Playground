#version 450 core
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in struct {
    vec2 UV;
} In;

//layout(set = 0, binding = 0) uniform sampler2D sTexture;

layout(location = 0) out vec4 fColor;

void main() {
    //fColor = In.Color * texture(sTexture, In.UV.st);
    fColor = vec4(In.UV.st, 1, 1);
}