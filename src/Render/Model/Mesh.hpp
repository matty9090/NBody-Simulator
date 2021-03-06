#pragma once

#include <string>
#include <vector>
#include <memory>
#include <d3d11.h>
#include <wrl/client.h>
#include <SimpleMath.h>

#include <assimp/scene.h>
#include <assimp/importer.hpp>
#include <assimp/postprocess.h>

#include "Render/DX/RenderCommon.hpp"

struct MeshVertex
{
    DirectX::SimpleMath::Vector3 Position;
    DirectX::SimpleMath::Vector2 TexCoord;
};

class CMesh
{
public:
    CMesh(ID3D11Device* device, std::vector<MeshVertex> vertices, std::vector<unsigned int> indices, ID3D11ShaderResourceView* texture);

    static std::unique_ptr<CMesh> Load(ID3D11Device* device, std::string file);

    Microsoft::WRL::ComPtr<ID3D11Buffer> VertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> IndexBuffer;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Texture;

    unsigned int NumIndices = 0;
};