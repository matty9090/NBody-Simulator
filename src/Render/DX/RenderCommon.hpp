#pragma once

#include <string>
#include <vector>
#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXColors.h>
#include <functional>

#include "Core/Except.hpp"
#include "Render/Misc/Particle.hpp"

enum class ECullMode { None, Clockwise, Anticlockwise };

struct RenderView
{
    Microsoft::WRL::ComPtr<ID3D11Texture2D> Rt;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> Ds;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> Rtv;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> Dsv;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Srv;

    int Width, Height;

    void Clear(ID3D11DeviceContext* context);
};

struct RenderPipeline
{
    ID3D11VertexShader*    VertexShader = nullptr;
    ID3D11GeometryShader*  GeometryShader = nullptr;
    ID3D11PixelShader*     PixelShader = nullptr;

    Microsoft::WRL::ComPtr<ID3D11InputLayout> InputLayout;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> Raster;

    D3D_PRIMITIVE_TOPOLOGY Topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    void LoadVertex(std::wstring file);
    void LoadPixel(std::wstring file);
    void LoadGeometry(std::wstring file);
    void CreateRasteriser(ID3D11Device* device, ECullMode cullMode);
    void CreateInputLayout(ID3D11Device* device, std::vector<D3D11_INPUT_ELEMENT_DESC> layout);

    void SetState(ID3D11DeviceContext* context, std::function<void()> state) const;

private:
    ID3DBlob* VertexCode;
};

RenderView CreateTarget(ID3D11Device* device, int width, int height);
void SetRenderTarget(ID3D11DeviceContext* context, RenderView& view);
void CreateParticleBuffer(ID3D11Device* device, ID3D11Buffer** buffer, const std::vector<Particle>& particles);

std::vector<D3D11_INPUT_ELEMENT_DESC> CreateInputLayoutPosition();
std::vector<D3D11_INPUT_ELEMENT_DESC> CreateInputLayoutPositionColour();
std::vector<D3D11_INPUT_ELEMENT_DESC> CreateInputLayoutPositionTexture();
std::vector<D3D11_INPUT_ELEMENT_DESC> CreateInputLayoutPositionTextureCube();
std::vector<D3D11_INPUT_ELEMENT_DESC> CreateInputLayoutPositionNormalTexture();