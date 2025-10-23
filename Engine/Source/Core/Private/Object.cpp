#include "pch.h"
#include "Core/Public/Object.h"
#include "Core/Public/EngineStatics.h"
#include "Core/Public/Name.h"

#include <json.hpp>

uint32 UEngineStatics::NextUUID = 0;

TArray<UObject*>& GetUObjectArray()
{
	static TArray<UObject*> GUObjectArray;
	return GUObjectArray;
}

IMPLEMENT_CLASS_BASE(UObject)

UObject::UObject()
	: Name(FName::GetNone()), Outer(nullptr)
{
	UUID = UEngineStatics::GenUUID();
	
	GetUObjectArray().emplace_back(this);
	InternalIndex = static_cast<uint32>(GetUObjectArray().size()) - 1;
}

UObject::~UObject()
{
	/** @todo: 이후에 리뷰 필요 */

	// std::vector에 맞는 올바른 인덱스 유효성 검사
	if (InternalIndex < GetUObjectArray().size())
	{
		GetUObjectArray()[InternalIndex] = nullptr;
	}
}

void UObject::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
}

UObject* UObject::Duplicate()
{
	UObject* Object = NewObject(GetClass());
	DuplicateSubObjects(Object);
	return Object;
}

void UObject::DuplicateSubObjects(UObject* DuplicatedObject)
{
	
}

void UObject::SetOuter(UObject* InObject)
{
	if (Outer == InObject)
	{
		return;
	}

	// 기존 Outer가 있었다면, 나의 전체 메모리 사용량을 빼달라고 전파
	// 새로운 Outer가 있다면, 나의 전체 메모리 사용량을 더해달라고 전파
	if (Outer)
	{
		Outer->PropagateMemoryChange(-static_cast<int64>(AllocatedBytes), -static_cast<int32>(AllocatedCounts));
	}

	Outer = InObject;

	if (Outer)
	{
		Outer->PropagateMemoryChange(AllocatedBytes, AllocatedCounts);
	}
}

void UObject::AddMemoryUsage(uint64 InBytes, uint32 InCount)
{
	uint64 BytesToAdd = InBytes;

	if (!BytesToAdd)
	{
		BytesToAdd = GetClass()->GetClassSize();
	}

	// 메모리 변경 전파
	PropagateMemoryChange(BytesToAdd, InCount);
}

void UObject::RemoveMemoryUsage(uint64 InBytes, uint32 InCount)
{
	PropagateMemoryChange(-static_cast<int64>(InBytes), -static_cast<int32>(InCount));
}

void UObject::PropagateMemoryChange(uint64 InBytesDelta, uint32 InCountDelta)
{
	// 자신의 값에 변화량을 더함
	AllocatedBytes += InBytesDelta;
	AllocatedCounts += InCountDelta;

	// Outer가 있다면, 동일한 변화량을 그대로 전파
	if (Outer)
	{
		Outer->PropagateMemoryChange(InBytesDelta, InCountDelta);
	}
}

/**
 * @brief 해당 클래스가 현재 내 클래스의 조상 클래스인지 판단하는 함수
 * 내부적으로 재귀를 활용해서 부모를 계속 탐색한 뒤 결과를 반환한다
 * @param InClass 판정할 Class
 * @return 판정 결과
 */
bool UObject::IsA(UClass* InClass) const
{
	if (!InClass)
	{
		return false;
	}

	return GetClass()->IsChildOf(InClass);
}

/**
 * @brief 해당 클래스가 현재 내 클래스와 동일한지 판단하는 함수
 * @return 판정 결과
 */
bool UObject::IsExactly(UClass* InClass) const
{
	if (!InClass)
	{
		return false;
	}

	return GetClass() == InClass;
}