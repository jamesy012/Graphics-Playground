
set("ENGINE_FILES" 
"FileIO.cpp" 
"FileIO.h"
"AABB.h"
"Transform.h"
"Transform.cpp"
"Window.h"
"Window.cpp"
"Engine.h"
"Engine.cpp"
"IGraphicsBase.h"
)

target_sources(GraphicsPlayground PRIVATE ${ENGINE_FILES})
