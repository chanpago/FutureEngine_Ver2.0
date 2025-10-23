#pragma once

#include "Component/Public/PrimitiveComponent.h"
#include "Global/Vector.h"

UCLASS()
class UBillBoardComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UBillBoardComponent, UPrimitiveComponent)

public:
	UBillBoardComponent();
	virtual ~UBillBoardComponent() override;

	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	void FaceCamera(const FVector& CameraForward);

	UTexture* GetSprite() const;
	void SetSprite(UTexture* Sprite);

	FVector4 GetSpriteTint() const { return SpriteTint; }
	void SetSpriteTint(const FVector4& InTint);
	void SetSpriteTint(const FVector& InTint, float Alpha = 1.0f);

	UClass* GetSpecificWidgetClass() const override;

	static const FRenderState& GetClassDefaultRenderState(); 

private:
	UTexture* Sprite = nullptr;
	FVector4 SpriteTint = FVector4::OneVector();

// Screen Size Section
public:	
	bool IsScreenSizeScaled() const { return bScreenSizeScaled; }
	float GetScreenSize() const { return ScreenSize; }
	void SetScreenSizeScaled(bool bEnable, float InScreenSize = 0.1f)
	{
		bScreenSizeScaled = bEnable;
		ScreenSize = InScreenSize;
	}

private:
	bool bScreenSizeScaled = false;
	float ScreenSize = 0.1f;
};
