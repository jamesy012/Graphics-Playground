#version 450 core

layout(location = 0) in struct {
    vec2 UV;
} In;

layout(set = 1, binding = 0) uniform sampler2D sTexture;

layout(location = 0) out vec4 fColor;

void main() {
    fColor = texture(sTexture, In.UV.st);
    fColor *= vec4(floor(In.UV.xyx * 8.0f) / 8.0f, 1);
    //vec3 val = floor(In.UV.xyx * 8.0f) / 8.0f;
    //fColor = vec4(val, 1);
}