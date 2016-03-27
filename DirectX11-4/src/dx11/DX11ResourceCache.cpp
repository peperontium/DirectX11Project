#include "DX11ResourceCache.h"

#include "DX11GlobalDevice.h"
#include "DX11ThinWrapper.h"

#include <fstream>
#include <thread>


namespace {
	inline bool isFileExist(const std::wstring& path) {

		std::fstream fs(path.c_str());
		if (fs) {
			return true;
		}
		return false;
	}
	

	class ThreadGuard {
	private:
		std::thread& thread;

	public:
		ThreadGuard(std::thread& t): thread(t){}
		~ThreadGuard() {
			if (thread.joinable()) {
				thread.join();
			}
		}

	};
}

namespace dx11 {

	namespace ResourceCache {

		namespace Texture {

			static std::unordered_map<std::wstring, std::shared_ptr<ID3D11ShaderResourceView>>	_texContainer;
			static std::mutex	_containerMutex;


			void PreLoadAsync(std::vector<std::wstring> texPathList) {
				
				std::thread t([&](std::vector<std::wstring> texPathList){
					std::lock_guard<std::mutex> lock(_containerMutex);

					auto device = dx11::AccessDX11Device();
					for (const std::wstring& path : texPathList) {
						if ( (_texContainer.count(path) != 0) || !isFileExist(path))
							continue;

						_texContainer.emplace(
							path, DX11ThinWrapper::d3::CreateWICTextureFromFile(device, path.c_str())
							);
					}
				}
				,std::move(texPathList));
				ThreadGuard guard(t);
				t.detach();
			}
			std::shared_ptr<ID3D11ShaderResourceView> Get(const std::wstring& texturePath, bool useCache) {
				
				std::lock_guard<std::mutex> lock(_containerMutex);

				//	キャッシュに存在するならそれを返す
				auto it = _texContainer.find(texturePath);
				if (it != _texContainer.end()) {
					return it->second;
				} else {
				//	無い場合は作成、キャッシュする場合のみ保存
					auto tex = DX11ThinWrapper::d3::CreateWICTextureFromFile(
							dx11::AccessDX11Device(), texturePath.c_str()
						);

					if (useCache) {
						_texContainer.emplace(texturePath,tex);
					}
					return tex;
				}
			}
			void Release(const std::wstring& textureName) {
				
				std::lock_guard<std::mutex> lock(_containerMutex);

				auto it = _texContainer.find(textureName);
				if (it != _texContainer.end()) {
					_texContainer.erase(it);
				}

			}
			void ReleaseAll() {

				std::lock_guard<std::mutex> lock(_containerMutex);
				_texContainer.clear();
			}
		};

		namespace Shader {
		}

	}
}

