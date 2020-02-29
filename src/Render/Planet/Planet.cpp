#include "Planet.hpp"

#include <set>
#include <array>
#include <random>
#include <algorithm>
#include <imgui.h>

#include "Components/TerrainComponent.hpp"
#include "Components/AtmosphereComponent.hpp"

bool CPlanet::Wireframe = false;

TerrainHeightFunc::TerrainHeightFunc()
{
    Noise.SetFrequency(Frequency);
    Noise.SetFractalGain(Gain);
    Noise.SetFractalOctaves(Octaves);
}

bool TerrainHeightFunc::RenderUI()
{
    bool dirty = false;

    dirty |= ImGui::SliderFloat("Amplitude", &Amplitude, 0.04f, 1.2f);

    if (ImGui::SliderFloat("Frequency", &Frequency, 0.1f, 4.0f))
    {
        Noise.SetFrequency(Frequency);
        dirty |= true;
    }

    if (ImGui::SliderFloat("Gain", &Gain, 0.3f, 0.8f))
    {
        Noise.SetFractalGain(Gain);
        dirty |= true;
    }

    if (ImGui::SliderInt("Octaves", &Octaves, 0, 20))
    {
        Noise.SetFractalOctaves(Octaves);
        dirty |= true;
    }

    return dirty;
}

float TerrainHeightFunc::operator()(DirectX::SimpleMath::Vector3 normal, int depth)
{
    return Noise.GetSimplexFractal(normal.x, normal.y, normal.z) * Amplitude;
}

bool WaterHeightFunc::RenderUI()
{
    return ImGui::SliderFloat("Water level", &Height, -0.5f, 0.6f);
}

float WaterHeightFunc::operator()(DirectX::SimpleMath::Vector3 normal, int depth)
{
    return Height;
}

CPlanet::CPlanet(ID3D11DeviceContext* context, ICamera& cam)
    : Camera(cam),
      Context(context)
{
    Context->GetDevice(&Device);

    World = DirectX::SimpleMath::Matrix::Identity;
}

CPlanet::~CPlanet()
{
    
}

void CPlanet::Seed(uint64_t seed)
{
    RemoveAllComponents();

    auto gen = std::default_random_engine { static_cast<unsigned int>(seed) };
    std::uniform_int_distribution<> distType(0, EType::_Max);

    auto type = distType(gen);

    // Habitable
    if (type == EType::Habitable)
    {
        std::normal_distribution<float> distRadius(30.0f, 2.0f);
        Radius = distRadius(gen) + 30.0f;

        AddComponent<CAtmosphereComponent>(this, seed);
        AddComponent<CTerrainComponent<WaterHeightFunc>>(this, seed);
        AddComponent<CTerrainComponent<TerrainHeightFunc>>(this, seed);
    }
    // Rocky
    else if (type == EType::Rocky)
    {
        AddComponent<CTerrainComponent<TerrainHeightFunc>>(this, seed);
    }
    // Gas Giant
    else
    {
        std::normal_distribution<float> distRadius(220.0f, 2.0f);
        Radius = distRadius(gen) + 100.0f;

        AddComponent<CAtmosphereComponent>(this, seed);
        AddComponent<CTerrainComponent<WaterHeightFunc>>(this, seed);
    }

    LOGM(to_string())
}

void CPlanet::Update(float dt)
{
    for (auto& component : Components)
        component->Update(dt);
}

void CPlanet::Render(float scale)
{
    for (auto& component : Components)
        component->Render(Camera.GetViewMatrix() * Camera.GetProjectionMatrix());
}

void CPlanet::RenderUI()
{
    ImGui::SetNextWindowSize(ImVec2(260, Camera.GetSize().y));
    ImGui::SetNextWindowPos(ImVec2(Camera.GetSize().x, 0), 0, ImVec2(1.0f, 0.0f));
    ImGui::Begin("Planet", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    
    if (ImGui::CollapsingHeader("Planet"))
    {
        ImGui::Checkbox("Editable", &Editable);

        if (Editable)
        {
            static bool chkTerr = HasComponent<CTerrainComponent<TerrainHeightFunc>>();

            if (ImGui::Checkbox("Terrain", &chkTerr))
            {
                chkTerr ? AddComponent<CTerrainComponent<TerrainHeightFunc>>(this) : RemoveComponent<CTerrainComponent<TerrainHeightFunc>>();
            }

            static bool chkAtm = HasComponent<CAtmosphereComponent>();

            if (ImGui::Checkbox("Atmosphere", &chkAtm))
            {
                chkAtm ? AddComponent<CAtmosphereComponent>(this) : RemoveComponent<CAtmosphereComponent>();
            }
            
            static bool chkWater = HasComponent<CTerrainComponent<WaterHeightFunc>>();

            if (ImGui::Checkbox("Water", &chkWater))
            {
                chkWater ? AddComponent<CTerrainComponent<WaterHeightFunc>>(this) : RemoveComponent<CTerrainComponent<WaterHeightFunc>>();
            }
        }
    }

    ImGui::BeginChild("Components");

    if (Editable)
    {
        for (auto& component : Components)
            component->RenderUI();
    }

    ImGui::EndChild();
    ImGui::End();
}

void CPlanet::Move(DirectX::SimpleMath::Vector3 v)
{
    Position += v;
    UpdateMatrix();
}

void CPlanet::Scale(float s)
{
    PlanetScale *= s;
    UpdateMatrix();
}

void CPlanet::SetScale(float s)
{
    PlanetScale = s;
    UpdateMatrix();
}

void CPlanet::SetPosition(DirectX::SimpleMath::Vector3 p)
{
    Position = p;
    UpdateMatrix();
}

void CPlanet::RemoveAllComponents()
{
    Components.clear();
}

std::string CPlanet::to_string() const
{
    std::string type = (Type == Rocky) ? "Rocky" : (Type == Habitable ? "Habitable" : "Gas Giant");

    std::ostringstream ss;
    ss << "Planet " << Name << " (" << type << ")\n";
    
    for (const auto& c : Components)
    {
        ss << "\tComponent [" << c->GetName() << "]\n";
    }

    return ss.str();
}

void CPlanet::UpdateMatrix()
{
    World = DirectX::SimpleMath::Matrix::CreateScale(PlanetScale) *
            DirectX::SimpleMath::Matrix::CreateTranslation(Position); 
}

void CPlanet::RefreshComponents()
{
    for (auto& c : Components)
        c->Init();
}
