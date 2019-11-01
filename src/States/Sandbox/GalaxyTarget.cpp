#include "GalaxyTarget.hpp"

GalaxyTarget::GalaxyTarget(ID3D11DeviceContext* context, std::string name, DX::DeviceResources* resources, CShipCamera* camera, const std::vector<Particle>& seedData)
    : SandboxTarget(context, name, resources, camera),
      Particles(seedData)
{
    auto vp = Resources->GetScreenViewport();
    unsigned int width = static_cast<size_t>(vp.Width);
    unsigned int height = static_cast<size_t>(vp.Height);

    PostProcess = std::make_unique<CPostProcess>(Device, Context, width, height);
    CommonStates = std::make_unique<DirectX::CommonStates>(Device);

    CreateParticlePipeline();
}

void GalaxyTarget::Render()
{
    auto rtv = Resources->GetRenderTargetView();
    auto dsv = Resources->GetDepthStencilView();

    ParticleRenderTarget.Clear(Context);
    SetRenderTarget(Context, ParticleRenderTarget);

    Matrix view = Camera->GetViewMatrix();
    Matrix viewProj = view * Camera->GetProjectionMatrix();

    ParticlePipeline.SetState(Context, [&]() {
        unsigned int offset = 0;
        unsigned int stride = sizeof(Particle);

        Context->IASetVertexBuffers(0, 1, ParticleBuffer.GetAddressOf(), &stride, &offset);
        GSBuffer->SetData(Context, GSConstantBuffer{ viewProj, view.Invert() });
        Context->GSSetConstantBuffers(0, 1, GSBuffer->GetBuffer());
        Context->RSSetState(CommonStates->CullNone());
        Context->OMSetBlendState(CommonStates->Additive(), DirectX::Colors::Black, 0xFFFFFFFF);
        Context->Draw(static_cast<unsigned int>(Particles.size()), 0);
    });

    Context->GSSetShader(nullptr, 0, 0);
    PostProcess->Render(rtv, dsv, ParticleRenderTarget.Srv.Get());
}

void GalaxyTarget::MoveObjects(Vector3 v)
{
    for (auto& particle : Particles)
        particle.Position += v;

    D3D11_MAPPED_SUBRESOURCE mapped;
    Context->Map(ParticleBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, Particles.data(), Particles.size() * sizeof(Particle));
    Context->Unmap(ParticleBuffer.Get(), 0);
}

Vector3 GalaxyTarget::GetClosestObject(Vector3 pos) const
{
    return Vector3();
}

void GalaxyTarget::StateIdle()
{
    /*auto closest = Maths::ClosestParticle(Ship->GetPosition(), Particles);
    float d = Vector3::Distance(Ship->GetPosition(), closest.Position);

    float scaledDist = Maths::Clamp((d - 400.0f) / 1800.0f, 0.0f, 1.0f);
    Ship->VelocityScale = Maths::Lerp(StellarSpeed, GalacticSpeed, scaledDist);*/
}

void GalaxyTarget::CreateParticlePipeline()
{
    std::vector<D3D11_INPUT_ELEMENT_DESC> layout = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT   , 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    ParticlePipeline.Topology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    ParticlePipeline.LoadVertex(Device, L"shaders/PassThruGS.vsh");
    ParticlePipeline.LoadPixel(Device, L"shaders/PlainColour.psh");
    ParticlePipeline.LoadGeometry(Device, L"shaders/SandboxParticle.gsh");
    ParticlePipeline.CreateInputLayout(Device, layout);

    CreateParticleBuffer(Device, ParticleBuffer.ReleaseAndGetAddressOf(), Particles);
    GSBuffer = std::make_unique<ConstantBuffer<GSConstantBuffer>>(Device);

    auto vp = Resources->GetScreenViewport();
    ParticleRenderTarget = CreateTarget(Device, static_cast<int>(vp.Width), static_cast<int>(vp.Height));
}
