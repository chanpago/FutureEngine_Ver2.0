#include "pch.h"
#include "Global/Memory.h"

#include <new>

using std::align_val_t;

// XXX(KHJ): Atomic 처리할 필요가 있을까?
uint32 TotalAllocationBytes = 0;
uint32 TotalAllocationCount = 0;

/**
 * @brief 전역 메모리 관리를 위한 메모리 할당자 오버로딩 함수
 * @param InSize 할당 size
 * @return 할당한 공간에서 할당 공간 정보를 저장한 헤더를 제외한 나머지 공간의 첫 메모리 주소
 */
void* operator new(size_t InSize)
{
	++TotalAllocationCount;
	TotalAllocationBytes += static_cast<uint32>(InSize);

	// Debug Print
	// printf("New: Size=%zu, TotalBytes=%u, TotalCount=%u\n",
	//        InSize, TotalAllocationBytes, TotalAllocationCount);

	AllocHeader* MemoryHeader = static_cast<AllocHeader*>(malloc(sizeof(AllocHeader) + InSize));
	MemoryHeader->size = InSize;
	MemoryHeader->bIsAligned = false;

	return MemoryHeader + 1;
}

/**
 * @brief 오버로드된 함수로 생성 처리한 메모리 공간을 할당 해제하는 함수
 * @param InMemory 처음에 객체 할당용으로 제공된 메모리 주소
 */
void operator delete(void* InMemory) noexcept
{
	if (!InMemory)
	{
		return;
	}

	AllocHeader* MemoryHeader = static_cast<AllocHeader*>(InMemory) - 1;
	size_t MemoryAllocSize = MemoryHeader->size;

	// Debug Print
	// printf("Delete: Size=%zu, TotalBytes=%u, TotalCount=%u\n",
	//        MemoryAllocSize, TotalAllocationBytes, TotalAllocationCount);

	if (TotalAllocationCount > 0)
	{
		--TotalAllocationCount;
	}
	else
	{
		assert(!u8"allocation 처리한 객체보다 더 많은 수를 해제할 수 없음");
	}

	if (TotalAllocationBytes >= MemoryAllocSize)
	{
		TotalAllocationBytes -= static_cast<uint32>(MemoryAllocSize);
	}
	else
	{
		assert(!u8"allocation 처리한 메모리보다 더 많은 양의 메모리를 해제할 수 없음");
	}

	if (MemoryHeader->bIsAligned)
	{
		_aligned_free(MemoryHeader);
	}
	else
	{
		free(MemoryHeader);
	}
}

/**
 * 배열에 대한 메모리 할당자 오버로딩 함수
 * @param InSize 할당 size
 * 세부 처리는 new 단일 객체 처리로 진행된다
 */
void* operator new[](size_t InSize)
{
	return ::operator new(InSize);
}

/**
 * 배열에 대한 메모리 할당 해제 오버로딩 함수
 * @param InMemory 제공된 메모리 주소
 * 세부 처리는 delete 단일 객체 처리로 진행된다
 */
void operator delete[](void* InMemory) noexcept
{
	::operator delete(InMemory);
}

// C++17에서 추가로 제공된 Align된 메모리에 대한 오버로딩 함수
// SIMD 타입이 추후 필요한 것으로 보고 미리 구현해 둠

void* operator new(size_t InSize, align_val_t InAlignment)
{
	size_t Alignment = static_cast<size_t>(InAlignment);

	++TotalAllocationCount;
	TotalAllocationBytes += static_cast<uint32>(InSize);

	// XXX(KHJ): 헤더 크기도 정렬에 맞춰 패딩을 고려해야 할 수 있음
	size_t TotalSize = sizeof(AllocHeader) + InSize;

	// aligned_alloc 사용
	// 크기는 정렬값의 배수로 처리해야 함
	size_t AlignedTotalSize = (TotalSize + Alignment - 1) & ~(Alignment - 1);

#ifdef _MSC_VER
	AllocHeader* MemoryHeader = static_cast<AllocHeader*>(_aligned_malloc(AlignedTotalSize, Alignment));
#else
	AllocHeader* MemoryHeader = static_cast<AllocHeader*>(aligned_malloc(Alignment, AlignedTotalSize));
#endif

	// 실제 할당된 크기를 저장
	MemoryHeader->size = InSize;
	MemoryHeader->bIsAligned = true;

	return MemoryHeader + 1;
}

void operator delete(void* InMemory, align_val_t InAlignment) noexcept
{
	::operator delete(InMemory);
}
