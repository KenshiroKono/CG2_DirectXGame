#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#include <vector>
#include <string>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#define DIRECTINPUT_VERSION     0x0800   // DirectInputのバージョン指定
#include <dinput.h>
#include <DirectXTex.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")

using namespace DirectX;

//構造体宣言------------------------




//ウィンドウプロシージャ
LRESULT Windowproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	//メッセージに応じて固有の処理を行う
	switch (msg) {
	case WM_DESTROY://ウィンドウが破棄された
		//OSに対しアプリの終了を伝える
		PostQuitMessage(0);
		return 0;
	}
	//標準のメッセージ処理
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

//Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	////コンソールへの文字出力
	//OutputDebugStringA("");
	// 
	  //winAPI初期化処理ここから
#pragma region WinAPIInitialize

	//ウィンドウサイズ
	const int win_W = 1280;//横
	const int win_H = 720;//縦
	//ウィンドウクラスの設定
	WNDCLASSEX w{};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)Windowproc;
	w.lpszClassName = L"DirectXGame";
	w.hInstance = GetModuleHandle(nullptr);
	w.hCursor = LoadCursor(NULL, IDC_ARROW);

	//ウィンドウクラスをOSに登録する
	RegisterClassExW(&w);
	//ウィンドウサイズ{X座標 Y座標 横幅 縦幅 }
	RECT wrc = { 0,0,win_W,win_H };
	//自動でサイズを補正する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	//ウィンドウオブジェクトの生成
	HWND hwnd = CreateWindow(
		w.lpszClassName,		//クラス名
		L"DirectXGame",			//タイトルバーの文字
		WS_OVERLAPPEDWINDOW,	//標準的なウィンドウスタイル
		CW_USEDEFAULT,			//表示X座標(OSに任せる)
		CW_USEDEFAULT,			//表示Y座標(OSに任せる)
		wrc.right - wrc.left,	//ウィンドウ横幅
		wrc.bottom - wrc.top,	//ウィンドウ縦幅
		nullptr,				//親ウィンドウハンドル
		nullptr,				//メニューハンドル
		w.hInstance,			//呼びだしアプリケーションハンドル
		nullptr, );				//オプション

		//ウィンドウを表示状態にする
	ShowWindow(hwnd, SW_SHOW);

	MSG msg{};//メッセージ
#pragma endregion
	//winAPI初期化処理ここまで

	  //DirectX初期化処理 ここから
#pragma region DirectXInitialize

#ifdef _DEBUG
	//デバッグレイヤーをONに
	ID3D12Debug* debugcontroller;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugcontroller)))) {
		debugcontroller->EnableDebugLayer();
	}
