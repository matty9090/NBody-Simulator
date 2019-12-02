#include "UniverseTarget.hpp"
#include "Sim/IParticleSeeder.hpp"

UniverseTarget::UniverseTarget(ID3D11DeviceContext* context, DX::DeviceResources* resources, CShipCamera* camera, ID3D11RenderTargetView* rtv)
    : SandboxTarget(context, "Universal", resources, camera, rtv)
{
    Scale = 0.006f;
    BeginTransitionDist = 3200.0f;
    EndTransitionDist = 300.0f;

    auto vp = Resources->GetScreenViewport();
    unsigned int width = static_cast<size_t>(vp.Width);
    unsigned int height = static_cast<size_t>(vp.Height);

    Splatting = std::make_unique<CSplatting>(Context, width, height);
    PostProcess = std::make_unique<CPostProcess>(Device, Context, width, height);
    CommonStates = std::make_unique<DirectX::CommonStates>(Device);

    Seed(0);
}

void UniverseTarget::Render()
{
    RenderLerp(1.0f);
}

void UniverseTarget::RenderTransitionParent(float t)
{
    RenderLerp(t, true);
}

void UniverseTarget::MoveObjects(Vector3 v)
{
    for (auto& galaxy : Galaxies)
        galaxy->Move(v);

    Centre += v;
}

void UniverseTarget::ScaleObjects(float scale)
{
    for (auto& galaxy : Galaxies)
        galaxy->Scale(scale);
}

Vector3 UniverseTarget::GetClosestObject(Vector3 pos)
{
    return Maths::ClosestObject(pos, Galaxies, &CurrentClosestObjectID)->GetPosition();
}

void UniverseTarget::RenderLerp(float t, bool single)
{
    auto dsv = Resources->GetDepthStencilView();
    Context->OMSetRenderTargets(1, &RenderTarget, dsv);

    for (auto& galaxy : Galaxies)
        galaxy->Render(*Camera, t, 1.0f, Vector3::Zero, single);
}

void UniverseTarget::Seed(uint64_t seed)
{
    Galaxies.clear();

    std::vector<Particle> particles;
    particles.resize(60);

    auto seeder = CreateParticleSeeder(particles, EParticleSeeder::Random);
    seeder->Seed(seed);

    uint64_t i = 0;

    for (const auto& particle : particles)
    {
        Galaxies.push_back(std::make_unique<Galaxy>(Context));
        Galaxies.back()->Seed(++i);
        Galaxies.back()->Scale(4000.0f);
        Galaxies.back()->Move(particle.Position / 0.02f);
    }
}

void UniverseTarget::BakeSkybox(Vector3 object)
{
    SkyboxGenerator->Render([&](const ICamera& cam) {
        for (auto& galaxy : Galaxies)
            galaxy->Render(cam, 1.0f);
    });
}