#include "Galaxy.hpp"
#include "Core/Maths.hpp"
#include "Services/Log.hpp"
#include "Services/ResourceManager.hpp"
#include "Sim/IParticleSeeder.hpp"

#include <random>

using namespace DirectX::SimpleMath;

float Galaxy::ImposterThreshold = 8000.0f;
float Galaxy::ImposterFadeDist = 600.0f;
float Galaxy::ImposterOffsetPercent = 0.4f;

Microsoft::WRL::ComPtr<ID3D11Buffer> Galaxy::ParticleBuffer;

Galaxy::Galaxy(ID3D11DeviceContext* context) : Context(context)
{
    context->GetDevice(&Device);

    std::vector<D3D11_INPUT_ELEMENT_DESC> layout = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT   , 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    ParticlePipeline.Topology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    ParticlePipeline.LoadVertex(L"shaders/PassThruGS.vsh");
    ParticlePipeline.LoadPixel(L"shaders/Particles/StarParticle.psh");
    ParticlePipeline.LoadGeometry(L"shaders/Particles/GalaxyParticle.gsh");
    ParticlePipeline.CreateRasteriser(Device, ECullMode::Anticlockwise);
    ParticlePipeline.CreateInputLayout(Device, layout);    

    CommonStates = std::make_unique<DirectX::CommonStates>(Device);
    GSBuffer = std::make_unique<ConstantBuffer<GSConstantBuffer>>(Device);
    LerpBuffer = std::make_unique<ConstantBuffer<LerpConstantBuffer>>(Device);
    Imposter = std::make_unique<CBillboard>(Context, L"assets/GalaxyImposter.png");
    DustRenderer = std::make_unique<CBillboard>(Context, L"assets/Fog.png", 860);

    StarTexture = RESM.GetTexture(L"assets/StarImposter.png");
}

void Galaxy::InitialSeed(uint64_t seed)
{
    Seed = seed;

    std::default_random_engine gen { static_cast<unsigned int>(Seed) };
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    Colour = Color(dist(gen), dist(gen), dist(gen));

    BillboardInstance inst = { Vector3::Zero, ImposterSize, Colour };
    Imposter->UpdateInstances(std::vector<BillboardInstance> { inst });
    Imposter->SetPosition(Position);
}

void Galaxy::FinishSeed(const std::vector<Particle>& particles, const std::vector<BillboardInstance>& clouds)
{
    Particles = particles;
    DustClouds = clouds;

    DustRenderer->UpdateInstances(DustClouds);
}

void Galaxy::Move(Vector3 v)
{
    for (auto& particle : Particles) particle.Position += v;
    for (auto& cloud : DustClouds) cloud.Position += v;

    Position += v;
    Imposter->SetPosition(Position);
    RegenerateBuffer();
}

void Galaxy::Scale(float scale)
{
    for (auto& particle : Particles) particle.Position /= scale;
    for (auto& cloud : DustClouds) cloud.Position /= scale;

    RegenerateBuffer();
}

void Galaxy::Render(const ICamera& cam, float t, float scale, Vector3 voffset, bool single)
{
    if (Particles.size() <= 0)
        return;

    Context->OMSetDepthStencilState(CommonStates->DepthRead(), 0);

    Matrix view = cam.GetViewMatrix();
    Matrix viewProj = view * cam.GetProjectionMatrix();

    view = view.Invert();
    view *= Matrix::CreateScale(scale);

    ParticlePipeline.SetState(Context, [&]() {
        unsigned int offset = 0;
        unsigned int stride = sizeof(Particle);

        LerpBuffer->SetData(Context, LerpConstantBuffer { 1.0f });

        Context->IASetVertexBuffers(0, 1, ParticleBuffer.GetAddressOf(), &stride, &offset);
        GSBuffer->SetData(Context, GSConstantBuffer { viewProj, view, voffset });
        Context->GSSetConstantBuffers(0, 1, GSBuffer->GetBuffer());
        Context->GSSetConstantBuffers(1, 1, LerpBuffer->GetBuffer());
        Context->OMSetBlendState(CommonStates->Additive(), DirectX::Colors::Black, 0xFFFFFFFF);
        Context->PSSetShaderResources(0, 1, StarTexture.GetAddressOf());

        auto sampler = CommonStates->AnisotropicClamp();
        Context->PSSetSamplers(0, 1, &sampler);

        if (single)
        {
            auto pivot1 = static_cast<UINT>(CurrentClosestObjectID);
            auto pivot2 = static_cast<UINT>(Particles.size() - CurrentClosestObjectID - 1);

            Context->Draw(pivot1, 0);
            Context->Draw(pivot2, static_cast<UINT>(CurrentClosestObjectID + 1));

            LerpBuffer->SetData(Context, LerpConstantBuffer { t });
            Context->GSSetConstantBuffers(1, 1, LerpBuffer->GetBuffer());
            Context->Draw(1, static_cast<UINT>(CurrentClosestObjectID));
        }
        else
        {
            Context->Draw(static_cast<UINT>(Particles.size()), 0);
        }
    });

    Context->OMSetBlendState(CommonStates->NonPremultiplied(), DirectX::Colors::Black, 0xFFFFFFFF);
    DustRenderer->Render(cam, scale, voffset);
        
    Context->GSSetShader(nullptr, 0, 0);
    Context->OMSetDepthStencilState(CommonStates->DepthDefault(), 0);
}

void Galaxy::RenderImposter(const ICamera& cam)
{
    float dist = ((Vector3::Distance(cam.GetPosition(), Position) - ImposterFadeDist) / ImposterThreshold) - 1.0f;
    float imposterT = Maths::Clamp(dist + ImposterOffsetPercent, 0.0f, 1.0f);

    Context->OMSetDepthStencilState(CommonStates->DepthRead(), 0);

    if (imposterT > 0.0f)
    {
        BillboardInstance inst = { Vector3::Zero, ImposterSize, Color(Colour.R(), Colour.G(), Colour.B(), imposterT) };

        auto sampler = CommonStates->AnisotropicClamp();
        Context->PSSetSamplers(0, 1, &sampler);

        Context->OMSetBlendState(CommonStates->Additive(), DirectX::Colors::Black, 0xFFFFFFFF);
        Imposter->UpdateInstances(std::vector<BillboardInstance> { inst });
        Imposter->Render(cam);
    }

    Context->OMSetBlendState(CommonStates->NonPremultiplied(), DirectX::Colors::Black, 0xFFFFFFFF);
    DustRenderer->Render(cam);

    Context->OMSetDepthStencilState(CommonStates->DepthDefault(), 0);
}

Vector3 Galaxy::GetClosestObject(Vector3 pos)
{
    return Particles.size() > 0 ? Maths::ClosestParticle(pos, Particles, &CurrentClosestObjectID).Position : Vector3::Zero;
}

void Galaxy::RegenerateBuffer()
{
    if (Particles.size() <= 0)
        return;

    D3D11_MAPPED_SUBRESOURCE mapped;
    Context->Map(ParticleBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, Particles.data(), Particles.size() * sizeof(Particle));
    Context->Unmap(ParticleBuffer.Get(), 0);

    DustRenderer->UpdateInstances(DustClouds);
}