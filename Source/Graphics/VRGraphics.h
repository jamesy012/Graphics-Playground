#pragma once

#include <vector>
#include <string>

class VRGraphics {
public:

	void Create();

	std::vector<std::string> GetVulkanInstanceExtensions();

	void Destroy();

};