#include "pch.h"
#include "CubeRenderer.h"

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::UI::Core;

CubeRenderer::CubeRenderer() :
	m_loadingComplete(false),
	m_indexCount(0)
{
}

void CubeRenderer::CreateDeviceResources()
{
	Direct3DBase::CreateDeviceResources();

	auto loadVSTask = DX::ReadDataAsync("SimpleVertexShader.cso");
	auto loadPSTask = DX::ReadDataAsync("SimplePixelShader.cso");

	auto createVSTask = loadVSTask.then([this](Platform::Array<byte>^ fileData) {
		DX::ThrowIfFailed(
			m_d3dDevice->CreateVertexShader(
 				fileData->Data,
				fileData->Length,
				nullptr,
				&m_vertexShader
				)
			);

		const D3D11_INPUT_ELEMENT_DESC vertexDesc[] = 
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		DX::ThrowIfFailed(
			m_d3dDevice->CreateInputLayout(
				vertexDesc,
				ARRAYSIZE(vertexDesc),
				fileData->Data,
				fileData->Length,
				&m_inputLayout
				)
			);
	});

	auto createPSTask = loadPSTask.then([this](Platform::Array<byte>^ fileData) {
		DX::ThrowIfFailed(
			m_d3dDevice->CreatePixelShader(
				fileData->Data,
				fileData->Length,
				nullptr,
				&m_pixelShader
				)
			);

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_d3dDevice->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_constantBuffer
				)
			);
	});

	// 2つのポリゴンを描画するためのVertexBufferを用意
	auto createCubeTask = (createPSTask && createVSTask).then([this] () {
		VertexPositionColor cubeVertices[] = 
		{
			{XMFLOAT3(0.5f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f)},
			{XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f)},
			{XMFLOAT3(0.0f, 0.5f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f)},
		};

		D3D11_SUBRESOURCE_DATA vertexBufferData = {0};
		vertexBufferData.pSysMem = cubeVertices;
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(cubeVertices), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			m_d3dDevice->CreateBuffer(
				&vertexBufferDesc,
				&vertexBufferData,
				&m_vertexBuffer
				)
			);

		VertexPositionColor cubeVertices2[] = 
		{
			{XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT3(0.6f, 1.0f, 1.0f)},
			{XMFLOAT3(0.0f, -0.0f, 0.0f), XMFLOAT3(0.6f, 1.0f, 1.0f)},
			{XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT3(0.6f, 1.0f, 1.0f)},
		};

		D3D11_SUBRESOURCE_DATA vertexBufferData2 = {0};
		vertexBufferData2.pSysMem = cubeVertices2;
		vertexBufferData2.SysMemPitch = 0;
		vertexBufferData2.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc2(sizeof(cubeVertices2), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			m_d3dDevice->CreateBuffer(
				&vertexBufferDesc2,
				&vertexBufferData2,
				&m_vertexBuffer2
				)
			);


		unsigned short cubeIndices[] = 
		{
			0,1,2
		};

		m_indexCount = ARRAYSIZE(cubeIndices);

		D3D11_SUBRESOURCE_DATA indexBufferData = {0};
		indexBufferData.pSysMem = cubeIndices;
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(cubeIndices), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(
			m_d3dDevice->CreateBuffer(
				&indexBufferDesc,
				&indexBufferData,
				&m_indexBuffer
				)
			);

		D3D11_RASTERIZER_DESC rdc;
		ZeroMemory(&rdc, sizeof(rdc));

		rdc.FillMode = D3D11_FILL_SOLID;
		rdc.CullMode = D3D11_CULL_FRONT;
		rdc.FrontCounterClockwise = true;

		m_d3dDevice->CreateRasterizerState(&rdc, &m_pRasterizerState);

	    rdc.CullMode = D3D11_CULL_BACK;
		m_d3dDevice->CreateRasterizerState(&rdc, &m_pRasterizerStateBack);
	});

	createCubeTask.then([this] () {
		m_loadingComplete = true;
	});
}

