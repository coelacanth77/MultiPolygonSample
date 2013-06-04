﻿#pragma once

#include "Direct3DBase.h"

struct ModelViewProjectionConstantBuffer
{
	DirectX::XMFLOAT4X4 model;
	DirectX::XMFLOAT4X4 view;
	DirectX::XMFLOAT4X4 projection;
};

struct VertexPositionColor
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 color;
};

// このクラスは、スピンしている立方体を描画します。
ref class CubeRenderer sealed : public Direct3DBase
{
public:
	CubeRenderer();

	// Direct3DBase メソッド。
	virtual void CreateDeviceResources() override;
	virtual void CreateWindowSizeDependentResources() override;
	virtual void Render() override;


	// 時間に依存するオブジェクトを更新するメソッドです。
	void Update(float timeTotal, float timeDelta);

private:
	void CubeRenderer::RenderObject(Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer);

	bool m_loadingComplete;

	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer2;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_constantBuffer;

    ID3D11RasterizerState* m_pRasterizerState;
	ID3D11RasterizerState* m_pRasterizerStateBack;

	uint32 m_indexCount;
	ModelViewProjectionConstantBuffer m_constantBufferData;
};
