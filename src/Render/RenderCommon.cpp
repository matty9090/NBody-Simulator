#include "RenderCommon.hpp"

RenderView CreateTarget(ID3D11Device* device, int width, int height)
{
    RenderView t;
    t.Width = width;
    t.Height = height;

    CD3D11_TEXTURE2D_DESC desc(DXGI_FORMAT_R16G16B16A16_FLOAT, width, height,
                               1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);

    CD3D11_TEXTURE2D_DESC dsDesc(DXGI_FORMAT_D32_FLOAT, width, height,
                                 1, 1, D3D11_BIND_DEPTH_STENCIL, D3D11_USAGE_DEFAULT, 0, 1, 0 );

    CD3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc(D3D11_DSV_DIMENSION_TEXTURE2DMS);

    DX::ThrowIfFailed(device->CreateTexture2D(&desc, nullptr, t.Rt.GetAddressOf()));
    DX::ThrowIfFailed(device->CreateTexture2D(&dsDesc, nullptr, t.Ds.ReleaseAndGetAddressOf()));
    DX::ThrowIfFailed(device->CreateShaderResourceView(t.Rt.Get(), nullptr, t.Srv.ReleaseAndGetAddressOf()));
    DX::ThrowIfFailed(device->CreateRenderTargetView(t.Rt.Get(), nullptr, t.Rtv.ReleaseAndGetAddressOf()));
    DX::ThrowIfFailed(device->CreateDepthStencilView(t.Ds.Get(), &dsvDesc, t.Dsv.ReleaseAndGetAddressOf()));

    return t;
}

void CreateParticleBuffer(ID3D11Device* device, ID3D11Buffer** buffer, const std::vector<Particle>& particles)
{
    D3D11_BUFFER_DESC desc;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.ByteWidth = particles.size() * sizeof(Particle);
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA init;
    init.pSysMem = &particles[0];

    DX::ThrowIfFailed(device->CreateBuffer(&desc, &init, buffer));
}

void RenderView::Clear(ID3D11DeviceContext* context)
{
    context->ClearRenderTargetView(Rtv.Get(), DirectX::Colors::Black);
    context->ClearDepthStencilView(Dsv.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}