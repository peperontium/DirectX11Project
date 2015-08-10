#pragma once

#include <memory>
#include <Unknwn.h>

namespace comUtil {
	inline void ReleaseIUnknown(IUnknown * p) { p->Release(); }


	template <typename T>
	inline std::shared_ptr<T> QueryInterface(IUnknown* pUnknown) {
		T* pResource;
		HRESULT hr = pUnknown->QueryInterface(__uuidof(T), (LPVOID*)&pResource);
		if (FAILED(hr))
			throw(std::runtime_error("Query Failed"));

		return std::shared_ptr<T>(pResource, comUtil::ReleaseIUnknown);
	}
}
