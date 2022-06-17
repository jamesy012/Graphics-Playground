//this should be in graphics section?
//needed here for Window.cpp

#include <vulkan/vulkan.h>

static VkAllocationCallbacks* CreateAllocationCallbacks() {
	VkAllocationCallbacks callback;
	callback.pUserData			   = nullptr;
	callback.pfnAllocation		   = nullptr;
	callback.pfnReallocation	   = nullptr;
	callback.pfnFree			   = nullptr;
	callback.pfnInternalAllocation = nullptr;
	callback.pfnInternalFree	   = nullptr;
	return nullptr;
}

static VkAllocationCallbacks* GetAllocationCallback() {
	static VkAllocationCallbacks* allocationCallback = CreateAllocationCallbacks();
	return allocationCallback;
}