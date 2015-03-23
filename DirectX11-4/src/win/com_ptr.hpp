#pragma once
/*!
@file com_ptr.hpp
@brief IUnknown関連
@author takamoto
@date 14/10/15
*/

#include <memory>
#include "Unknwnbase.h"

/*!
@namespace win
@brief Windows独自実装に関するクラスや関数を含む名前空間
*/
namespace win{
	struct IUnknownRelease {
		void operator()(IUnknown* p) const {
			auto a = p->Release();
		}
	};

	template<class T>
	using com_ptr = std::unique_ptr<T, IUnknownRelease>;

	template<class T>
	std::shared_ptr<T> to_shared(com_ptr<T> && p) {
		return std::shared_ptr<T>(std::forward<com_ptr<T>>(p));
	}
}