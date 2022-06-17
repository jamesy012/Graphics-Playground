#pragma once

class IGraphicsBase;

class Window;

//controls the opening of the game
//game loop
//which game states/levels are ticking
//threading?
//shutdown
class Engine {
public:
	void Startup(IGraphicsBase* aGraphics);
    bool GameLoop();
	void Shutdown();

    float GetTimeSinceStart() const;
    float GetDeltaTime() const;

    Window* GetWindow() const;
private:
    IGraphicsBase* mGraphics;
};

extern Engine* gEngine;