set(ENGINE_FILES
    "IGraphicsBase.h"
    "FileIO.cpp"
    "FileIO.h"
    "AABB.h"
    "Transform.h"
    "Transform.cpp"
    "Window.h"
    "Window.cpp"
    "Engine.h"
    "Engine.cpp"
    "Job.h"
    "Job.cpp"
    "Input.h"
    "Input.cpp"
    "FlyCamera.h"
    "FlyCamera.cpp"
    )

target_sources(GraphicsPlayground PRIVATE ${ENGINE_FILES})