void CubeRenderer::CreateWindowSizeDependentResources()
{
	Direct3DBase::CreateWindowSizeDependentResources();

	float aspectRatio = m_windowBounds.Width / m_windowBounds.Height;
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	// m_orientationTransform3D マトリックスは、ここで事後乗算されます。
	// それにより、シーンの方向を表示方向と正しく一致させます。
	// この事後乗算ステップは、スワップ チェーンのターゲット ビットマップに対して行われるすべての
	// 描画呼び出しで実行する必要があります。他のターゲットに対する呼び出しでは、
	// 適用する必要はありません。
	XMStoreFloat4x4(
		&m_constantBufferData.projection,
		XMMatrixTranspose(
			XMMatrixMultiply(
				XMMatrixPerspectiveFovRH(
					fovAngleY,
					aspectRatio,
					0.01f,
					100.0f
					),
				XMLoadFloat4x4(&m_orientationTransform3D)
				)
			)
		);
}

void CubeRenderer::Update(float timeTotal, float timeDelta)
{
	(void) timeDelta; // 未使用のパラメーター。

	XMVECTOR eye = XMVectorSet(0.0f, 0.7f, 1.5f, 0.0f);
	XMVECTOR at = XMVectorSet(0.0f, -0.1f, 0.0f, 0.0f);
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixLookAtRH(eye, at, up)));
	XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(XMMatrixRotationY(timeTotal * XM_PIDIV4)));
}

void CubeRenderer::Render()
{
	const float midnightBlue[] = { 0.098f, 0.098f, 0.439f, 1.000f };
	m_d3dContext->ClearRenderTargetView(
		m_renderTargetView.Get(),
		midnightBlue
		);

	m_d3dContext->ClearDepthStencilView(
		m_depthStencilView.Get(),
		D3D11_CLEAR_DEPTH,
		1.0f,
		0
		);

	// キューブを読み込み時に 1 度だけ描画します (読み込みは非同期です)。
	if (!m_loadingComplete)
	{
		return;
	}

	m_d3dContext->OMSetRenderTargets(
		1,
		m_renderTargetView.GetAddressOf(),
		m_depthStencilView.Get()
		);

	m_d3dContext->UpdateSubresource(
		m_constantBuffer.Get(),
		0,
		NULL,
		&m_constantBufferData,
		0,
		0
		);

	// 複数オブジェクトの描画に対応するためにメソッド化
	// さらに増やしたい場合はVertexBufferを配列に格納してぐるぐる回すと良さそう
	this->RenderObject(m_vertexBuffer);
	this->RenderObject(m_vertexBuffer2);
}

/**
 * 複数のポリゴンの描画に対応するためにメソッド抽出
 */
void CubeRenderer::RenderObject(Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer)
{

	UINT stride = sizeof(VertexPositionColor);
	UINT offset = 0;
	m_d3dContext->IASetVertexBuffers(
		0,
		1,
		m_vertexBuffer.GetAddressOf(),
		&stride,
		&offset
		);

	m_d3dContext->IASetIndexBuffer(
		m_indexBuffer.Get(),
		DXGI_FORMAT_R16_UINT,
		0
		);

	m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_d3dContext->IASetInputLayout(m_inputLayout.Get());

	m_d3dContext->VSSetShader(
		m_vertexShader.Get(),
		nullptr,
		0
		);

	m_d3dContext->VSSetConstantBuffers(
		0,
		1,
		m_constantBuffer.GetAddressOf()
		);

	m_d3dContext->PSSetShader(
		m_pixelShader.Get(),
		nullptr,
		0
		);

    m_d3dContext->RSSetState(m_pRasterizerState);

	m_d3dContext->DrawIndexed(
		m_indexCount,
		0,
		0
		);

    m_d3dContext->RSSetState(m_pRasterizerStateBack);

	m_d3dContext->DrawIndexed(
		m_indexCount,
		0,
		0
		);
}