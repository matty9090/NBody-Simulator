#include "Skybox.hpp"
#include "Services/Log.hpp"
#include "Services/ResourceManager.hpp"

#include <DDSTextureLoader.h>

CSkyBox::CSkyBox(ID3D11DeviceContext* context) : Context(context)
{
    ID3D11Device* device;
    Context->GetDevice(&device);

    Pipeline.Topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    Pipeline.LoadVertex(L"shaders/Misc/Skybox.vsh");
    Pipeline.LoadPixel(L"shaders/Misc/Skybox.psh");
    Pipeline.CreateDepthState(device, EDepthState::Normal);
    Pipeline.CreateRasteriser(device, ECullMode::Clockwise);
    Pipeline.CreateInputLayout(device, CreateInputLayoutPositionTexture());

    Sphere = std::make_unique<CModel>(device, RESM.GetMesh("assets/Skysphere.obj"));
    Sphere->Scale(38000000.0f);
    Sphere->Rotate(DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f));

    CommonStates = std::make_unique<DirectX::CommonStates>(device);

    float border[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    CD3D11_SAMPLER_DESC samplerDesc(
        D3D11_FILTER_MIN_MAG_MIP_LINEAR,
        D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP,
        0, 1, D3D11_COMPARISON_NEVER, border,
        0, D3D11_FLOAT32_MAX
    );

    DX::ThrowIfFailed(device->CreateSamplerState(&samplerDesc, Sampler.ReleaseAndGetAddressOf()));
}

void CSkyBox::Draw(DirectX::SimpleMath::Matrix viewProj)
{
    Context->RSSetState(CommonStates->CullClockwise());
    Context->PSSetSamplers(0, 1, Sampler.GetAddressOf());
    Context->OMSetBlendState(CommonStates->Opaque(), DirectX::Colors::Black, 0xFFFFFFFF);
    Sphere->Draw(Context, viewProj, Pipeline);
}

void CSkyBox::SetTextureReceiveOwnership(ID3D11ShaderResourceView* tex)
{
    Sphere->SetTexture(tex);
    Texture.Attach(tex);
}
