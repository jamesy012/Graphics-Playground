#pragma once

#include <vector>
#include <functional>

template<typename T>
class Callback {
public:
	void AddCallback(std::function<T> aCallback) {
		mCallbacks.push_back(aCallback);
	}

	//template<typename Func>
	//void AddCallback(const Func& aCallback, const void* aClass) {
	//	mCallbacks.push_back(std::bind(aCallback, aClass, std::placeholders::_1));
	//}

	//void RemoveCallback(std::function<T> aCallback) {
	//		auto index = std::find(mCallbacks.begin(), mCallbacks.end(), aCallback);
	//		if(index != mCallbacks.end()) {
	//			mCallbacks.erase(index);
	//		}
	//}

	//void RemoveCallback(T& aCallback) {
	//	auto pred = [&aCallback](const std::function<T>& func){return aCallback == *func.target<decltype(aCallback)>(); };
    //    mCallbacks.erase(std::remove_if(mCallbacks.begin(), mCallbacks.end(), pred), mCallbacks.end());
	//}

	template<typename... Args>
	void Call(Args&&... args) {
		for(auto callback: mCallbacks) {
			callback(args...);
		}
	};

private:
	std::vector<std::function<T>> mCallbacks;
};
