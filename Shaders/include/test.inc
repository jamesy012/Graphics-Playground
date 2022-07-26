//move to another uniform?
layout(push_constant) uniform uPushConstant {
	mat4 mWorld;
	uint mAlbedoTexture;
	uint mNormalTexture;
	uint mMetallicRoughnessTexture;

	float mAlphaCutoff;
	vec4 mColorFactor;
	vec2 mMetallicRoughness;
	bool mNormalBC5; //bc5 textures are incorrect atm
} pc;

struct DirLight {
	vec4 mDirection;

	vec4 mAmbient;
	vec4 mDiffuse;
	vec4 mSpecular;
};

layout(set = 0, binding = 0) uniform SceneBuffer {
	mat4 mViewProj[2];
	vec4 mViewPos[2]; //pad

	DirLight mDirectionalLight;
} sceneData;

//testing constant_id/shader specialization
layout(constant_id = 0) const int specializationTest = 0;