#endif // _DEBUG
	HRESULT result;
	ID3D12Device* device = nullptr;
	IDXGIFactory7* dxgiFactory = nullptr;
	IDXGISwapChain4* swapChain = nullptr;
	ID3D12CommandAllocator* commandAllocator = nullptr;
	ID3D12GraphicsCommandList* commandList = nullptr;
	ID3D12CommandQueue* commandQueue = nullptr;
	ID3D12DescriptorHeap* rtvHeap = nullptr;
	//DXGIファクトリーの生成
	result = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
	assert(SUCCEEDED(result));
	//アダプターの列挙型
	std::vector<IDXGIAdapter4*> adapters;
	//ここに特定の名前を持つアダプターオブジェクトが入る
	IDXGIAdapter4* tmpAdapter = nullptr;
	//パフォーマンスが高いものから順に全てのアダプターを列挙する
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(
		i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&tmpAdapter)) != DXGI_ERROR_NOT_FOUND; i++) {
		//動的配列を追加する
		adapters.push_back(tmpAdapter);
	}
	//妥当なアダプタを選別する
	for (size_t i = 0; i < adapters.size(); i++) {
		DXGI_ADAPTER_DESC3 adapterDesc;
		//アダプターの情報を取得する
		adapters[i]->GetDesc3(&adapterDesc);

		//ソフトウェアデバイスを回避
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			//デバイスを使用してループを抜ける
			tmpAdapter = adapters[i];
			break;
		}
	}
	//対応レベルの配列
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	D3D_FEATURE_LEVEL featurelevel;
	for (size_t i = 0; i < _countof(levels); i++) {
		//使用したアダプターデバイスを生成
		result = D3D12CreateDevice(tmpAdapter, levels[i], IID_PPV_ARGS(&device));
		if (result == S_OK) {
			//デバイス生成できた時点でループを抜ける
			featurelevel = levels[i];
			break;
		}
	}
	//コマンドアロケータを生成
	result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	assert(SUCCEEDED(result));
	//コマンドリストを生成
	result = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
	assert(SUCCEEDED(result));
	//コマンドキューの設定
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	//コマンドキューの生成
	result = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
	assert(SUCCEEDED(result));
	//スワップチェーンの設定
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = win_W;
	swapChainDesc.Height = win_H;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//色情報の書式
	swapChainDesc.SampleDesc.Count = 1;//マルチサンプルしない
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;//バックバッファ用
	swapChainDesc.BufferCount = 2;//バッファ数を2つに設定
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;//フリップ後は破棄
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	//スワップチェーンの生成
	result = dxgiFactory->CreateSwapChainForHwnd(commandQueue, hwnd, &swapChainDesc, nullptr, nullptr, (IDXGISwapChain1**)&swapChain);
	assert(SUCCEEDED(result));
	//デスクリプタヒープの設定
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;//レンダーターゲットビュー
	rtvHeapDesc.NumDescriptors = swapChainDesc.BufferCount;//表裏の2つ
	//デスクリプタヒープの生成
	device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));

	//バックバッファ
	std::vector<ID3D12Resource*> backBuffers;
	backBuffers.resize(swapChainDesc.BufferCount);

	//スワップチェーンの全てのバッファについて処理する
	for (size_t i = 0; i < backBuffers.size(); i++) {
		//スワップチェーンからバッファを取得
		swapChain->GetBuffer((UINT)i, IID_PPV_ARGS(&backBuffers[i]));
		//デスクリプタヒープのハンドルを取得
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
		//裏か表かでアドレスがずれる
		rtvHandle.ptr += i * device->GetDescriptorHandleIncrementSize(rtvHeapDesc.Type);
		//レンダーターゲットビューの設定
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		//シェーダーの計算結果をSRGBに変換して書き込む
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		//レンダービューターゲットの生成
		device->CreateRenderTargetView(backBuffers[i], &rtvDesc, rtvHandle);
	}
	//フェンスの設定
	ID3D12Fence* fence = nullptr;
	UINT64 fenceVal = 0;
	result = device->CreateFence(fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

	// DirectInputの初期化
	IDirectInput8* directInput = nullptr;
	result = DirectInput8Create(
		w.hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput, nullptr);
	assert(SUCCEEDED(result));

	// キーボードデバイスの生成
	IDirectInputDevice8* keyboard = nullptr;
	result = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	assert(SUCCEEDED(result));

	// 入力データ形式のセット
	result = keyboard->SetDataFormat(&c_dfDIKeyboard); // 標準形式
	assert(SUCCEEDED(result));

	// 排他制御レベルのセット
	result = keyboard->SetCooperativeLevel(
		hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(result));

#pragma endregion
	//DirectX初期化処理 ここまで

	  //描画初期化処理ここから
#pragma region DrawInitialize

	// リソース設定
	D3D12_RESOURCE_DESC depthResourceDesc{};
	depthResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResourceDesc.Width = win_W; //レンダーターゲットに合わせる
	depthResourceDesc.Height = win_H; //レンダーターゲットに合わせる
	depthResourceDesc.DepthOrArraySize = 1;
	depthResourceDesc.Format = DXGI_FORMAT_D32_FLOAT;//深度値フォーット
	depthResourceDesc.SampleDesc.Count = 1;
	depthResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;//デプスステンシル

	//深度値用ヒーププロパティ
	D3D12_HEAP_PROPERTIES depthHeapProp{};
	depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	//深度値のクリア設定
	D3D12_CLEAR_VALUE   depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;

	//リソース生成
	ID3D12Resource* depthBuff = nullptr;
	result = device->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthResourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,//深度書き込みに使用
		&depthClearValue,
		IID_PPV_ARGS(&depthBuff)
	);

	//深度ビュー 一用デスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
	dsvHeapDesc.NumDescriptors = 1; // 深度ビュー ーは1つ
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;// デプスステンシルビュー
	ID3D12DescriptorHeap* dsvHeap = nullptr;
	result = device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));



	//深度ビュー作成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;//深度値フォーマット
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	device->CreateDepthStencilView(
		depthBuff,
		&dsvDesc,
		dsvHeap->GetCPUDescriptorHandleForHeapStart()
	);






	//	画像イメージデータの作成---

		/*
	// 横方向ピクセル数
		const size_t textureWidth = 256;
		// 縦方向ピクセル数
		const size_t textureHeight = 256;
		// 配列の要素数
		const size_t imageDataCount = textureWidth * textureHeight;
		// 画像イメージデータ配列
		XMFLOAT4* imageData = new XMFLOAT4[imageDataCount]; // ※必ず後で解放する

		// 全ピクセルの色を初期化
		for (size_t i = 0; i < imageDataCount; i++) {
			imageData[i].x = 1.0f;    // R
			imageData[i].y = 0.0f;    // G
			imageData[i].z = 0.0f;    // B
			imageData[i].w = 1.0f;    // A
		}
		*/

		// 頂点データ構造体
	struct Vertex {
		XMFLOAT3 pos; // xyz座標
		XMFLOAT2 uv;  // uv座標
	};
	// 頂点データ
	Vertex vertices[] = {
		// x       y       z       u    v
		//前
		{{-5.0f,-5.0f,5.0f}, {0.0f, 1.0f}}, // 左下
		{{-5.0f, 5.0f,5.0f}, {0.0f, 0.0f}}, // 左上
		{{ 5.0f,-5.0f,5.0f}, {1.0f, 1.0f}}, // 右下
		{{ 5.0f, 5.0f,5.0f}, {1.0f, 0.0f}}, // 右上
		//後
		{{-5.0f,-5.0f,-5.0f}, {0.0f, 1.0f}}, // 左下
		{{-5.0f, 5.0f,-5.0f}, {0.0f, 0.0f}}, // 左上
		{{ 5.0f,-5.0f,-5.0f}, {1.0f, 1.0f}}, // 右下
		{{ 5.0f, 5.0f,-5.0f}, {1.0f, 0.0f}}, // 右上
		//左
		{{-5.0f,-5.0f,-5.0f}, {0.0f, 1.0f}}, // 奥下
		{{-5.0f, 5.0f,-5.0f}, {0.0f, 0.0f}}, // 奥上
		{{-5.0f,-5.0f, 5.0f}, {1.0f, 1.0f}}, // 前下
		{{-5.0f, 5.0f, 5.0f}, {1.0f, 0.0f}}, // 前上
		//右
		{{5.0f,-5.0f,-5.0f}, {0.0f, 1.0f}}, // 奥下
		{{5.0f, 5.0f,-5.0f}, {0.0f, 0.0f}}, // 奥上
		{{5.0f,-5.0f, 5.0f}, {1.0f, 1.0f}}, // 前下
		{{5.0f, 5.0f, 5.0f}, {1.0f, 0.0f}}, // 前上
		//上
		{{-5.0f, 5.0f,-5.0f}, {0.0f, 1.0f}}, // 奥左
		{{ 5.0f, 5.0f,-5.0f}, {0.0f, 0.0f}}, // 奥右
		{{-5.0f, 5.0f, 5.0f}, {1.0f, 1.0f}}, // 前左
		{{ 5.0f, 5.0f, 5.0f}, {1.0f, 0.0f}}, // 前右
		//下
		{{-5.0f,-5.0f,-5.0f}, {0.0f, 1.0f}}, // 奥左
		{{ 5.0f,-5.0f,-5.0f}, {0.0f, 0.0f}}, // 奥右
		{{-5.0f,-5.0f, 5.0f}, {1.0f, 1.0f}}, // 前左
		{{ 5.0f,-5.0f, 5.0f}, {1.0f, 0.0f}}, // 前右

	};


	// インデックスデータ
	unsigned short indices[] = {
		//0, 1, 2, // 三角形1つ目
		1, 2, 3, // 三角形2つ目

		4,5,6,
		//5,6,7,

		//8,9,10,
		9,10,11,

		12,13,14,
		//13,14,15,

		//16,17,18,
		17,18,19,

		20,21,22,
		//21,22,23
		/*
				*/
	};


	// 頂点データ全体のサイズ = 頂点データ一つ分のサイズ * 頂点データの要素数
	UINT sizeVB = static_cast<UINT>(sizeof(vertices[0]) * _countof(vertices));



	// 頂点バッファの設定
	D3D12_HEAP_PROPERTIES heapProp{}; // ヒープ設定
	heapProp.Type = D3D12_HEAP_TYPE_UPLOAD; // GPUへの転送用


	// リソース設定
	D3D12_RESOURCE_DESC resDesc{};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = sizeVB; // 頂点データ全体のサイズ
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;


	// 頂点バッファの生成
	ID3D12Resource* vertBuff = nullptr;
	result = device->CreateCommittedResource(
		&heapProp, // ヒープ設定
		D3D12_HEAP_FLAG_NONE,
		&resDesc, // リソース設定
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff));
	assert(SUCCEEDED(result));


	// インデックスデータ全体のサイズ
	UINT sizeIB = static_cast<UINT>(sizeof(uint16_t) * _countof(indices));

	// リソース設定
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = sizeIB; // インデックス情報が入る分のサイズ
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// インデックスバッファの生成
	ID3D12Resource* indexBuff = nullptr;
	result = device->CreateCommittedResource(
		&heapProp, // ヒープ設定
		D3D12_HEAP_FLAG_NONE,
		&resDesc, // リソース設定
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&indexBuff));

	// インデックスバッファをマッピング
	uint16_t* indexMap = nullptr;
	result = indexBuff->Map(0, nullptr, (void**)&indexMap);
	// 全インデックスに対して
	for (int i = 0; i < _countof(indices); i++) {
		indexMap[i] = indices[i];   // インデックスをコピー
	}
	// マッピング解除
	indexBuff->Unmap(0, nullptr);

	// インデックスバッファビューの作成
	D3D12_INDEX_BUFFER_VIEW ibView{};
	ibView.BufferLocation = indexBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = sizeIB;


	// GPU上のバッファに対応した仮想メモリ(メインメモリ上)を取得
	Vertex* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	assert(SUCCEEDED(result));

	// 全頂点に対して
	for (int i = 0; i < _countof(vertices); i++) {
		vertMap[i] = vertices[i]; // 座標をコピー
	}
	// 繋がりを解除
	vertBuff->Unmap(0, nullptr);
	// 頂点バッファビューの作成
	D3D12_VERTEX_BUFFER_VIEW vbView{};
	// GPU仮想アドレス
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();
	// 頂点バッファのサイズ
	vbView.SizeInBytes = sizeVB;
	// 頂点1つ分のデータサイズ
	vbView.StrideInBytes = sizeof(vertices[0]);
	ID3DBlob* vsBlob = nullptr; // 頂点シェーダオブジェクト
	ID3DBlob* psBlob = nullptr; // ピクセルシェーダオブジェクト
	ID3DBlob* errorBlob = nullptr; // エラーオブジェクト

	// 定数バッファ用データ構造体（マテリアル）
	struct ConstBufferDataMaterial {
		XMFLOAT4 color; // 色 (RGBA)
		XMFLOAT4 graColor; // 色 (RGBA)
	};
	// 定数バッファ用データ構造体（3D変換行列）
	struct ConstBufferDataTransform {
		XMMATRIX mat; // 3D変換行列
	};



	// ヒープ設定
	D3D12_HEAP_PROPERTIES cbHeapProp{};
	cbHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;                   // GPUへの転送用
	// リソース設定
	D3D12_RESOURCE_DESC cbResourceDesc{};
	cbResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	cbResourceDesc.Width = (sizeof(ConstBufferDataMaterial) + 0xff) & ~0xff;   // 256バイトアラインメント
	cbResourceDesc.Height = 1;
	cbResourceDesc.DepthOrArraySize = 1;
	cbResourceDesc.MipLevels = 1;
	cbResourceDesc.SampleDesc.Count = 1;
	cbResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ID3D12Resource* constBuffMaterial = nullptr;

	// 定数バッファの生成
	result = device->CreateCommittedResource(
		&cbHeapProp, // ヒープ設定
		D3D12_HEAP_FLAG_NONE,
		&cbResourceDesc, // リソース設定
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuffMaterial));
	assert(SUCCEEDED(result));

	// 定数バッファのマッピング
	ConstBufferDataMaterial* constMapMaterial = nullptr;
	result = constBuffMaterial->Map(0, nullptr, (void**)&constMapMaterial); // マッピング
	assert(SUCCEEDED(result));


	ID3D12Resource* constBuffTransform = nullptr;
	ConstBufferDataTransform* constMapTransform = nullptr;






	{
		// ヒープ設定
		D3D12_HEAP_PROPERTIES cbheapProp{};
		cbHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD; // GPUへの転送用
		// リソース設定
		D3D12_RESOURCE_DESC cbResourceDesc{};
		cbResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		cbResourceDesc.Width = (sizeof(ConstBufferDataTransform) + 0xff) & ~0xff; // 256バイトアラインメント
		cbResourceDesc.Height = 1;
		cbResourceDesc.DepthOrArraySize = 1;
		cbResourceDesc.MipLevels = 1;
		cbResourceDesc.SampleDesc.Count = 1;
		cbResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		// 定数バッファ生成
		result = device->CreateCommittedResource(
			&cbHeapProp, // ヒープ設定
			D3D12_HEAP_FLAG_NONE,
			&cbResourceDesc, // リソース設定
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&constBuffTransform));
		assert(SUCCEEDED(result));
		// 定数バッファのマッピング
		result = constBuffTransform->Map(0, nullptr, (void**)&constMapTransform); // マッピング
		assert(SUCCEEDED(result));

	}

	// 単位行列を代入
	/*
	constMapTransform->mat = XMMatrixIdentity();

	constMapTransform->mat.r[0].m128_f32[0] = 2.0f / win_W;
	constMapTransform->mat.r[1].m128_f32[1] = -2.0f / win_H;
	constMapTransform->mat.r[3].m128_f32[0] = -1.0f;
	constMapTransform->mat.r[3].m128_f32[1] = 1.0f;

	// 並行投影行列の計算
	XMMATRIX matProjection= XMMatrixOrthographicOffCenterLH(
		0, win_W,
		win_H, 0,
		0.0f, 1.0f
	);

	constMapTransform->mat = matProjection;

	*/




	XMFLOAT3 scale;
	XMFLOAT3 rotation;
	XMFLOAT3 position;


	scale = { 1.0f, 1.0f, 1.0f };
	rotation = { 0.0f, 0.0f, 0.0f };
	position = { 0.0f, 0.0f, 0.0f };



	XMMATRIX matWorld;

	XMMATRIX matScaling;
	XMMATRIX matRot;
	XMMATRIX matTrans;

	//= XMMatrixTranslation(50.0f, 0, 0)


	//matWorld *= matScaling * matRot * matTrans;



	//ビュー変換行列
	XMMATRIX matView;
	XMFLOAT3 eye(0, 0, -100);  //視点
	XMFLOAT3 target(0, 0, 0);  //注視点
	XMFLOAT3 up(0, 1, 0);		//上方向ベクトル
	float cameraRange = -100;	//カメラの距離
	float angle = 0;			//カメラの角度

	matView = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));

	// 透視投影行列(射影変換)の変換

	XMMATRIX matProject = XMMatrixPerspectiveFovLH(
		XMConvertToRadians(45.0f), // 上下画角45度
		(float)win_W / win_H,         // アスペクト比（画面横幅 / 画面縦幅）
		0.1f, 1000.0f              //  前端、奥端
	);

	//constMapTransform->mat = matWorld * matView * matProject;




	// 値を書き込むと自動的に転送される
	constMapMaterial->color = XMFLOAT4(1, 0, 0, 0.5f);              // RGBAで半透明の赤
	constMapMaterial->graColor = XMFLOAT4(1, 1, 1, 1.0f);           //

	TexMetadata metadata{};
	ScratchImage scratchImg{};
	// WICテクスチャのロード
	result = LoadFromWICFile(
		L"GraResource/uvChecker.png",
		WIC_FLAGS_NONE,
		&metadata, scratchImg);

	assert(result);

	ScratchImage mipChain{};
	// ミップマップ生成
	result = GenerateMipMaps(
		scratchImg.GetImages(), scratchImg.GetImageCount(), scratchImg.GetMetadata(),
		TEX_FILTER_DEFAULT, 0, mipChain);
	if (SUCCEEDED(result)) {
		scratchImg = std::move(mipChain);
		metadata = scratchImg.GetMetadata();
	}

	// 読み込んだディフューズテクスチャをSRGBとして扱う
	metadata.format = MakeSRGB(metadata.format);


	//テクスチャバッファ

	// ヒープ設定
	D3D12_HEAP_PROPERTIES textureHeapProp{};
	textureHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	textureHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	textureHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	// リソース設定
	D3D12_RESOURCE_DESC textureResourceDesc{};
	textureResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureResourceDesc.Format = metadata.format;
	textureResourceDesc.Width = metadata.width;
	textureResourceDesc.Height = (UINT)metadata.height;
	textureResourceDesc.DepthOrArraySize = (UINT16)metadata.arraySize;
	textureResourceDesc.MipLevels = (UINT16)metadata.mipLevels;
	textureResourceDesc.SampleDesc.Count = 1;

	// テクスチャバッファの生成
	ID3D12Resource* texBuff = nullptr;
	result = device->CreateCommittedResource(
		&textureHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&textureResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&texBuff));

	// テクスチャバッファにデータ転送
	// 全ミップマップについて
	for (size_t i = 0; i < metadata.mipLevels; i++) {

		// ミップマップレベルを指定してイメージを取得
		const Image* img = scratchImg.GetImage(i, 0, 0);
		// テクスチャバッファにデータ転送
		result = texBuff->WriteToSubresource(
			(UINT)i,
			nullptr,              // 全領域へコピー
			img->pixels,          // 元データアドレス
			(UINT)img->rowPitch,  // 1ラインサイズ
			(UINT)img->slicePitch // 1枚サイズ
		);
		assert(SUCCEEDED(result));
	}

	// 元データ開放
	//delete[] imageData;
	// SRVの最大個数
	const size_t kMaxSRVCount = 2056;

	// デスクリプタヒープの設定
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;//シェーダから見えるように
	srvHeapDesc.NumDescriptors = kMaxSRVCount;

	// 設定を元にSRV用デスクリプタヒープを生成
	ID3D12DescriptorHeap* srvHeap = nullptr;
	result = device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap));
	assert(SUCCEEDED(result));

	//SRVヒープの先頭ハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();


	// シェーダリソースビュー設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = resDesc.Format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = resDesc.MipLevels;



	// ハンドルの指す位置にシェーダーリソースビュー作成
	device->CreateShaderResourceView(texBuff, &srvDesc, srvHandle);



	// 頂点シェーダの読み込みとコンパイル
	result = D3DCompileFromFile(
		L"BasicVS.hlsl", // シェーダファイル名
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルード可能にする
		"main", "vs_5_0", // エントリーポイント名、シェーダーモデル指定
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用設定
		0,
		&vsBlob, &errorBlob);
	// エラーなら
	if (FAILED(result)) {
		// errorBlobからエラー内容をstring型にコピー
		std::string error;
		error.resize(errorBlob->GetBufferSize());
		std::copy_n((char*)errorBlob->GetBufferPointer(),
					errorBlob->GetBufferSize(),
					error.begin());
		error += "\n";
		// エラー内容を出力ウィンドウに表示
		OutputDebugStringA(error.c_str());
		assert(0);
	}

	// ピクセルシェーダの読み込みとコンパイル
	result = D3DCompileFromFile(
		L"BasicPS.hlsl", // シェーダファイル名
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルード可能にする
		"main", "ps_5_0", // エントリーポイント名、シェーダーモデル指定
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用設定
		0,
		&psBlob, &errorBlob);
	// エラーなら
	if (FAILED(result)) {
		// errorBlobからエラー内容をstring型にコピー
		std::string error;
		error.resize(errorBlob->GetBufferSize());
		std::copy_n((char*)errorBlob->GetBufferPointer(),
					errorBlob->GetBufferSize(),
					error.begin());
		error += "\n";
		// エラー内容を出力ウィンドウに表示
		OutputDebugStringA(error.c_str());
		assert(0);
	}
	// 頂点レイアウト
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
	},

	};

	// グラフィックスパイプライン設定
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc{};

	// シェーダーの設定
	pipelineDesc.VS.pShaderBytecode = vsBlob->GetBufferPointer();
	pipelineDesc.VS.BytecodeLength = vsBlob->GetBufferSize();
	pipelineDesc.PS.pShaderBytecode = psBlob->GetBufferPointer();
	pipelineDesc.PS.BytecodeLength = psBlob->GetBufferSize();
	// サンプルマスクの設定
	pipelineDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; // 標準設定
	// ラスタライザの設定
	pipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // カリングしない
	pipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; // ポリゴン内塗りつぶし
	pipelineDesc.RasterizerState.DepthClipEnable = true; // 深度クリッピングを有効に
	//デプスステンシルアートの設定
	pipelineDesc.DepthStencilState.DepthEnable = true;
	pipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	pipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	pipelineDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;



	// ブレンドステート

	/*
	pipelineDesc.BlendState.RenderTarget[0].RenderTargetWriteMask// RBGA全てのチャンネルを描画
		= D3D12_COLOR_WRITE_ENABLE_ALL;
	*/

	// レンダーターゲットのブレンド設定
	D3D12_RENDER_TARGET_BLEND_DESC& blenddesc = pipelineDesc.BlendState.RenderTarget[0];
	blenddesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL; // RBGA全てのチャンネルを描画

	blenddesc.BlendEnable = true;                   // ブレンドを有効にする
	//加算合成
	blenddesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;    // 加算
	blenddesc.SrcBlendAlpha = D3D12_BLEND_ONE;      // ソースの値を100% 使う
	blenddesc.DestBlendAlpha = D3D12_BLEND_ZERO;    // デストの値を  0% 使う

	//// 減算合成
	//blenddesc.BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;    // デストからソースを減算
	//blenddesc.SrcBlend = D3D12_BLEND_ONE;               // ソースの値を100% 使う
	//blenddesc.DestBlend = D3D12_BLEND_ONE;              // デストの値を100% 使う
	//// 色反転
	//blenddesc.BlendOp = D3D12_BLEND_OP_ADD;             // 加算
	//blenddesc.SrcBlend = D3D12_BLEND_INV_DEST_COLOR;    // 1.0f-デストカラーの値
	//blenddesc.DestBlend = D3D12_BLEND_ZERO;             // 使わない

	// 半透明合成
	blenddesc.BlendOp = D3D12_BLEND_OP_ADD;             // 加算
	blenddesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;         // ソースのアルファ値
	blenddesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;    // 1.0f-ソースのアルファ値

	// 頂点レイアウトの設定
	pipelineDesc.InputLayout.pInputElementDescs = inputLayout;
	pipelineDesc.InputLayout.NumElements = _countof(inputLayout);
	// 図形の形状設定
	pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// その他の設定
	pipelineDesc.NumRenderTargets = 1; // 描画対象は1つ
	pipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 0~255指定のRGBA
	pipelineDesc.SampleDesc.Count = 1; // 1ピクセルにつき1回サンプリング


	// デスクリプタレンジの設定
	D3D12_DESCRIPTOR_RANGE descriptorRange{};
	descriptorRange.NumDescriptors = 1;         //一度の描画に使うテクスチャが1枚なので1
	descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange.BaseShaderRegister = 0;     //テクスチャレジスタ0番
	descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;


	// ルートパラメータの設定
	D3D12_ROOT_PARAMETER rootParams[3] = {};
	// 定数バッファ0番
	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;   // 種類
	rootParams[0].Descriptor.ShaderRegister = 0;                   // 定数バッファ番号
	rootParams[0].Descriptor.RegisterSpace = 0;                    // デフォルト値
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;  // 全てのシェーダから見える
	// テクスチャレジスタ0番
	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;   //種類
	rootParams[1].DescriptorTable.pDescriptorRanges = &descriptorRange;		  //デスクリプタレンジ
	rootParams[1].DescriptorTable.NumDescriptorRanges = 1;              		  //デスクリプタレンジ数
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;               //全てのシェーダから見える
	// 定数バッファ1番
	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // 種類
	rootParams[2].Descriptor.ShaderRegister = 1; // 定数バッファ番号
	rootParams[2].Descriptor.RegisterSpace = 0; // デフォルト値
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // すべてのシェーダから見える



	// ルートシグネチャ
	ID3D12RootSignature* rootSignature;

	// テクスチャサンプラーの設定
	D3D12_STATIC_SAMPLER_DESC samplerDesc{};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;                 //横繰り返し（タイリング）
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;                 //縦繰り返し（タイリング）
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;                 //奥行繰り返し（タイリング）
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;  //ボーダーの時は黒
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;                   //全てリニア補間
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;                                 //ミップマップ最大値
	samplerDesc.MinLOD = 0.0f;                                              //ミップマップ最小値
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;           //ピクセルシェーダからのみ使用可能
	// ルートシグネチャの設定
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.pParameters = rootParams; //ルートパラメータの先頭アドレス
	rootSignatureDesc.NumParameters = _countof(rootParams);        //ルートパラメータ数
	rootSignatureDesc.pStaticSamplers = &samplerDesc;
	rootSignatureDesc.NumStaticSamplers = 1;



	// ルートシグネチャのシリアライズ
	ID3DBlob* rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0,
										 &rootSigBlob, &errorBlob);
	assert(SUCCEEDED(result));
	result = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(),
										 IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(result));
	rootSigBlob->Release();
	// パイプラインにルートシグネチャをセット
	pipelineDesc.pRootSignature = rootSignature;
	// パイプラインステートの生成
	ID3D12PipelineState* pipelineState = nullptr;
	result = device->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&pipelineState));
	assert(SUCCEEDED(result));

