#pragma once

#include "Camera.h"

class FlyCamera : public Camera {
public:
	FlyCamera() : Camera() {};
	void Update() override;
};
