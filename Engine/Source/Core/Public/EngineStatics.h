#pragma once

class UEngineStatics
{
public:
	static uint32 GenUUID()
	{
		return NextUUID++;
	}

private:
	static uint32 NextUUID;
};
