#pragma once

#include <map>
#include <list>
#include <vector>
#include <memory>
#include <typeinfo>
#include <d3d11.h>
#include <SimpleMath.h>
#include <CommonStates.h>

#include "Render/DX/RenderCommon.hpp"
#include "Render/Cameras/Camera.hpp"
#include "Render/Model/Model.hpp"

#include "Misc/FastNoise.hpp"
#include "Components/PlanetComponent.hpp"

class TerrainHeightFunc
{
public:
    TerrainHeightFunc();

    float operator()(DirectX::SimpleMath::Vector3 normal, int depth = 0);

    std::wstring PixelShader = L"shaders/Planet/Planet";

private:
    FastNoise Noise;
    const float Amplitude = 4.0f;
};

class WaterHeightFunc
{
public:
    float operator()(DirectX::SimpleMath::Vector3 normal, int depth = 0);

    std::wstring PixelShader = L"shaders/Planet/PlanetWater";
};

class CPlanet
{
public:
    CPlanet(ID3D11DeviceContext* context, ICamera& cam);
    ~CPlanet();

    void Seed(uint64_t seed);
    void Update(float dt);
    void Render(float scale = 1.0f);
    void Move(DirectX::SimpleMath::Vector3 v);
    void Scale(float s);
    void SetScale(float s);
    void SetPosition(DirectX::SimpleMath::Vector3 p);
    
    template <class Component>
    bool HasComponent();
    
    float GetScale() const { return PlanetScale; }
    float GetHeight(DirectX::SimpleMath::Vector3 normal);

    DirectX::SimpleMath::Vector3 GetPosition() const { return Position; }

    ID3D11Device* GetDevice() const { return Device; }
    ID3D11DeviceContext* GetContext() const { return Context; }
    
    ICamera& Camera;
    float Radius = 200.0f;
    float SplitDistance = 720.0f;

    DirectX::SimpleMath::Vector3 LightSource;
    DirectX::SimpleMath::Matrix World;

    static bool Wireframe;

private:
    ID3D11Device* Device;
    ID3D11DeviceContext* Context;

    float PlanetScale = 1.0f;
    DirectX::SimpleMath::Vector3 Position;

    std::list<std::unique_ptr<IPlanetComponent>> Components;
    
    void UpdateMatrix();
};

template <class Component>
bool CPlanet::HasComponent()
{
    for (const auto& component : Components)
    {
        if (typeid(*component) == typeid(Component))
        {
            return true;
        }
    }

    return false;
}