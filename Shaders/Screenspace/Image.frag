#version 450 core

layout (location = 0) in vec2 inUV;

layout(set=0, binding=0) uniform sampler2D sTexture;

layout(location = 0) out vec4 fColor;

void main()
{
    //fColor = vec4(inUV.x, inUV.y, 0, 1);//texture(sTexture, inUV);
    fColor = texture(sTexture, inUV);
    //fColor = texture(sTexture, inUV) * vec4(inUV.x, inUV.y, 1, 1);
}