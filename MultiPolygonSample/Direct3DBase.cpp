#include "pch.h"
#include "Direct3DBase.h"

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::UI::Core;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;

// コンストラクター。
Direct3DBase::Direct3DBase()
{
}

// 実行するのに必要な Direct3D リソースを初期化します。
void Direct3DBase::Initialize(CoreWindow^ window)
{
	m_window = window;
	
	CreateDeviceResources();
	CreateWindowSizeDependentResources();
}

// すべてのデバイス リソースを再作成し、現在の状態を設定します。
void Direct3DBase::HandleDeviceLost()
{
	// UpdateForWindowSizeChange がすべてのリソースを確実に再作成するように、これらのメンバー変数をリセットします。
	m_windowBounds.Width = 0;
	m_windowBounds.Height = 0;
	m_swapChain = nullptr;

	CreateDeviceResources();
	UpdateForWindowSizeChange();
}

// これらは、デバイスに依存するリソースです。
void Direct3DBase::CreateDeviceResources()
{
	// このフラグは、カラー チャネルの順序が API の既定値とは異なるサーフェスのサポートを追加します。 
	// これは、Direct2D との互換性を保持するために必要です。
	UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
	// プロジェクトがデバッグ ビルドに含まれる場合、このフラグを使用して SDK レイヤーによるデバッグを有効にします。
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	// この配列では、このアプリケーションでサポートされる DirectX ハードウェア機能レベルのセットを定義します。
	// 順序が保存されることに注意してください。
	// アプリケーションの最低限必要な機能レベルをその説明で宣言することを忘れないでください。
	// 特に記載がない限り、すべてのアプリケーションは 9.1 をサポートすることが想定されます。
	D3D_FEATURE_LEVEL featureLevels[] = 
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	// Direct3D 11 API デバイス オブジェクトと、対応するコンテキストを作成します。
	ComPtr<ID3D11Device> device;
	ComPtr<ID3D11DeviceContext> context;
	DX::ThrowIfFailed(
		D3D11CreateDevice(
			nullptr, // 既定のアダプターを使用する nullptr を指定します。
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			creationFlags, // デバッグ フラグと Direct2D 互換性フラグを設定します。
			featureLevels, // このアプリがサポートできる機能レベルの一覧を表示します。
			ARRAYSIZE(featureLevels),
			D3D11_SDK_VERSION, // Windows ストア アプリでは、これには常に D3D11_SDK_VERSION を設定します。
			&device, // 作成された Direct3D デバイスを返します。
			&m_featureLevel, // 作成されたデバイスの機能レベルを返します。
			&context // デバイスのイミディエイト コンテキストを返します。
			)
		);

	// Direct3D 11.1 API のデバイス インターフェイスとコンテキスト インターフェイスを取得します。
	DX::ThrowIfFailed(
		device.As(&m_d3dDevice)
		);

	DX::ThrowIfFailed(
		context.As(&m_d3dContext)
		);
}

