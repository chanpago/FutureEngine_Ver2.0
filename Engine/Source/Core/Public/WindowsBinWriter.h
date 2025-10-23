#pragma once

#include <filesystem>
#include <fstream>

#include "Core/Public/Archive.h"
#include "Global/Macro.h"

struct FWindowsBinWriter : public FArchive
{
	virtual ~FWindowsBinWriter()
	{
		if (Stream.is_open())
		{
			Stream.close();
		}
	}

	FWindowsBinWriter(const std::filesystem::path& FilePath)
		: Stream(FilePath, std::ios::binary | std::ios::out)
	{
		if (!Stream)
		{
			UE_LOG_ERROR("쓰기용 파일을 여는데 실패했습니다: %s", FilePath.string());
			assert("쓰기용 파일을 여는데 실패했습니다" && false);
		}
	}

	bool IsLoading() const override { return false; }

	void Serialize(void* V, size_t Length) override
	{
		Stream.write(reinterpret_cast<const char*>(V), Length);
		if (!Stream)
		{
			UE_LOG_ERROR("파일 쓰기를 실패했습니다.");
			assert("파일 쓰기를 실패했습니다." && false);
		}
	}

private:
	std::ofstream Stream;
};
