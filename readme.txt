cmoファイルのスキンメッシュアニメーション、及びDirect2D導入までのソース。


・cmoファイルスキンメッシュ
現状Blender出力と決め打ちしているため、それ以外のFBXを用いる場合はAxisConvert.hおよび決め打ちしているシェーダーのuvを変更のこと。
またメッシュも最初の一つしか読み込まないようになっています。

参考
https://code.msdn.microsoft.com/windowsapps/Visual-Studio-3D-Starter-455a15f1/view/Discussions

FBX->cmoへの変換参考
http://masafumi.cocolog-nifty.com/masafumis_diary/2013/02/windows-storefb.html



・Direct2D
とりあえず導入しただけといった感じ。サンプルソースをクラス化してD3Dのマルチサンプリングに対応させただけ。
。
スプライトとテキスト描画実装したが…スプライトもどきなので2DのBegin-End間で3D描画しようとするとアカンことに。何とかしなきゃ
参考＜Direct2D と Direct3D 11 の共有方法＞
http://mitsunagistudio.net/tips/d2d-d3d11-sharing/



・DirectX11ヘルパークラスのソース元
takamoto様/FBXSample
https://github.com/takamoto/FBXSample