// ウィンドウの SizeChanged イベントで変更されるすべてのメモリ リソースを割り当てます。
void Direct3DBase::CreateWindowSizeDependentResources()
{ 
	// ウィンドウの境界を保存すると、次回 SizeChanged イベントを取得するときに、サイズが同じであれば、
	// 再ビルドを回避できるようになります。
	m_windowBounds = m_window->Bounds;

	// 必要なスワップ チェーンとレンダー ターゲットのサイズをピクセル単位で計算します。
	float windowWidth = ConvertDipsToPixels(m_windowBounds.Width);
	float windowHeight = ConvertDipsToPixels(m_windowBounds.Height);

	// スワップ チェーンの幅と高さは、ウィンドウの横長方向の幅と高さを
	// ベースにする必要があります。ウィンドウが縦長方向の場合は、
	// サイズを反転させる必要があります
	m_orientation = DisplayProperties::CurrentOrientation;
	bool swapDimensions =
		m_orientation == DisplayOrientations::Portrait ||
		m_orientation == DisplayOrientations::PortraitFlipped;
	m_renderTargetSize.Width = swapDimensions ? windowHeight : windowWidth;
	m_renderTargetSize.Height = swapDimensions ? windowWidth : windowHeight;

	if(m_swapChain != nullptr)
	{
		// スワップ チェーンが既に存在する場合は、そのサイズを変更します。
		DX::ThrowIfFailed(
			m_swapChain->ResizeBuffers(
				2, // ダブル バッファーされたスワップ チェーンです。
				static_cast<UINT>(m_renderTargetSize.Width),
				static_cast<UINT>(m_renderTargetSize.Height),
				DXGI_FORMAT_B8G8R8A8_UNORM,
				0
				)
			);
	}
	else
	{
		// それ以外の場合は、既存の Direct3D デバイスと同じアダプターを使用して、新規作成します。
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};
		swapChainDesc.Width = static_cast<UINT>(m_renderTargetSize.Width); // ウィンドウのサイズと一致させます。
		swapChainDesc.Height = static_cast<UINT>(m_renderTargetSize.Height);
		swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // これは、最も一般的なスワップ チェーンのフォーマットです。
		swapChainDesc.Stereo = false;
		swapChainDesc.SampleDesc.Count = 1; // マルチサンプリングは使いません。
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 2; // 遅延を最小限に抑えるにはダブル バッファーを使用します。
		swapChainDesc.Scaling = DXGI_SCALING_NONE;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // Windows ストア アプリはすべて、この SwapEffect を使用する必要があります。
		swapChainDesc.Flags = 0;

		ComPtr<IDXGIDevice1>  dxgiDevice;
		DX::ThrowIfFailed(
			m_d3dDevice.As(&dxgiDevice)
			);

		ComPtr<IDXGIAdapter> dxgiAdapter;
		DX::ThrowIfFailed(
			dxgiDevice->GetAdapter(&dxgiAdapter)
			);

		ComPtr<IDXGIFactory2> dxgiFactory;
		DX::ThrowIfFailed(
			dxgiAdapter->GetParent(
				__uuidof(IDXGIFactory2), 
				&dxgiFactory
				)
			);

		Windows::UI::Core::CoreWindow^ window = m_window.Get();
		DX::ThrowIfFailed(
			dxgiFactory->CreateSwapChainForCoreWindow(
				m_d3dDevice.Get(),
				reinterpret_cast<IUnknown*>(window),
				&swapChainDesc,
				nullptr, // すべてのディスプレイで許可されます。
				&m_swapChain
				)
			);
			
		// DXGI が 1 度に複数のフレームをキュー処理していないことを確認します。これにより、遅延が減少し、
		// アプリケーションが各 VSync の後でのみレンダリングすることが保証され、消費電力が最小限に抑えられます。
		DX::ThrowIfFailed(
			dxgiDevice->SetMaximumFrameLatency(1)
			);
	}
	
	// スワップ チェーンの適切な方向を設定し、回転されたスワップ チェーンにレンダリングするための
	// 3D マトリックス変換を生成します。
	DXGI_MODE_ROTATION rotation = DXGI_MODE_ROTATION_UNSPECIFIED;
	switch (m_orientation)
	{
		case DisplayOrientations::Landscape:
			rotation = DXGI_MODE_ROTATION_IDENTITY;
			m_orientationTransform3D = XMFLOAT4X4( // 0 度 Z 回転
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f
				);
			break;

		case DisplayOrientations::Portrait:
			rotation = DXGI_MODE_ROTATION_ROTATE270;
			m_orientationTransform3D = XMFLOAT4X4( // 90 度 Z 回転
				0.0f, 1.0f, 0.0f, 0.0f,
				-1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f
				);
			break;

		case DisplayOrientations::LandscapeFlipped:
			rotation = DXGI_MODE_ROTATION_ROTATE180;
			m_orientationTransform3D = XMFLOAT4X4( // 180 度 Z 回転
				-1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, -1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f
				);
			break;

		case DisplayOrientations::PortraitFlipped:
			rotation = DXGI_MODE_ROTATION_ROTATE90;
			m_orientationTransform3D = XMFLOAT4X4( // 270 度 Z 回転
				0.0f, -1.0f, 0.0f, 0.0f,
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f
				);
			break;

		default:
			throw ref new Platform::FailureException();
	}

	DX::ThrowIfFailed(
		m_swapChain->SetRotation(rotation)
		);

	// スワップ チェーンのバック バッファーのレンダリング ターゲット ビューを作成します。
	ComPtr<ID3D11Texture2D> backBuffer;
	DX::ThrowIfFailed(
		m_swapChain->GetBuffer(
			0,
			__uuidof(ID3D11Texture2D),
			&backBuffer
			)
		);

	DX::ThrowIfFailed(
		m_d3dDevice->CreateRenderTargetView(
			backBuffer.Get(),
			nullptr,
			&m_renderTargetView
			)
		);

	// 深度ステンシル ビューを作成します。
	CD3D11_TEXTURE2D_DESC depthStencilDesc(
		DXGI_FORMAT_D24_UNORM_S8_UINT, 
		static_cast<UINT>(m_renderTargetSize.Width),
		static_cast<UINT>(m_renderTargetSize.Height),
		1,
		1,
		D3D11_BIND_DEPTH_STENCIL
		);

	ComPtr<ID3D11Texture2D> depthStencil;
	DX::ThrowIfFailed(
		m_d3dDevice->CreateTexture2D(
			&depthStencilDesc,
			nullptr,
			&depthStencil
			)
		);

	CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
	DX::ThrowIfFailed(
		m_d3dDevice->CreateDepthStencilView(
			depthStencil.Get(),
			&depthStencilViewDesc,
			&m_depthStencilView
			)
		);

	// ウィンドウ全体をターゲットとするレンダリング ビューポートを作成します。
	CD3D11_VIEWPORT viewport(
		0.0f,
		0.0f,
		m_renderTargetSize.Width,
		m_renderTargetSize.Height
		);

	m_d3dContext->RSSetViewports(1, &viewport);
}

