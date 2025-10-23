#pragma once
#include "Global/Types.h"
#include "Core/Public/Object.h"
#include "Core/Public/Name.h"


namespace json { class JSON; }
using JSON = json::JSON;

/**
 * @brief 뷰포트 클라이언트의 카메라 정보
 * @param Location 위치
 * @param Rotation 회전
 * @param FocusLotation 직교 투영 카메라가 바라볼 위치
 * @param FovY 원근 투영 시야각
 * @param FarClip  Far Z 값
 * @param NearClip Near Z 값
 * @param OrthoWidth 직교 투영 시야각
 * @param CameraType 카메라 모드
 */
/**
* @brief 현재 주석처리된 FViewportCameraData는 Week04 기준 코드입니다.
*/
//struct FViewportCameraData
//{
//	FVector Location{};
//	FVector Rotation{};
//	FVector FocusLocation{};
//	float FovY{};
//	float FarClip{};
//	float NearClip{};
//	float OrthoWidth{};
//	EViewportCameraType ViewportCameraType = EViewportCameraType::Perspective;
//};

/**
* @brief 해당 FViewportCameraData는 Week05 전용으로 설정된 구조체입니다. [PYB]
*/
//struct FViewportCameraData
//{
//	FVector Location{};
//	FVector Rotation{};
//	float FovY{};
//	float FarClip{};
//	float NearClip{};
//	EViewportCameraType ViewportCameraType = EViewportCameraType::Perspective;
//};

UCLASS()
class UConfigManager : public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UConfigManager, UObject)

public:
	void SaveEditorSetting();
	void LoadEditorSetting();

	// FutureEngine 철학: 카메라 설정은 ViewportManager가 관리
	// 하위 호환성을 위해 함수 시그니처만 유지
	JSON GetCameraSettingsAsJson();
	void SetCameraSettingsFromJson(const JSON& InData);

	float GetCellSize() const
	{
		return CellSize;
	}

	float GetCameraSensitivity() const
	{
		return CameraSensitivity;
	}

	TArray<float> GetSplitterRatio() const
	{
		return TArray<float>({ RootSplitterRatio, LeftSplitterRatio, RightSplitterRatio });
	}

	// 주석 처리된 SetViewportCameraData는 Week04 기준으로 작성된 코드입니다.
	//const FViewportCameraData& GetViewportCameraData(int InIndex) const { return ViewportCameraSettings[InIndex]; }

	// 현재 이 코드는 Week05 전용으로 사용되는 코드입니다.
	// const FViewportCameraData& GetViewportCameraData() const { return ViewportCameraSettings[0]; }

	void SetCellSize(const float cellSize)
	{
		CellSize = cellSize;
	}

	void SetCameraSensitivity(const float cameraSensitivity)
	{
		CameraSensitivity = cameraSensitivity;
	}

	void SetSplitterRatio(const float RootRatio, const float LeftRatio, const float RightRatio)
	{
		RootSplitterRatio = RootRatio;
		LeftSplitterRatio = LeftRatio;
		RightSplitterRatio = RightRatio;
	}

	FString GetLastSavedLevelPath()
	{
		return LastUsedLevelPath;
	}

	void SetLastUsedLevelPath(FString InLevelPath)
	{
		LastUsedLevelPath = InLevelPath;
	}

	//void SetViewportCameraData(int InIndex, const FViewportCameraData& InData) { ViewportCameraSettings[InIndex] = InData; }

private:
	// ini 파일에 저장
	FName EditorIniFileName;
	float CellSize;
	float CameraSensitivity;
	float RootSplitterRatio;
	float LeftSplitterRatio;
	float RightSplitterRatio;
	FString LastUsedLevelPath;

	// Json으로 Level에 같이 저장
	//FViewportCameraData ViewportCameraSettings[4];
};
