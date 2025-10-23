#include "pch.h"
#include "Texture/Public/Texture.h"

#include "Texture/Public/TextureRenderProxy.h"

IMPLEMENT_CLASS(UTexture, UObject)

/**
 * @brief Default Constructor
 */
UTexture::UTexture() = default;

UTexture::~UTexture()
{
    if (RenderProxy)
    {
        SafeDelete(RenderProxy);
    }
}

void UTexture::CreateRenderProxy(const ComPtr<ID3D11ShaderResourceView>& SRV, const ComPtr<ID3D11SamplerState>& Sampler)
{
    if (RenderProxy)
    {
        SafeDelete(RenderProxy);
    }

    RenderProxy = new FTextureRenderProxy(SRV, Sampler);
}

ID3D11ShaderResourceView* UTexture::GetTextureSRV() const
{
    if (RenderProxy)
    {
        return RenderProxy->GetSRV();
    }
    return nullptr;
}

ID3D11SamplerState* UTexture::GetTextureSampler() const
{
    if (RenderProxy)
    {
        return RenderProxy->GetSampler();
    }
    return nullptr;
}
