#include "Model.hpp"
#include "Mesh.hpp"

using DirectX::SimpleMath::Matrix;

CModel::CModel(ID3D11Device* device, CMesh* mesh) : MatrixBuffer(device), Mesh(mesh)
{
    Texture = Mesh->Texture.Get();
}

void CModel::Move(DirectX::SimpleMath::Vector3 v)
{
    Position += v;
    UpdateMatrices();
}

void CModel::SetPosition(DirectX::SimpleMath::Vector3 p)
{
    Position = p;
    UpdateMatrices();
}

void CModel::Rotate(DirectX::SimpleMath::Vector3 r)
{
    Rotation *= DirectX::SimpleMath::Quaternion::CreateFromYawPitchRoll(r.y, r.x, r.z);
    UpdateMatrices();
}

void CModel::Scale(float s)
{
    RelativeScale *= s;
    UpdateMatrices();
}

void CModel::SetScale(float s)
{
    RelativeScale = s;
    UpdateMatrices();
}

void CModel::Draw(ID3D11DeviceContext* context, DirectX::SimpleMath::Matrix viewProj, const RenderPipeline& pipeline)
{
    MatrixBuffer.SetData(context, { World * viewProj, World });
    
    pipeline.SetState(context, [&]() {
        unsigned int offset = 0;
        unsigned int stride = sizeof(MeshVertex);

        context->IASetVertexBuffers(0, 1, Mesh->VertexBuffer.GetAddressOf(), &stride, &offset);
        context->IASetIndexBuffer(Mesh->IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        context->VSSetConstantBuffers(0, 1, MatrixBuffer.GetBuffer());
        context->PSSetShaderResources(0, 1, &Texture);

        context->DrawIndexed(Mesh->NumIndices, 0, 0);
    });
}

void CModel::Draw(ID3D11DeviceContext* context, DirectX::SimpleMath::Matrix viewProj, DirectX::SimpleMath::Matrix parentWorld, const RenderPipeline& pipeline)
{
    auto world = World * parentWorld;

    MatrixBuffer.SetData(context, { world * viewProj, world });

    pipeline.SetState(context, [&]() {
        unsigned int offset = 0;
        unsigned int stride = sizeof(MeshVertex);

        context->IASetVertexBuffers(0, 1, Mesh->VertexBuffer.GetAddressOf(), &stride, &offset);
        context->IASetIndexBuffer(Mesh->IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        context->VSSetConstantBuffers(0, 1, MatrixBuffer.GetBuffer());
        context->PSSetShaderResources(0, 1, &Texture);

        context->DrawIndexed(Mesh->NumIndices, 0, 0);
    });
}

void CModel::UpdateMatrices()
{
    World = Matrix::CreateScale(RelativeScale) *
            Matrix::CreateFromQuaternion(Rotation) *
            Matrix::CreateTranslation(Position);
}
