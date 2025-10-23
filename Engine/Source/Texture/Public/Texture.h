#pragma once
#include "Core/Public/Object.h"

struct FTextureRenderProxy;

UCLASS()
class UTexture : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UTexture, UObject)

public:
	// Special member function
	UTexture();
	~UTexture() override;

	// Getter & Setter
	uint32 GetWidth() const { return Width; }
	uint32 GetHeight() const { return Height; }

	FName GetFilePath() const { return TextureFilePath.ToString();  }
	void SetFilePath(const FName& InFilePath) { TextureFilePath = InFilePath;  }

	const FTextureRenderProxy* GetRenderProxy() const { return RenderProxy; }
	void CreateRenderProxy(const ComPtr<ID3D11ShaderResourceView>& SRV, const ComPtr<ID3D11SamplerState>& Sampler);

	/// @note 내부의 SRV가 ComPtr로 되어있으므로 반환되는 SRV로 Release 등 소유권과 관련있는 행위를 하지 말 것
	ID3D11ShaderResourceView* GetTextureSRV() const;
	/// @note 내부의 Sampler가 ComPtr로 되어있으므로 반환되는 Sampler로 Release 등 소유권과 관련있는 행위를 하지 말 것
	ID3D11SamplerState* GetTextureSampler() const;

private:
	FName TextureFilePath;
	uint32 Width = 0;
	uint32 Height = 0;

	FTextureRenderProxy* RenderProxy = nullptr;

	friend class UAssetManager;
};