#pragma endregion
	//描画初期化処理ここまで


	//ゲームループ---------------
	while (1) {

#pragma region Messege

		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {//メッセージはあるか？
			TranslateMessage(&msg); //キー入力メッセージの処理
			DispatchMessage(&msg);  //プロシージャにメッセージを送る
		}

		if (msg.message == WM_QUIT) {
			break;
		}
#pragma endregion//メッセージ処理

		//DirectX毎フレーム処理 ここから
#pragma region UpDate

		// キーボード情報の取得開始
		keyboard->Acquire();

		// 全キーの入力状態を取得する
		BYTE key[256] = {};
		keyboard->GetDeviceState(sizeof(key), key);

		// 数字の0キーが押されていたら
		if (key[DIK_0]) {
			OutputDebugStringA("Hit 0\n");  // 出力ウィンドウに「Hit 0」と表示
		}


		//バックバッファの番号を取得
		UINT bbIndex = swapChain->GetCurrentBackBufferIndex();

		//1. リソースバリアで書き込み可能に変更
		D3D12_RESOURCE_BARRIER barrierDesc{};
		barrierDesc.Transition.pResource = backBuffers[bbIndex];//バックバッファを指定
		barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;		 //表示状態から↓
		barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET; //描画状態へ
		commandList->ResourceBarrier(1, &barrierDesc);


		//2. 描画先の変更
		//レンダーターゲットビューのハンドルを取得
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += bbIndex * device->GetDescriptorHandleIncrementSize(rtvHeapDesc.Type);
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
		commandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);



		//3. 画面クリア(背景)
		FLOAT clearColo[] = { 0.1f, 0.25f, 0.5f, 0.0f };//青っぽい色

		if (key[DIK_SPACE]) {     // スペースキーが押されていたら

			clearColo[1] = 1.0f;
		}
		commandList->ClearRenderTargetView(rtvHandle, clearColo, 0, nullptr);
		commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);



		//4.グラフィックスコマンドここから------------

		// ビューポート設定コマンド
		D3D12_VIEWPORT viewport{};
		viewport.Width = swapChainDesc.Width;
		viewport.Height = swapChainDesc.Height;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		// ビューポート設定コマンドを、コマンドリストに積む
		commandList->RSSetViewports(1, &viewport);

		// シザー矩形
		D3D12_RECT scissorRect{};
		scissorRect.left = 0; // 切り抜き座標左
		scissorRect.right = scissorRect.left + swapChainDesc.Width; // 切り抜き座標右
		scissorRect.top = 0; // 切り抜き座標上
		scissorRect.bottom = scissorRect.top + swapChainDesc.Height; // 切り抜き座標下
		// シザー矩形設定コマンドを、コマンドリストに積む
		commandList->RSSetScissorRects(1, &scissorRect);

		// パイプラインステートとルートシグネチャの設定コマンド
		commandList->SetPipelineState(pipelineState);
		commandList->SetGraphicsRootSignature(rootSignature);



		// SRVヒープの設定コマンド
		commandList->SetDescriptorHeaps(1, &srvHeap);
		// SRVヒープの先頭ハンドルを取得（SRVを指しているはず）
		D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();

		// SRVヒープの先頭にあるSRVをルートパラメータ1番に設定
		commandList->SetGraphicsRootDescriptorTable(1, srvGpuHandle);
		// 定数バッファビュー(CBV)の設定コマンド
		commandList->SetGraphicsRootConstantBufferView(0, constBuffMaterial->GetGPUVirtualAddress());
		// インデックスバッファビューの設定コマンド
		commandList->IASetIndexBuffer(&ibView);
		// プリミティブ形状の設定コマンド
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // 三角形リスト
		// 頂点バッファビューの設定コマンド
		commandList->IASetVertexBuffers(0, 1, &vbView);
		// 定数バッファビュー(CBV)の設定コマンド
		commandList->SetGraphicsRootConstantBufferView(2, constBuffTransform->GetGPUVirtualAddress());

		// 描画コマンド

		commandList->DrawIndexedInstanced(_countof(indices), 1, 0, 0, 0); // 全ての頂点を使って描画


		if (constMapMaterial->color.z<=0 ) {
			constMapMaterial->color.z = 1;
		}
		else {
			constMapMaterial->color.z -= 0.001;
		}
		


		if (key[DIK_UP] || key[DIK_DOWN] || key[DIK_RIGHT] || key[DIK_LEFT]) {
			// 座標を移動する処理（Z座標）
			if (key[DIK_UP]) { position.z += 1.0f; }
			else if (key[DIK_DOWN]) { position.z -= 1.0f; }
			if (key[DIK_RIGHT]) { position.x += 1.0f; }
			else if (key[DIK_LEFT]) { position.x -= 1.0f; }
		}



		//		XMMATRIX matTrans; // 平行移動行列

		matWorld = XMMatrixIdentity();
		matScaling = XMMatrixScaling(scale.x, scale.y, scale.z);
		matRot = XMMatrixIdentity();
		matRot *= XMMatrixRotationZ(rotation.z);
		matRot *= XMMatrixRotationX(rotation.x);
		matRot *= XMMatrixRotationY(rotation.y);
		matTrans = XMMatrixTranslation(position.x, position.y, position.z);

		matWorld *= matScaling * matRot * matTrans;// ワールド行列に平行移動を反映




		if (key[DIK_D] || key[DIK_A]) {
			if (key[DIK_D])angle += XMConvertToRadians(1.0f);
			else if (key[DIK_A])angle -= XMConvertToRadians(1.0f);

			eye.x = cameraRange * sinf(angle);
			eye.z = cameraRange * cosf(angle);

			matView = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
		}


		// 定数バッファに転送
		constMapTransform->mat = matWorld * matView * matProject;

		//4.グラフィックスコマンドここまで

		//5. リソースバリアを戻す
		barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET; //描画状態から
		barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;		//表示状態へ
		commandList->ResourceBarrier(1, &barrierDesc);

		//命令のクローズ
		result = commandList->Close();
		assert(SUCCEEDED(result));
		//コマンドリストの実行
		ID3D12CommandList* commandLists[] = { commandList };
		commandQueue->ExecuteCommandLists(1, commandLists);
		//画面に表示するバッファをフリップ(表裏の入れ替え)
		result = swapChain->Present(1, 0);
		assert(SUCCEEDED(result));

		//コマンドの実行完了を待つ
		commandQueue->Signal(fence, ++fenceVal);
		if (fence->GetCompletedValue() != fenceVal) {
			HANDLE event = CreateEvent(nullptr, false, false, nullptr);
			fence->SetEventOnCompletion(fenceVal, event);
			WaitForSingleObject(event, INFINITE);
			CloseHandle(event);
		}

		//キューをクリア
		result = commandAllocator->Reset();
		assert(SUCCEEDED(result));
		//再びコマンドリストを溜める準備
		result = commandList->Reset(commandAllocator, nullptr);
		assert(SUCCEEDED(result));

#pragma endregion
		//DirectX毎フレーム処理 ここまで


	}


	//ウィンドウクラスを登録解除
	UnregisterClass(w.lpszClassName, w.hInstance);

	return 0;
}