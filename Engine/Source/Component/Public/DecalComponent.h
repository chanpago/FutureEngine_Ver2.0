#pragma once
#include "Component/Public/PrimitiveComponent.h"

UCLASS()
class UDecalComponent : public UPrimitiveComponent
{
    GENERATED_BODY()
    DECLARE_CLASS(UDecalComponent, UPrimitiveComponent)

public:
    UDecalComponent();
    virtual ~UDecalComponent() override;

    virtual void TickComponent(float DeltaTime) override;
    virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

    void SetTexture(UTexture* InTexture);
    
    void SetFadeTexture(UTexture* InFadeTexture);
    
    virtual UTexture* GetTexture() const { return DecalTexture; }

    virtual UTexture* GetFadeTexture() const { return FadeTexture; }

    const TPair<FName, ID3D11ShaderResourceView*>& GetSprite() const;
    UClass* GetSpecificWidgetClass() const override;

    // --- Perspective Projection ---
    void SetPerspective(bool bEnable);
    //void SetFOV(float InFOV) { FOV = InFOV; UpdateProjectionMatrix(); UpdateOBB(); }
    //void SetAspectRatio(float InAspectRatio) { AspectRatio = InAspectRatio; UpdateProjectionMatrix(); UpdateOBB(); }
    //void SetClipDistances(float InNear, float InFar) { NearClip = InNear; FarClip = InFar; UpdateProjectionMatrix(); UpdateOBB(); }

    FMatrix GetProjectionMatrix() const { return ProjectionMatrix; }
    bool IsPerspective() const { return bIsPerspective; }

    virtual void UpdateProjectionMatrix();

protected:
    UTexture* DecalTexture = nullptr;
    
    UTexture* FadeTexture = nullptr;

    FMatrix ProjectionMatrix;
public:
	virtual UObject* Duplicate() override;

private:
    // --- Projection Properties ---
    bool bIsPerspective = false;


    /*-----------------------------------------------------------------------------
        Decal Fade in/out
     -----------------------------------------------------------------------------*/
public:
    /** @brief Starts only the fade-out process */
    void BeginFade();

    /** @brief Starts only the fade-in process */
    void BeginFadeIn();

    /** @brief Immediately stops any fade in/out process in progress. */
    void StopFade();

    /** @brief Pauses any ongoing fade in or fade out. FadeElapsedTime is preserved. */
    void PauseFade();

    /** @brief Resumes a previously paused fade from where it was paused. */
    void ResumeFade();

    /**
     * @brief Called every frame (or tick) to update the fade state over time.
     * @todo Update should be done inside of Tick method of the component.
     */
    void UpdateFade(float DeltaTime);

    // --- Getters & Setters ---

    float GetFadeStartDelay() const { return FadeStartDelay; }
    
    float GetFadeDuration() const { return FadeDuration; }
    
    float GetFadeInDuration() const { return FadeInDuration; }
    
    float GetFadeInStartDelay() const { return FadeInStartDelay; }
    
    float GetFadeProgress() const { return FadeProgress; }
    
    bool GetDestroyOwnerAfterFade() const { return bDestroyOwnerAfterFade; }

    void SetFadeStartDelay(float InFadeStartDelay) { FadeStartDelay = InFadeStartDelay; }
    
    void SetFadeDuration(float InFadeDuration) { FadeDuration = InFadeDuration; }
    
    void SetFadeInDuration(float InFadeInDuration) { FadeInDuration = InFadeInDuration; }
    
    void SetFadeInStartDelay(float InFadeInStartDelay) { FadeInStartDelay = InFadeInStartDelay; }
    
    void SetDestroyOwnerAfterFade(bool bInDestroyOwnerAfterFade) { bDestroyOwnerAfterFade = bInDestroyOwnerAfterFade; }

private:
    /** @brief Time in seconds to wait before fading out the decal. */
    float FadeStartDelay = 0.0f;

    /** @brief Time in seconds for the decal to fade out. Set the Fade Duration and Start Delay to 0 to make the decal persistent. */
    float FadeDuration = 3.0f;

    /** @brief Time in seconds to wait before fading in the decal. */
    float FadeInStartDelay;
    
    /** @brief Time in seconds for the decal to fade in. */
    float FadeInDuration = 3.0f;

    /** @brief Tracks elapsed time since the fade (in or out) started. */
    float FadeElapsedTime = 0.0f;

    /**
     * @brief Normalized fade progress from 0 to 1.
     * 0 = fully faded in, 1 = fully faded out.
     */
    float FadeProgress = 0.0f;

    /** @brief 	When enabled, the owning actor is automatically destroyed after fading out. */
    bool bDestroyOwnerAfterFade = false;

    /** @brief True if the decal is currently fading out. */
    bool bIsFading = false;

    /** @brief True if the decal is currently fading in. */
    bool bIsFadingIn = false;

    /** @brief True if the fade process is currently paused. */
    bool bIsFadePaused = false;
};
