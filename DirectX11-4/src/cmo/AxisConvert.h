#pragma once

#include <DirectXMath.h>

namespace axis{

	//!	使う座標系のリストアップ
	//!	Blender以外にも使う場合は適宜追加してください
	enum ModelCoordinate {
		Blender,
	};



	//!	DirectXの左手系座標系へ合わせてモデルを回転させる行列を取得
	/**
	* @param  modelCoordinate	モデル側の座標系
	*
	* @return 変換行列、ローカル変換×得られた変換×ワールド変換の形で使う
	 */
	inline DirectX::XMMATRIX getAxisConvertMatrix(ModelCoordinate modelCoordinate){
		
		using namespace DirectX;

		switch (modelCoordinate) {

		case ModelCoordinate::Blender:
			return XMMatrixRotationX(-XM_PI / 2)*XMMatrixRotationY(XM_PI) * XMMatrixScaling(-1, 1, 1);

		default:
			assert(false);
			return XMMatrixIdentity();
		}

	}

}	//	end of namespace axis