// このメソッドは、SizeChanged イベント用のイベント ハンドラーの中で呼び出されます。
void Direct3DBase::UpdateForWindowSizeChange()
{
	if (m_window->Bounds.Width  != m_windowBounds.Width ||
		m_window->Bounds.Height != m_windowBounds.Height ||
		m_orientation != DisplayProperties::CurrentOrientation)
	{
		ID3D11RenderTargetView* nullViews[] = {nullptr};
		m_d3dContext->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
		m_renderTargetView = nullptr;
		m_depthStencilView = nullptr;
		m_d3dContext->Flush();
		CreateWindowSizeDependentResources();
	}
}

// 最終イメージを画面に配信するメソッドです。
void Direct3DBase::Present()
{
	// 特定のシナリオでは、効率を良くするために、アプリケーションで "dirty" または "scroll"  rect を
	// オプションで指定できます。
	DXGI_PRESENT_PARAMETERS parameters = {0};
	parameters.DirtyRectsCount = 0;
	parameters.pDirtyRects = nullptr;
	parameters.pScrollRect = nullptr;
	parameters.pScrollOffset = nullptr;
	
	// 最初の引数は、DXGI に VSync までブロックするよう指示し、アプリケーションを次の VSync まで
	// スリープさせます。これにより、画面に表示されることのないフレームをレンダリングする
	// サイクルに時間を費やすことがなくなります。
	HRESULT hr = m_swapChain->Present1(1, 0, &parameters);

	// レンダリング ターゲットのコンテンツを破棄します。
	// この操作は、既存のコンテンツ全体が上書きされる場合のみ有効です。
	// dirty rect または scroll rect を使用する場合は、この呼び出しを削除する必要があります。
	m_d3dContext->DiscardView(m_renderTargetView.Get());

	// 深度ステンシルのコンテンツを破棄します。
	m_d3dContext->DiscardView(m_depthStencilView.Get());

	// デバイスが切断またはドライバーの更新によって削除された場合は、
	// すべてのデバイス リソースを再作成する必要があります。
	if (hr == DXGI_ERROR_DEVICE_REMOVED)
	{
		HandleDeviceLost();
	}
	else
	{
		DX::ThrowIfFailed(hr);
	}
}

// デバイスに依存しないピクセル単位 (DIP) の長さを物理的なピクセルの長さに変換するメソッド。
float Direct3DBase::ConvertDipsToPixels(float dips)
{
	static const float dipsPerInch = 96.0f;
	return floor(dips * DisplayProperties::LogicalDpi / dipsPerInch + 0.5f); // 最も近い整数値に丸めます。
}
