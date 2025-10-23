#include "pch.h"
#include "Manager/Config/Public/ConfigManager.h"
#include "Core/Public/Class.h"
#include "Editor/Public/Camera.h"
#include "Utility/Public/JsonSerializer.h"

#include <json.hpp>

IMPLEMENT_SINGLETON_CLASS(UConfigManager, UObject)

UConfigManager::UConfigManager()
	: EditorIniFileName("editor.ini")
{
	LoadEditorSetting();
}

UConfigManager::~UConfigManager()
{
	SaveEditorSetting();
}

void UConfigManager::LoadEditorSetting()
{
	const FString& FileNameStr = EditorIniFileName.ToString();
	std::ifstream Ifs(FileNameStr);
	if (!Ifs.is_open())
	{
		CellSize = 1.0f;
		CameraSensitivity = UCamera::DEFAULT_SPEED;
		return; // 파일이 없으면 기본값 유지
	}

	FString Line;
	while (std::getline(Ifs, Line))
	{
		size_t DelimiterPos = Line.find('=');
		if (DelimiterPos == FString::npos) continue;

		FString Key = Line.substr(0, DelimiterPos);
		FString Value = Line.substr(DelimiterPos + 1);

		try
		{
			if (Key == "CellSize") CellSize = std::stof(Value);
			else if (Key == "CameraSensitivity") CameraSensitivity = std::stof(Value);
			else if (Key == "RootSplitterRatio") RootSplitterRatio = std::stof(Value);
			else if (Key == "LeftSplitterRatio") LeftSplitterRatio = std::stof(Value);
			else if (Key == "RightSplitterRatio") RightSplitterRatio = std::stof(Value);
			else if (Key == "LastUsedLevelPath") LastUsedLevelPath = Value;
		}
		catch (const std::exception&) {}
	}
}

void UConfigManager::SaveEditorSetting()
{
	std::ofstream Ofs(EditorIniFileName.ToString());
	if (Ofs.is_open())
	{
		Ofs << "CellSize=" << CellSize << "\n";
		Ofs << "CameraSensitivity=" << CameraSensitivity << "\n";
		Ofs << "RootSplitterRatio=" << RootSplitterRatio << "\n";
		Ofs << "LeftSplitterRatio=" << LeftSplitterRatio << "\n";
		Ofs << "RightSplitterRatio=" << RightSplitterRatio << "\n";
		Ofs << "LastUsedLevelPath=" << LastUsedLevelPath << "\n";
	}
}

// FutureEngine 철학: 카메라 설정은 ViewportManager를 통해 각 ViewportClient의 Camera에서 관리
// ConfigManager는 더 이상 카메라 설정을 직접 관리하지 않음
JSON UConfigManager::GetCameraSettingsAsJson()
{
	// 빈 배열 반환 (하위 호환성을 위해 함수는 유지)
	return json::Array();
}

void UConfigManager::SetCameraSettingsFromJson(const JSON& InData)
{
	// 카메라 설정은 ViewportManager를 통해 처리되어야 함
	// 이 함수는 하위 호환성을 위해 유지되지만 아무 작업도 수행하지 않음
}
