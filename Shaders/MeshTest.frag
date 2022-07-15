#version 450 core
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable
#include "test.inc"

layout(location = 0) in struct {
    vec4 Color;
    vec2 UV;
} In;

layout(set = 1, binding = 0) uniform sampler2D sTextures[];

layout(location = 0) out vec4 fColor;

const float gAlphaClip = 0.5f;

void main() {
    fColor = texture(sTextures[pc.uAlbedo], In.UV.st) * In.Color;

    //ATI texture does not come with blue channel?
    //if(normalTex.b == 0) {
    //    normalTex.b = 1;
    //    normalTex.rgb = normalize(normalTex.rgb);
    //}

    if(fColor.a <= gAlphaClip) {
        discard;
    }

    //fColor *= vec4(floor(In.UV.xyx * 8.0f) / 8.0f, 1);
    //vec3 val = floor(In.UV.xyx * 8.0f) / 8.0f;
    //fColor = vec4(val, 1);
}