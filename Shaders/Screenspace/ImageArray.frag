#version 450 core

layout (location = 0) in vec2 inUV;

layout(set=0, binding=0) uniform sampler2DArray sTextures;

layout(location = 0) out vec4 fColor;

void main()
{
    //fColor = vec4(inUV.x, inUV.y, 0, 1);//texture(sTexture, inUV);
    //fColor = texture(sTexture, inUV);
    vec2 halfUV;// = inUV * vec2(2,1);
    int image = int(inUV.x >= 0.5f);
    if(inUV.x <= 0.5f){
      halfUV = inUV * vec2(2,1);
    }else{
      halfUV = (inUV - vec2(0.5f,0)) * vec2(2,1);
    }
    //halfUV.x += (halfUV.x + vec2(1,0)) * image;
    vec3 uv = vec3(halfUV,image);
    fColor = texture(sTextures, uv);
    //fColor = vec4(uv, 1);
}