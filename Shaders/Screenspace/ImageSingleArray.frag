#version 450 core
//copies one Texture to another 
//array sampler version

layout (location = 0) in vec3 inUV;

layout(set=0, binding=0) uniform sampler2DArray sTextures;

layout(location = 0) out vec4 fColor;

void main()
{
    fColor = texture(sTextures, inUV);
}