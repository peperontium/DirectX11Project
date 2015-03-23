#pragma once
#include <memory>

#include <d2d1.h>
#include <d3d10_1.h>

#include "../dx11/DX11ThinWrapper.h"

//!	Direct3DレイヤーとDirect2Dレイヤーを共存させるためにDirectX10.1を用います
//!	参考
//!	http://mitsunagistudio.net/tips/d2d-d3d11-sharing/

//	DX11のデバイスに初期設定だけを依存させたい…どうやって？
namespace dx10 {
	class DX10DeviceSharedGuard {
	private:
		std::shared_ptr<ID3D10Device1> _device;
	public:
		DX10DeviceSharedGuard(ID3D11Device *pDX11Device);
	};

	::ID3D10Device1 * AccessDX10Device();



	namespace thin_template{
		LPVOID OpenSharedResource_base(ID3D11Texture2D* texture, const IID& uuid);
	} //	end of namespace thin_template 

	template <typename T>
	//!	DirectX11で作成したリソースをDX10で共有
	//!	渡すリソースは作成時にD3D11_RESOURCE_MISC_SHAREDの共有フラグを設定した2Dテクスチャ。
	/**
	* @param  texture11	共有したい2Dテクスチャリソース
	* @return DX10.1の共有オブジェクト
	*
	* @see https://msdn.microsoft.com/ja-jp/library/ee419814(v=vs.85).aspx
	*/
	std::shared_ptr<T> OpenSharedResource(ID3D11Texture2D* texture11) {
		T* sharedResource = reinterpret_cast<T*>(thin_template::OpenSharedResource_base(texture11, __uuidof(T)));
		
		return(std::shared_ptr<T>(sharedResource, DX11ThinWrapper::ReleaseIUnknown));
	}

} //	end of namespace dx10