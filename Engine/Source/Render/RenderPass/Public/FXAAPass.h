#pragma once
#include "Render/RenderPass/Public/RenderPass.h"


class UDeviceResources;

struct alignas(16) FFXAAConstants
{
    FVector2 InvResolution = FVector2();
    float FXAASpanMax = 16.0f;
    float FXAAReduceMul = 1.0f / 16.0f;
    float FXAAReduceMin = 1.0f / 256.0f;
    float Padding = 0.0f;
};

class FFXAAPass : public FRenderPass
{
public:
	/**
	 * @brief FXAAPass 클래스의 생성자입니다.
	 * @param InPipeline 파이프라인 객체입니다.
	 * @param InDeviceResources 디바이스 리소스 객체입니다.
	 * @param InVS 정점 셰이더입니다.
	 * @param InPS 픽셀 셰이더입니다.
	 * @param InLayout 입력 레이아웃입니다.
	 * @param InSampler 샘플러 상태입니다.
	 */
	FFXAAPass(UPipeline* InPipeline,
	  UDeviceResources* InDeviceResources,
	  ID3D11VertexShader* InVS,
	  ID3D11PixelShader* InPS,
	  ID3D11InputLayout* InLayout,
	  ID3D11SamplerState* InSampler);

	/**
	 * @brief FXAAPass 클래스의 소멸자입니다.
	 */
	~FFXAAPass();

	/**
	 * @brief FXAA 렌더링 패스를 실행합니다.
	 * @param Context 렌더링 컨텍스트입니다.
	 */
	void Execute(FRenderingContext& Context) override;

	/**
	 * @brief FXAAPass에서 사용된 리소스를 해제합니다.
	 */
	void Release() override;

	void SetVertexShader(ID3D11VertexShader* InVS) { VertexShader = InVS; }
	void SetPixelShader(ID3D11PixelShader* InPS) { PixelShader = InPS; }
	void SetInputLayout(ID3D11InputLayout* InLayout) { InputLayout = InLayout; }
	void SetSamplerState(ID3D11SamplerState* InSampler) { SamplerState = InSampler; }

private:
	/**
	 * @brief 전체 화면 사각형을 초기화합니다.
	 */
	void InitializeFullscreenQuad();

	/**
	 * @brief FXAA에 사용되는 상수 버퍼를 업데이트합니다.
	 */
	void UpdateConstants();

	/**
	 * @brief 렌더 타겟을 설정합니다.
	 */
	void SetRenderTargets();
    
private:
    UDeviceResources* DeviceResources = nullptr;

    ID3D11VertexShader* VertexShader = nullptr;
    ID3D11PixelShader* PixelShader = nullptr;
    ID3D11InputLayout* InputLayout = nullptr;
    ID3D11SamplerState* SamplerState = nullptr;

    ID3D11Buffer* FullscreenVB = nullptr;
    ID3D11Buffer* FullscreenIB = nullptr;
    UINT FullscreenStride = 0;
    UINT FullscreenIndexCount = 0;

    ID3D11Buffer* FXAAConstantBuffer = nullptr;
    FFXAAConstants FXAAParams{};
};
