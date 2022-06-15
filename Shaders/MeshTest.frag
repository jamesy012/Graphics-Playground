#version 450 core
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in struct {
    vec2 UV;
} In;

//layout(set = 0, binding = 0) uniform sampler2D sTexture;

layout(location = 0) out vec4 fColor;

void main() {
    //fColor = In.Color * texture(sTexture, In.UV.st);
    //vec3 val = vec3(floor(In.UV.x * 8.0f) / 8.0f, floor(In.UV.y * 8.0f) / 8.0f, floor(In.UV.x * 8.0f) / 8.0f);
    vec3 val = floor(In.UV.xyx * 8.0f) / 8.0f;
    fColor = vec4(val, 1);
}