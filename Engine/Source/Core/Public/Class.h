#pragma once
#include "Name.h"

class UObject;
/**
 * @brief UClass Metadata System
 * Runtime에 컴파일 시에 다양한 클래스 정보를 제공하기 위해 만들어진 클래스
 * @param ClassName 클래스 이름
 * @param SuperClass 부모 클래스
 * @param ClassSize 클래스 크기
 * @param Constructor 생성자 함수 포인터
 */
class UClass
{
public:
    // 생성자 함수 포인터 타입 정의
    typedef UObject* (*ClassConstructorType)();
public:
    static void SignUpClass(UClass* InClass);
    static UClass* FindClass(const FName& InClassName);
    static TArray<UClass*> FindClasses(UClass* SuperClass);
private:
    static TArray<UClass*>& GetAllClasses();
    
public:
    UClass(const FName& InName, UClass* InSuperClass, size_t InClassSize, ClassConstructorType InConstructor, bool InIsAbstract = false);

    // Getter
    const FName& GetName() const { return ClassName; }
    UClass* GetSuperClass() const { return SuperClass; }
    size_t GetClassSize() const { return ClassSize; }
    
    bool IsChildOf(UClass* InClass) const;
    UObject* CreateDefaultObject() const;

    bool IsAbstract() const { return bIsAbstract; }

private:
    FName ClassName;
    UClass* SuperClass;
    size_t ClassSize;
    ClassConstructorType Constructor;
    bool bIsAbstract;
};

/**
 * @brief RTTI 매크로 시스템
 *
 * 언리얼엔진의 UCLASS(), DECLARE_CLASS, IMPLEMENT_CLASS와 유사한 매크로들 정의
 */
 // UCLASS 매크로, 기능은 존재하지 않으며 외관 상의 유사성을 목표로 세팅
#define UCLASS()

// Friend Class 처리를 위한 Generated Body 매크로
#define GENERATED_BODY() \
public: \
    friend class UClass; \
    friend class UObject;

// 클래스 선언부에 사용하는 매크로
#define DECLARE_CLASS(ClassName, SuperClassName) \
public: \
    typedef ClassName ThisClass; \
    typedef SuperClassName Super; \
    static UClass* StaticClass(); \
    virtual UClass* GetClass() const; \
    static UObject* CreateDefaultObject##ClassName();

// 클래스 구현부에 사용하는 매크로
#define IMPLEMENT_CLASS(ClassName, SuperClassName) \
UClass* ClassName::StaticClass() \
{ \
/* 정적 지역 변수를 사용하여 UClass 객체를 자동 관리 */ \
    static UClass Instance( \
        FString(#ClassName), \
        SuperClassName::StaticClass(), \
        sizeof(ClassName), \
        &ClassName::CreateDefaultObject##ClassName \
    ); \
    UClass::SignUpClass(&Instance); \
    return &Instance; \
} \
UClass* ClassName::GetClass() const \
    { \
        return ClassName::StaticClass(); \
    } \
UObject* ClassName::CreateDefaultObject##ClassName() \
    { \
        return new ClassName(); \
    } \
static bool bIsRegistered_##ClassName = [](){ ClassName::StaticClass(); return true; }();

// 추상 클래스에 사용하는 매크로 (기본 객체를 생성하지 않음)
#define IMPLEMENT_ABSTRACT_CLASS(ClassName, SuperClassName) \
UClass* ClassName::StaticClass() \
{ \
    static UClass Instance( \
        FString(#ClassName), \
        SuperClassName::StaticClass(), \
        sizeof(ClassName), \
        nullptr, \
        true \
    ); \
    UClass::SignUpClass(&Instance); \
    return &Instance; \
} \
UClass* ClassName::GetClass() const \
{ \
    return ClassName::StaticClass(); \
}\
static bool bIsRegistered_##ClassName = [](){ ClassName::StaticClass(); return true; }();

/**
 * @brief 싱글톤 클래스용 RTTI 매크로 시스템
 */

 // 싱글톤 클래스 선언부에 사용하는 매크로
#define DECLARE_SINGLETON_CLASS(ClassName, SuperClassName) \
public: \
    typedef ClassName ThisClass; \
    typedef SuperClassName Super; \
    static UClass* StaticClass(); \
    virtual UClass* GetClass() const; \
    static UObject* CreateDefaultObject##ClassName(); \
    static ClassName& GetInstance(); \
private: \
    ClassName(); \
    virtual ~ClassName(); \
    ClassName(const ClassName&) = delete; \
    ClassName& operator=(const ClassName&) = delete; \
    ClassName(ClassName&&) = delete; \
    ClassName& operator=(ClassName&&) = delete;

// 싱글톤 클래스 구현부에 사용하는 매크로
#define IMPLEMENT_SINGLETON_CLASS(ClassName, SuperClassName) \
UClass* ClassName::StaticClass() \
{ \
    /* 정적 지역 변수를 사용하여 UClass 객체를 자동 관리 */ \
    static UClass Instance( \
        FString(#ClassName), \
        SuperClassName::StaticClass(), \
        sizeof(ClassName), \
        nullptr /* 싱글톤은 동적 생성을 지원하지 않으므로 생성자 포인터를 null로 전달 */ \
    ); \
    UClass::SignUpClass(&Instance); \
    return &Instance; \
} \
UClass* ClassName::GetClass() const \
{ \
    return ClassName::StaticClass(); \
} \
UObject* ClassName::CreateDefaultObject##ClassName() \
{ \
    /* 싱글톤 인스턴스의 주소를 UObject* 타입으로 반환합니다. */ \
    return &ClassName::GetInstance(); \
} \
ClassName& ClassName::GetInstance() \
{ \
    static ClassName Instance; \
    return Instance; \
}\
static bool bIsRegistered_##ClassName = [](){ ClassName::StaticClass(); return true; }();

// UObject의 기본 매크로 (Base Class)
#define IMPLEMENT_CLASS_BASE(ClassName) \
UClass* ClassName::StaticClass() \
{ \
    /* 정적 지역 변수를 사용하여 UClass 객체를 자동 관리 (기반 클래스 버전) */ \
    static UClass Instance( \
        FString(#ClassName), \
        nullptr, \
        sizeof(ClassName), \
        &ClassName::CreateDefaultObject##ClassName \
    ); \
    UClass::SignUpClass(&Instance); \
    return &Instance; \
} \
UClass* ClassName::GetClass() const \
{ \
    return ClassName::StaticClass(); \
} \
UObject* ClassName::CreateDefaultObject##ClassName() \
{ \
    return new ClassName(); \
}\
static bool bIsRegistered_##ClassName = [](){ ClassName::StaticClass(); return true; }();