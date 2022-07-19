#include "PlatformDebug.h"
#include "Graphics/Graphics.h"
#include "Engine/Engine.h"

#include "Game/StateTest.h"

int main() {
	//vs code is annoying, doesnt clear the last output
	LOGGER::Log("--------------------------------\n");

	Graphics vulkanGraphics;

	Engine gameEngine;
	gameEngine.Startup(&vulkanGraphics);

	StateTest stateTest;
	gEngine->SetDesiredState(&stateTest);

	//while(!gEngine->GetWindow()->ShouldClose())
	gEngine->GameLoop();
	
	gEngine->Shutdown();

	return 0;
}