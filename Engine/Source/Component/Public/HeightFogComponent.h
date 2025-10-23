#pragma once

UCLASS()
class UHeightFogComponent : public USceneComponent
{
    GENERATED_BODY()
    DECLARE_CLASS(UHeightFogComponent, USceneComponent)
public:
    UHeightFogComponent();
    virtual ~UHeightFogComponent() override;

    virtual void TickComponent(float DeltaTime) override;
    virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

    UClass* GetSpecificWidgetClass() const override;

    virtual UObject* Duplicate() override;

    // --- getter/setter --- //

    float GetFogDensity() const { return FogDensity; }
    float GetFogHeightFalloff() const { return FogHeightFalloff; }
    float GetStartDistance() const { return StartDistance; }
    float GetFogCutoffDistance() const { return FogCutoffDistance; }
    float GetFogMaxOpacity() const { return FogMaxOpacity; }
    FVector GetFogInscatteringColor() const { return FogInScatteringColor; }
    bool GetVisible() const { return bVisible; }

    void SetFogDensity(float InValue) { FogDensity = InValue; }
    void SetFogHeightFalloff(float InValue) { FogHeightFalloff = InValue; }
    void SetStartDistance(float InValue) { StartDistance = InValue; }
    void SetFogCutoffDistance(float InValue) { FogCutoffDistance = InValue; }
    void SetFogMaxOpacity(float InValue) { FogMaxOpacity = InValue; }
    void SetFogInscatteringColor(const FVector& InValue) { FogInScatteringColor = InValue; }
    void SetVisible(bool InVisible) { bVisible = InVisible; }
    
protected:
    // 안개의 밀도 
    float FogDensity = 0.05f;
    // 높이에 따라 안개가 얼마나 빠르게 옅어지는지
    float FogHeightFalloff = 0.01f;
    // 카메라로부터 안개가 시작되는 거리
    float StartDistance = 1.5f;
    // 안개 효과 계산을 중단하는 최대 거리
    float FogCutoffDistance = 50000.f;
    // 안개가 최대로 짙어졌을때 최대 불투명도
    float FogMaxOpacity = 0.98f;

    // 산란되어 들어오는 빛의 색상
    FVector FogInScatteringColor = {0.5f, 0.5f, 0.5f};
    
    // 안개 표시 여부
    bool bVisible = true;
};
