#pragma once

#include <filesystem>
#include <fstream>

#include "Core/Public/Archive.h"
#include "Global/Macro.h"

struct FWindowsBinReader : public FArchive
{
	virtual ~FWindowsBinReader()
	{
		if (Stream.is_open())
		{
			Stream.close();
		}
	}

	FWindowsBinReader(const std::filesystem::path& FilePath)
		: Stream(FilePath, std::ios::binary | std::ios::in)
	{
		if (!Stream)
		{
			UE_LOG_ERROR("읽기용 파일을 여는데 실패했습니다: %s", FilePath.string());
			//assert("읽기용 파일을 여는데 실패했습니다" && false);
		}
	}

	bool IsLoading() const override { return true; }

	void Serialize(void* V, size_t Length) override
	{
		Stream.read(reinterpret_cast<char*>(V), Length);
		if (!Stream)
		{
			UE_LOG_ERROR("파일 읽기를 실패했습니다.");
			//assert("파일 읽기를 실패했습니다." && false);
		}
	}

private:
	std::ifstream Stream;
};

