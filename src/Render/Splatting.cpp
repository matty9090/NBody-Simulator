#include "Splatting.hpp"
#include "render/Shader.hpp"

#include <DirectXColors.h>

Splatting::Splatting(ID3D11DeviceContext* context, int width, int height) : Context(context)
{
    ID3D11Device* device;
    context->GetDevice(&device);

    States = std::make_unique<DirectX::CommonStates>(device);
    DualPostProcess  = std::make_unique<DirectX::DualPostProcess>(device);
    BasicPostProcess = std::make_unique<DirectX::BasicPostProcess>(device);

    LoadPixelShader(device, L"shaders/Splat.psh", PixelShader.ReleaseAndGetAddressOf());
    LoadGeometryShader(device, L"shaders/Splat.gsh", GeometryShader.ReleaseAndGetAddressOf());

    Target0 = CreateTarget(device, width, height);
    Target1 = CreateTarget(device, width, height);
}

void Splatting::Render(unsigned int num, ID3D11ShaderResourceView *scene)
{
    ID3D11RenderTargetView* OriginalTarget;
    ID3D11DepthStencilView* OriginalDepthView;
    Context->OMGetRenderTargets(1, &OriginalTarget, &OriginalDepthView);
    
    /////
    Target0.Clear(Context);
    Context->GSSetShader(GeometryShader.Get(), 0, 0);
    Context->PSSetShader(PixelShader.Get(), 0, 0);
    Context->OMSetBlendState(States->Additive(), DirectX::Colors::Black, 0xFFFFFFFF);
    Context->OMSetRenderTargets(1, Target0.Rtv.GetAddressOf(), Target0.Dsv.Get());
    Context->Draw(num, 0);
    Context->GSSetShader(nullptr, 0, 0);

    /////
    BasicPostProcess->SetEffect(DirectX::BasicPostProcess::GaussianBlur_5x5);
    BasicPostProcess->SetGaussianParameter(1.0f);
    BasicPostProcess->SetSourceTexture(Target0.Srv.Get());
    Context->OMSetBlendState(States->Additive(), DirectX::Colors::Black, 0xFFFFFFFF);
    Context->OMSetRenderTargets(1, &OriginalTarget, OriginalDepthView);
    BasicPostProcess->Process(Context);
    Context->GSSetShader(nullptr, 0, 0);
}