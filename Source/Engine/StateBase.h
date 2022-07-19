#pragma once

#include "imgui.h"

class StateBase {
public:
	//ASYNC - when the state is queued up to begin
	virtual void Initalize(){};
	//when this state becomes the active state
	virtual void StartUp(){};

	virtual void ImGuiRender(){};
	virtual void Update(){};
	virtual void Render(){};

	//state is being replaced as the active state
	virtual void Finish(){};

	//
	virtual void Destroy(){};
};
