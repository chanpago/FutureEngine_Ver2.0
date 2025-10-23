#pragma once

#define DT UTimeManager::GetInstance().GetDeltaTime()

// UE_LOG Macro 시스템
// 기본 UE_LOG (Info 타입)
#define UE_LOG(fmt, ...) \
    do { \
        printf(fmt "\n", ##__VA_ARGS__); \
        try { \
            UConsoleWindow::GetInstance().AddLog("" fmt, ##__VA_ARGS__); \
        } catch(...) {} \
    } while(0)

// 로그 타입별 매크로들
#define UE_LOG_INFO(fmt, ...) \
    do { \
        printf("[INFO] " fmt "\n", ##__VA_ARGS__); \
        try { \
            UConsoleWindow::GetInstance().AddLog(ELogType::Info, "" fmt, ##__VA_ARGS__); \
        } catch(...) {} \
    } while(0)

#define UE_LOG_WARNING(fmt, ...) \
    do { \
        printf("[WARNING] " fmt "\n", ##__VA_ARGS__); \
        try { \
            UConsoleWindow::GetInstance().AddLog(ELogType::Warning, "" fmt, ##__VA_ARGS__); \
        } catch(...) {} \
    } while(0)

#define UE_LOG_ERROR(fmt, ...) \
    do { \
        printf("[ERROR] " fmt "\n", ##__VA_ARGS__); \
        try { \
            UConsoleWindow::GetInstance().AddLog(ELogType::Error, "" fmt, ##__VA_ARGS__); \
        } catch(...) {} \
    } while(0)

#define UE_LOG_SUCCESS(fmt, ...) \
    do { \
        printf("[SUCCESS] " fmt "\n", ##__VA_ARGS__); \
        try { \
            UConsoleWindow::GetInstance().AddLog(ELogType::Success, "" fmt, ##__VA_ARGS__); \
        } catch(...) {} \
    } while(0)

#define UE_LOG_SYSTEM(fmt, ...) \
    do { \
        printf("[SYSTEM] " fmt "\n", ##__VA_ARGS__); \
        try { \
            UConsoleWindow::GetInstance().AddLog(ELogType::System, "" fmt, ##__VA_ARGS__); \
        } catch(...) {} \
    } while(0)

#define UE_LOG_DEBUG(fmt, ...) \
    do { \
        printf("[DEBUG] " fmt "\n", ##__VA_ARGS__); \
        try { \
            UConsoleWindow::GetInstance().AddLog(ELogType::Debug, "" fmt, ##__VA_ARGS__); \
        } catch(...) {} \
    } while(0)

#define UE_LOG_COMMAND(fmt, ...) \
    do { \
        printf("[CMD] " fmt "\n", ##__VA_ARGS__); \
        try { \
            UConsoleWindow::GetInstance().AddLog(ELogType::Command, "" fmt, ##__VA_ARGS__); \
        } catch(...) {} \
    } while(0)

#define UE_LOG_TERMINAL(fmt, ...) \
    do { \
        printf("[TERMINAL] " fmt "\n", ##__VA_ARGS__); \
        try { \
            UConsoleWindow::GetInstance().AddLog(ELogType::Terminal, "" fmt, ##__VA_ARGS__); \
        } catch(...) {} \
    } while(0)

#define UE_LOG_TERMINAL_ERROR(fmt, ...) \
    do { \
        printf("[TERMINAL_ERROR] " fmt "\n", ##__VA_ARGS__); \
        try { \
            UConsoleWindow::GetInstance().AddLog(ELogType::TerminalError, "" fmt, ##__VA_ARGS__); \
        } catch(...) {} \
    } while(0)


/**
 * @brief UENUM 매크로 시스템
 * UClass 시스템과 완전히 독립적인 enum reflection 매크로들
 *
 * 사용 예시:
 * UENUM()
 * enum class EMyEnum : uint8
 * {
 *     Value1,
 *     Value2,
 *     Value3
 * };
 * DECLARE_ENUM_REFLECTION(EMyEnum)
 */

/**
 * @brief UENUM() 매크로
 * 언리얼 엔진의 UCLASS()와 유사한 외관적 통일성을 위한 매크로
 * 실제로는 아무 기능도 하지 않지만, 코드의 가독성과 일관성을 위해 사용
 */
#define UENUM()


/**
 * @brief Enum reflection 기능을 활성화하는 매크로
 * enum 정의 후에 호출하여 RTTI 기능을 활성화
 *
 * @param EnumType enum class 타입명
 */
#define DECLARE_ENUM_REFLECTION(EnumType) \
    template<> \
    struct EnumReflection::EnumRange<EnumType> \
    { \
        static constexpr int Min = 0; \
        static constexpr int Max = 255; \
    }; \
    using EnumType##_Reflector = TEnumReflector<EnumType>;

/**
 * @brief 커스텀 범위를 가진 enum reflection 선언 매크로
 * enum 값의 범위가 기본값(-128~128)과 다를 때 사용
 *
 * @param EnumType enum class 타입명
 * @param MinValue 최소값
 * @param MaxValue 최대값
 */
#define DECLARE_ENUM_REFLECTION_RANGE(EnumType, MinValue, MaxValue) \
    template<> \
    struct EnumReflection::EnumRange<EnumType> \
    { \
        static constexpr int Min = MinValue; \
        static constexpr int Max = MaxValue; \
    }; \
    using EnumType##_Reflector = TEnumReflector<EnumType>;

/**
 * @brief uint8 기반 enum을 위한 reflection 매크로
 * 컴파일타임에 __FUNCSIG__ 파싱으로 자동 enum 값 감지
 * 런타임 메모리 할당 완전 제거
 *
 * @param EnumType enum class 타입명 (ex: EKeyInput)
 */
#define DECLARE_UINT8_ENUM_REFLECTION(EnumType) \
    template<> \
    struct EnumReflection::EnumRange<EnumType> \
    { \
        static constexpr int Min = 0; \
        static constexpr int Max = 255; \
    }; \
    using EnumType##_Reflector = TEnumReflector<EnumType>;

/**
 * @brief Flag enum (비트마스크)를 위한 reflection 매크로
 * 비트 플래그로 사용되는 enum을 위한 특별한 처리
 *
 * @param EnumType enum class 타입명
 */
#define DECLARE_FLAGS_ENUM_REFLECTION(EnumType) \
    template<> \
    struct EnumReflection::EnumRange<EnumType> \
    { \
        static constexpr int Min = 0; \
        static constexpr int Max = 64; /* 2^6 까지 커버 */ \
    }; \
    using EnumType##_Reflector = TEnumReflector<EnumType>; \
    \
    constexpr EnumType operator|(EnumType lhs, EnumType rhs) noexcept \
    { \
        using UnderlyingType = std::underlying_type_t<EnumType>; \
        return static_cast<EnumType>(static_cast<UnderlyingType>(lhs) | static_cast<UnderlyingType>(rhs)); \
    } \
    \
    constexpr EnumType operator&(EnumType lhs, EnumType rhs) noexcept \
    { \
        using UnderlyingType = std::underlying_type_t<EnumType>; \
        return static_cast<EnumType>(static_cast<UnderlyingType>(lhs) & static_cast<UnderlyingType>(rhs)); \
    } \
    \
    constexpr EnumType operator^(EnumType lhs, EnumType rhs) noexcept \
    { \
        using UnderlyingType = std::underlying_type_t<EnumType>; \
        return static_cast<EnumType>(static_cast<UnderlyingType>(lhs) ^ static_cast<UnderlyingType>(rhs)); \
    } \
    \
    constexpr EnumType operator~(EnumType value) noexcept \
    { \
        using UnderlyingType = std::underlying_type_t<EnumType>; \
        return static_cast<EnumType>(~static_cast<UnderlyingType>(value)); \
    }

/**
 * @brief Enum 관련 유틸리티 매크로들
 */

// enum 값의 개수를 컴파일 타임에 얻는 매크로
#define ENUM_COUNT(EnumType) TEnumReflector<EnumType>::GetCount()

// enum 값을 문자열로 변환하는 매크로
#define ENUM_TO_STRING(EnumValue) EnumToString(EnumValue)

// 문자열을 enum 값으로 변환하는 매크로
#define STRING_TO_ENUM(EnumType, StringValue) StringToEnum<EnumType>(StringValue)

// enum 값이 유효한지 확인하는 매크로
#define IS_VALID_ENUM(EnumValue) IsValidEnumValue(EnumValue)

/**
 * @brief 디버깅을 위한 매크로들
 */
#define ENUM_DEBUG_PRINT(EnumType) \
    do { \
        UE_LOG("=== %s Enum Values ===", #EnumType); \
        std::size_t ValidCount = 0; \
        for (int EnumIndex = EnumReflection::EnumRange<EnumType>::Min; EnumIndex <= EnumReflection::EnumRange<EnumType>::Max; ++EnumIndex) { \
            auto EnumValue = static_cast<EnumType>(EnumIndex); \
            const char* EnumName = TEnumReflector<EnumType>::ToString(EnumValue); \
            if (EnumName && strcmp(EnumName, "Unknown") != 0) { \
                UE_LOG("[%zu] %s = %d", ValidCount, EnumName, static_cast<int>(EnumValue)); \
                ++ValidCount; \
            } \
        } \
        UE_LOG("Total: %zu values", ValidCount); \
    } while(0)

/**
 * @brief 컴파일타임 enum iteration을 위한 헬퍼 (새 reflection 시스템 버전)
 */
#define ENUM_FOR_EACH(EnumType, VarName) \
    for (int _EnumIndex = EnumReflection::EnumRange<EnumType>::Min; _EnumIndex <= EnumReflection::EnumRange<EnumType>::Max; ++_EnumIndex) \
        if (auto VarName = static_cast<EnumType>(_EnumIndex); \
            TEnumReflector<EnumType>::IsValid(VarName))

/**
 * @brief enum 값과 이름을 함께 iterate하는 매크로 (새 reflection 시스템 버전)
 */
#define ENUM_FOR_EACH_WITH_NAME(EnumType, ValueVar, NameVar) \
    for (int _EnumIndex = EnumReflection::EnumRange<EnumType>::Min; _EnumIndex <= EnumReflection::EnumRange<EnumType>::Max; ++_EnumIndex) \
        if (auto ValueVar = static_cast<EnumType>(_EnumIndex); \
            TEnumReflector<EnumType>::IsValid(ValueVar)) \
            if (const char* NameVar = TEnumReflector<EnumType>::ToString(ValueVar); \
                NameVar && strcmp(NameVar, "Unknown") != 0)

/**
 * @brief 컴파일타임 enum 검증을 위한 매크로 (새 reflection 시스템 버전)
 */
#define STATIC_ASSERT_ENUM_VALID(EnumValue) \
    static_assert(TEnumReflector<decltype(EnumValue)>::IsValid(EnumValue), \
                  "Invalid enum value: " #EnumValue)

#define STATIC_ASSERT_ENUM_COUNT(EnumType, ExpectedCount) \
    static_assert(TEnumReflector<EnumType>::GetCount() == ExpectedCount, \
                  "Enum count mismatch for " #EnumType)

/**
 * @brief enum 값의 범위를 컴파일타임에 검증하는 매크로
 */
#define STATIC_ASSERT_ENUM_RANGE(EnumType, MinVal, MaxVal) \
    static_assert(EnumReflection::EnumRange<EnumType>::Min == MinVal && \
                  EnumReflection::EnumRange<EnumType>::Max == MaxVal, \
                  "Enum range mismatch for " #EnumType)

/**
 * @brief enum ToString 결과를 컴파일타임에 검증하는 매크로
 */
#define STATIC_ASSERT_ENUM_NAME(EnumValue, ExpectedName) \
    static_assert([]() constexpr { \
        const char* ActualName = TEnumReflector<decltype(EnumValue)>::ToString(EnumValue); \
        return ActualName && strcmp(ActualName, ExpectedName) == 0; \
    }(), "Enum name mismatch for " #EnumValue)

/**
 * @brief 컴파일타임 enum 유틸리티 매크로들
 */

// enum 값이 유효한 범위 내에 있는지 검사
#define ENUM_IN_RANGE(EnumValue) \
    (static_cast<int>(EnumValue) >= EnumReflection::EnumRange<decltype(EnumValue)>::Min && \
     static_cast<int>(EnumValue) <= EnumReflection::EnumRange<decltype(EnumValue)>::Max)

// enum 값을 안전하게 반복 (유효한 값만 처리)
#define ENUM_SAFE_FOREACH(EnumType, VarName, Block) \
    ENUM_FOR_EACH(EnumType, VarName) { Block }

// enum 이름과 값을 로그로 출력
#define ENUM_LOG_VALUE(EnumValue) \
    UE_LOG("%s::%s = %d", #EnumValue, \
           TEnumReflector<decltype(EnumValue)>::ToString(EnumValue), \
           static_cast<int>(EnumValue))

// enum 배열에서 이름으로 검색
#define ENUM_FIND_BY_NAME(EnumType, Name) \
    TEnumReflector<EnumType>::FromString(Name)

// enum 이름 비교 (대소문자 구분 안함)
#define ENUM_NAME_EQUALS(EnumValue, ExpectedName) \
    (strcmp(TEnumReflector<decltype(EnumValue)>::ToString(EnumValue), ExpectedName) == 0)

// 여러 enum 값들을 한 번에 출력
#define ENUM_LOG_MULTIPLE(...) \
    do { \
        UE_LOG("=== Multiple Enum Values ==="); \
        ([&]{ ENUM_LOG_VALUE(__VA_ARGS__); }(), ...); \
    } while(0)
