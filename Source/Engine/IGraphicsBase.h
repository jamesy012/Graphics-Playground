#pragma once

class Window;

//common functions for Engine layer to talk to Graphics layer
class IGraphicsBase {
public:
	virtual bool Startup()	 = 0;
	virtual bool Initalize() = 0;
	virtual bool Destroy()	 = 0;

	virtual void AddWindow(Window* aWindow) = 0;

	virtual void StartNewFrame() = 0;
	virtual void EndFrame()		 = 0;
};