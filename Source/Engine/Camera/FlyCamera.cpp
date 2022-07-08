#include "FlyCamera.h"

#include "Engine/Input.h"
#include "Engine/Engine.h" //dt
#include "Engine/Window.h" //focus

void FlyCamera::Update() {

	glm::vec3 movement = glm::vec3(0);
	glm::vec3 rotation = glm::vec3(0);
	const float dt = gEngine->GetDeltaTime();
	const float speed = 5 * dt;
	const float rotationSpeed = 100 * dt;
	const float mouseRotationSpeed = 50 * dt;

	movement.x -= gInput->IsKeyDown(GLFW_KEY_A);
	movement.x += gInput->IsKeyDown(GLFW_KEY_D);
	movement.z -= gInput->IsKeyDown(GLFW_KEY_W);
	movement.z += gInput->IsKeyDown(GLFW_KEY_S);
	movement.y -= gInput->IsKeyDown(GLFW_KEY_Q);
	movement.y += gInput->IsKeyDown(GLFW_KEY_E);

	mTransform.TranslateLocal(movement * speed);

	rotation.y += gInput->IsKeyDown(GLFW_KEY_LEFT);
	rotation.y -= gInput->IsKeyDown(GLFW_KEY_RIGHT);
	rotation.x += gInput->IsKeyDown(GLFW_KEY_UP);
	rotation.x -= gInput->IsKeyDown(GLFW_KEY_DOWN);
	//rotation.z += gInput->IsKeyDown(GLFW_KEY_LEFT_BRACKET);
	//rotation.z -= gInput->IsKeyDown(GLFW_KEY_RIGHT_BRACKET);

	{
		//mTransform.Rotate(rotation * rotationSpeed);
		//glm::quat pitch = glm::angleAxis(glm::radians(rotation.x), glm::vec3(1, 0, 0));
		//glm::quat yaw = glm::angleAxis(glm::radians(rotation.y), glm::vec3(0, 1, 0));
		//mTransform.Rotate(pitch*yaw);
		//glm::quat rot = mTransform.GetLocalRotation();
		//rot = rot * glm::angleAxis(glm::radians(rotation.x), glm::vec3(1, 0, 0));
		//rot = glm::angleAxis(glm::radians(rotation.y), glm::vec3(0, 1, 0)) * rot;
		//mTransform.SetRotation(rot);
		mTransform.RotateAxis(rotation * rotationSpeed);
	}

	if(gEngine->GetWindow()->IsLocked()) {
		glm::vec2 delta = -gInput->GetMouseDelta() * mouseRotationSpeed;
		delta = glm::vec2(delta.y, delta.x);//mouse is swapped
		//glm::quat yaw = glm::angleAxis(delta.x)
		//mTransform.Rotate(glm::vec3(0.0f, delta.x, 0));
		//mTransform.Rotate(glm::vec3(delta.y, 0, 0));
		//glm::quat rot = mTransform.GetLocalRotation();
		//rot = rot * glm::angleAxis(glm::radians(delta.x), glm::vec3(1, 0, 0));
		//rot = glm::angleAxis(glm::radians(delta.y), glm::vec3(0, 1, 0)) * rot;
		//mTransform.SetRotation(rot);
		mTransform.RotateAxis(delta);
	}
}