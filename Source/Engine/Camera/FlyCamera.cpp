#include "FlyCamera.h"

#include "Engine/Input.h"
#include "Engine/Engine.h" //dt
#include "Engine/Window.h" //focus

void FlyCamera::Update() {

	glm::vec3 movement = glm::vec3(0);
	glm::vec3 rotation = glm::vec3(0);
	const float dt = gEngine->GetDeltaTimeUnScaled();
	const float speed = 5 * dt;
	const float rotationSpeed = 100 * dt;
	const float mouseRotationSpeed = 50 * dt;
	const float mouseMovementSpeed = 5 * dt;
	//reverses movement of
	const int yawMovementMulti = mTransform.IsUp() ? 1 : -1;

	movement.x -= gInput->IsKeyDown(GLFW_KEY_A);
	movement.x += gInput->IsKeyDown(GLFW_KEY_D);
	movement.z -= gInput->IsKeyDown(GLFW_KEY_W);
	movement.z += gInput->IsKeyDown(GLFW_KEY_S);
	movement.y -= gInput->IsKeyDown(GLFW_KEY_Q);
	movement.y += gInput->IsKeyDown(GLFW_KEY_E);

	mTransform.TranslateLocal(movement * speed);

	rotation.y += gInput->IsKeyDown(GLFW_KEY_LEFT) * yawMovementMulti;
	rotation.y -= gInput->IsKeyDown(GLFW_KEY_RIGHT) * yawMovementMulti;
	rotation.x += gInput->IsKeyDown(GLFW_KEY_UP);
	rotation.x -= gInput->IsKeyDown(GLFW_KEY_DOWN);
	//rotation.z += gInput->IsKeyDown(GLFW_KEY_LEFT_BRACKET);
	//rotation.z -= gInput->IsKeyDown(GLFW_KEY_RIGHT_BRACKET);

	mTransform.RotateAxis(rotation * rotationSpeed);

	gEngine->GetWindow()->SetLock(mStartedDoubleClickFly || mStartedLeftClickFly || mStartedRightClickFly);

#if PLATFORM_APPLE
	//my mac has a trackpad to so it's hard to use the mouse controls
	//this should be a device check or an option instead of a platform define
	if(mStartedRightClickFly) {
		glm::vec2 delta = -gInput->GetMouseDelta() * mouseRotationSpeed;
		delta = glm::vec2(delta.y, delta.x * yawMovementMulti); //mouse is swapped
		mTransform.RotateAxis(delta);

		if(gInput->IsMouseButtonUp(GLFW_MOUSE_BUTTON_1) && gInput->IsMouseButtonUp(GLFW_MOUSE_BUTTON_2)) {
				mStartedRightClickFly = false;
			}
		return;
	} else {
		if(gInput->IsMouseButtonDown(GLFW_MOUSE_BUTTON_1) || gInput->IsMouseButtonDown(GLFW_MOUSE_BUTTON_2)) {
			mStartedRightClickFly = true;
		}
	}
#else

	if(mStartedDoubleClickFly) {
		glm::vec2 delta = gInput->GetMouseDelta() * mouseMovementSpeed;
		mTransform.TranslateLocal(glm::vec3(delta.x, -delta.y, 0));

		if(gInput->IsMouseButtonUp(GLFW_MOUSE_BUTTON_1) || gInput->IsMouseButtonUp(GLFW_MOUSE_BUTTON_2)) {
			mStartedDoubleClickFly = false;
		}
		return;
	} else {
		if(gInput->IsMouseButtonDown(GLFW_MOUSE_BUTTON_1) && gInput->IsMouseButtonDown(GLFW_MOUSE_BUTTON_2)) {
			mStartedDoubleClickFly = true;
		}
	}

	if(mStartedRightClickFly) {
		glm::vec2 delta = -gInput->GetMouseDelta() * mouseRotationSpeed;
		delta = glm::vec2(delta.y, delta.x * yawMovementMulti); //mouse is swapped
		mTransform.RotateAxis(delta);

		if(gInput->IsMouseButtonUp(GLFW_MOUSE_BUTTON_2)) {
			mStartedRightClickFly = false;
		}
		return;
	} else {
		if(gInput->IsMouseButtonDown(GLFW_MOUSE_BUTTON_2)) {
			mStartedRightClickFly = true;
		}
	}

	if(mStartedLeftClickFly) {
		glm::vec2 delta = gInput->GetMouseDelta();
		mTransform.TranslateLocal(glm::vec3(0, 0, delta.y) * mouseMovementSpeed);
		mTransform.RotateAxis(glm::vec3(0, -delta.x * yawMovementMulti, 0) * mouseRotationSpeed);

		if(gInput->IsMouseButtonUp(GLFW_MOUSE_BUTTON_1)) {
			mStartedLeftClickFly = false;
		}
		return;
	} else {
		if(gInput->IsMouseButtonDown(GLFW_MOUSE_BUTTON_1)) {
			mStartedLeftClickFly = true;
		}
	}
#endif
}