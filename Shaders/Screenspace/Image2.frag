#version 450 core

layout (location = 0) in vec2 inUV;

layout(set=0, binding=0) uniform sampler2D sTexture;
layout(set=0, binding=1) uniform sampler2D sTexture2;

layout(location = 0) out vec4 fColor;

void main()
{
    //fColor = vec4(inUV.x, inUV.y, 0, 1);//texture(sTexture, inUV);
    //fColor = texture(sTexture, inUV);
    fColor = texture(sTexture, inUV) * texture(sTexture2, inUV + vec2(0.5));
}