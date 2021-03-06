#pragma once

#include <d3d11.h>
#include <memory>
#include <wrl/client.h>
#include <SimpleMath.h>
#include <CommonStates.h>
#include <PostProcess.h>

#include "Render/DX/RenderCommon.hpp"
#include "Render/DX/ConstantBuffer.hpp"

class CSplatting
{
public:
    CSplatting(ID3D11DeviceContext* context, int width, int height);

    void Render(unsigned int num, DirectX::SimpleMath::Vector3 cam);

private:
    ID3D11DeviceContext* Context;

    std::unique_ptr<DirectX::CommonStates> States;
    std::unique_ptr<DirectX::DualPostProcess>  DualPostProcess;
    std::unique_ptr<DirectX::BasicPostProcess> BasicPostProcess;

    ID3D11PixelShader* PixelShader;
    ID3D11GeometryShader* GeometryShader;

    RenderView Target0;
    RenderView Target1;

    struct CamBuffer
    {
        DirectX::SimpleMath::Vector3 Eye;
        float Custom;
    };

    std::unique_ptr<ConstantBuffer<CamBuffer>> Buffer;
};