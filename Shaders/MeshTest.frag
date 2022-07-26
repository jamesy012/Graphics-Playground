#version 450 core
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_multiview : enable

#include "test.inc"

layout(set = 1, binding = 0) uniform sampler2D sTextures[];

//todo move this into combined include with a #if SHADER_FRAG
layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inFragPos;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec4 outColor;

void main() {
    const vec4 albedoColor = texture(sTextures[pc.mAlbedoTexture], inUV.st) * inColor * pc.mColorFactor;

    if(albedoColor.a <= pc.mAlphaCutoff) {
        discard;
    }

    const vec2 metallicRoughnessColor = texture(sTextures[pc.mMetallicRoughnessTexture], inUV.st).rg;//ba is unused
    const float metallic = metallicRoughnessColor.r;
    const float roughness = metallicRoughnessColor.g;

        //ATI texture does not come with blue channel?
    //if(normalTex.b == 0) {
    //    normalTex.b = 1;
    //    normalTex.rgb = normalize(normalTex.rgb);
    //}

    vec3 norm = normalize(inNormal);
    vec3 viewDir = normalize(sceneData.mViewPos[gl_ViewIndex].xyz - inFragPos);

    vec3 lightDir = normalize(-sceneData.mDirectionalLight.mDirection.xyz);
    // diffuse shading
    float diff = max(dot(norm, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), max(roughness, 0.01f));

    // combine results
    vec4 ambient = sceneData.mDirectionalLight.mAmbient * albedoColor;
    vec4 diffuse = sceneData.mDirectionalLight.mDiffuse * diff * albedoColor;
    vec4 specular = sceneData.mDirectionalLight.mSpecular * spec * metallic;
    outColor = vec4((ambient + diffuse + specular).xyz, albedoColor.a);

    //fColor *= vec4(floor(In.UV.xyx * 8.0f) / 8.0f, 1);
    //vec3 val = floor(In.UV.xyx * 8.0f) / 8.0f;
    //fColor = vec4(val, 1);
}