#pragma once
#include "Object.h"
using std::is_base_of_v;

/**
 * @brief 클래스를 지정하여 새 객체를 생성하는 함수
 * @tparam T 생성할 객체의 타입
 * @param InOuter 생성될 객체의 부모
 * @return 생성된 객체
 */
template <typename T>
T* NewObject(UObject* InOuter = nullptr)
{
	static_assert(is_base_of_v<UObject, T>, "생성할 클래스는 UObject를 반드시 상속 받아야 합니다");
	T* NewObject = new T();
	NewObject->SetName(FNameTable::GetInstance().GetUniqueName(NewObject->GetClass()->GetName().ToString()));
	NewObject->SetOuter(InOuter);
	return NewObject;
}

inline UObject* NewObject(UClass* ClassToCreate, UObject* InOuter = nullptr)
{
	if (!ClassToCreate) { return nullptr; }
	UObject* NewObject = ClassToCreate->CreateDefaultObject();
       
	if (NewObject)
	{
		FName NewName = FNameTable::GetInstance().GetUniqueName(ClassToCreate->GetName().ToString());
		NewObject->SetName(NewName);
		NewObject->SetOuter(InOuter);
	}

	return NewObject;
}