//move to another uniform?
layout(push_constant) uniform uPushConstant {
   mat4 world;
   int uAlbedo;
} pc;

layout(set = 0, binding = 0) uniform SceneBuffer {   
	mat4 viewProj[2]; 
} sceneData;

//testing constant_id/shader specialization
layout(constant_id = 0) const int specializationTest = 0;

