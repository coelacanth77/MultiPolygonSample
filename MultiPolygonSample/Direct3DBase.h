#pragma once

#include "DirectXHelper.h"

// 3D レンダリング用の DirectX API を初期化するヘルパー クラス。
ref class Direct3DBase abstract
{
internal:
	Direct3DBase();

public:
	virtual void Initialize(Windows::UI::Core::CoreWindow^ window);
	virtual void HandleDeviceLost();
	virtual void CreateDeviceResources();
	virtual void CreateWindowSizeDependentResources();
	virtual void UpdateForWindowSizeChange();
	virtual void Render() = 0;
	virtual void Present();
	virtual float ConvertDipsToPixels(float dips);

protected private:
	// Direct3D オブジェクト。
	Microsoft::WRL::ComPtr<ID3D11Device1> m_d3dDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext1> m_d3dContext;
	Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depthStencilView;

	// キャッシュされたレンダリング プロパティ。
	D3D_FEATURE_LEVEL m_featureLevel;
	Windows::Foundation::Size m_renderTargetSize;
	Windows::Foundation::Rect m_windowBounds;
	Platform::Agile<Windows::UI::Core::CoreWindow> m_window;
	Windows::Graphics::Display::DisplayOrientations m_orientation;

	// 表示方向のために使用される変換。
	DirectX::XMFLOAT4X4 m_orientationTransform3D;
};
