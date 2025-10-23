#pragma once
#include "Types.h"

// 있을 수도, 없을 수도 있는 상황을 표현하기 위한 기능
using std::nullopt;

// enum의 타입을 알 수 있는 기능
using std::underlying_type_t;

using std::string_view;
using std::index_sequence;
using std::make_index_sequence;

using std::char_traits;

/**
 * @brief Compile time enum reflection
 * __FUNCSIG__를 파싱하여 컴파일 타임에 enum 정보를 생성
 */
namespace EnumReflection
{
	class ConstexprStringView
	{
	public:
		const char* Data;
		size_t Size;

		constexpr ConstexprStringView() noexcept : Data(nullptr), Size(0)
		{
		}

		constexpr ConstexprStringView(const char* InString, size_t InLength) noexcept : Data(InString),
			Size(InLength)
		{
		}

		constexpr ConstexprStringView(const char* InString) noexcept : Data(InString),
		                                                               Size(InString
			                                                                    ? char_traits<char>::length(InString)
			                                                                    : 0)
		{
		}

		constexpr const char* data() const noexcept { return Data; }
		constexpr size_t size() const noexcept { return Size; }
		constexpr bool empty() const noexcept { return Size == 0; }
		constexpr char operator[](size_t InPosition) const noexcept { return Data[InPosition]; }

		constexpr ConstexprStringView substr(size_t InPosition, size_t InLength = string_view::npos) const noexcept
		{
			if (InPosition > Size)
			{
				return {};
			}
			InLength = std::min(InLength, Size - InPosition);

			return {Data + InPosition, InLength};
		}

		constexpr size_t find_last_of(char InChar) const noexcept
		{
			if (empty())
			{
				return string_view::npos;
			}

			for (size_t i = Size; i > 0; --i)
			{
				if (Data[i - 1] == InChar)
				{
					return i - 1;
				}
			}

			return string_view::npos;
		}

		constexpr size_t find(char InChar, size_t InPosition = 0) const noexcept
		{
			for (size_t i = InPosition; i < Size; ++i)
			{
				if (Data[i] == InChar)
				{
					return i;
				}
			}
			return string_view::npos;
		}

		constexpr bool starts_with(char InChar) const noexcept
		{
			return !empty() && Data[0] == InChar;
		}

		constexpr bool contains(char InChar) const noexcept
		{
			return find(InChar) != string_view::npos;
		}

		constexpr ConstexprStringView trim() const noexcept
		{
			size_t Start = 0;
			size_t End = Size;

			// 앞쪽 공백 제거
			while (Start < End && (Data[Start] == ' ' || Data[Start] == '\t'))
			{
				++Start;
			}

			// 뒤쪽 공백 제거
			while (End > Start && (Data[End - 1] == ' ' || Data[End - 1] == '\t'))
			{
				--End;
			}

			return {Data + Start, End - Start};
		}
	};

	/**
	 * 컴파일타임에 null-terminated enum 네임을 생성하는 클래스
	 */
	template <typename EnumType, EnumType Value, size_t N>
	struct EnumNameHolder
	{
		// null terminator 위한 +1
		char data[N + 1] = {};

		constexpr EnumNameHolder(const ConstexprStringView& InName) noexcept
		{
			for (size_t i = 0; i < N && i < InName.size(); ++i)
			{
				data[i] = InName[i];
			}

			// null terminator 추가
			data[N] = '\0';
		}
	};

	/**
	 * 컴파일타임에 enum 값의 이름을 추출하는 함수
	 */
	template <typename EnumType, EnumType Value>
	constexpr ConstexprStringView GetEnumNameRaw() noexcept
	{
#if defined(_MSC_VER)
		ConstexprStringView FunctionName = __FUNCSIG__;
#elif defined(__clang__) || defined(__GNUC__)
		ConstexprStringView FunctionName = __PRETTY_FUNCTION__;
#else
		return {};
#endif

		// MSVC: "constexpr_string_view __cdecl EnumReflection::GetEnumNameRaw<enum EKeyInput,EKeyInput::W>(void)"
		// 마지막 콤마 뒤부터 > 앞까지가 enum 값
		auto LastComma = FunctionName.find_last_of(',');
		if (LastComma == string_view::npos)
		{
			return {};
		}

		auto Start = LastComma + 1;
		auto End = FunctionName.find('>', Start);
		if (End == string_view::npos) return {};

		auto RawName = FunctionName.substr(Start, End - Start).trim();

		// enum 클래스명:: 제거
		auto ScopePosition = RawName.find_last_of(':');
		if (ScopePosition != string_view::npos && ScopePosition > 0 && RawName[ScopePosition - 1] == ':')
		{
			return RawName.substr(ScopePosition + 1);
		}

		return RawName;
	}

	/**
	 * null-terminated enum 네임 반환
	 */
	template <typename EnumType, EnumType Value>
	constexpr const char* GetEnumName() noexcept
	{
		constexpr auto RawName = GetEnumNameRaw<EnumType, Value>();
		if (RawName.empty()) return "";

		// 정확한 크기 계산
		constexpr size_t NameSize = RawName.size();
		static constexpr EnumNameHolder<EnumType, Value, NameSize> holder{RawName};
		return holder.data;
	}

	/**
	 * enum 값이 유효한지 검사
	 */
	template <typename EnumType, EnumType Value>
	constexpr bool IsValidEnum() noexcept
	{
		constexpr auto Name = GetEnumNameRaw<EnumType, Value>();
		return !Name.empty() &&
			!Name.contains('(') && // cast 연산자 감지
			!Name.starts_with('0') && !Name.starts_with('1') && !Name.starts_with('2') &&
			!Name.starts_with('3') && !Name.starts_with('4') && !Name.starts_with('5') &&
			!Name.starts_with('6') && !Name.starts_with('7') && !Name.starts_with('8') && !Name.starts_with('9');
	}

	/**
	 * enum 범위 설정
	 */
	template <typename EnumType>
	struct EnumRange
	{
		static constexpr int Min = 0; // uint8 enum은 0부터 시작
		static constexpr int Max = 255; // uint8 enum은 255까지
	};

	/**
	 * 컴파일타임 enum 배열 생성
	 */
	template <typename EnumType, size_t... Is>
	constexpr auto MakeEnumSequence(index_sequence<Is...>) noexcept
	{
		constexpr size_t N = sizeof...(Is);
		TStaticArray<EnumType, N> Values{};
		TStaticArray<const char*, N> Names{};
		size_t ValidCount = 0;

		// 각 인덱스를 enum 값으로 변환하고 유효성 검사
		(([&]
		{
			constexpr auto value = static_cast<EnumType>(Is + EnumRange<EnumType>::Min);
			if constexpr (IsValidEnum<EnumType, value>())
			{
				Values[ValidCount] = value;
				Names[ValidCount] = GetEnumName<EnumType, value>(); // null-terminated string
				++ValidCount;
			}
		}()), ...);

		return make_pair(Values, Names);
	}

	/**
	 * 컴파일타임 enum 메타데이터 생성
	 */
	template <typename EnumType>
	struct EnumInfo
	{
	private:
		static constexpr size_t Range = EnumRange<EnumType>::Max - EnumRange<EnumType>::Min + 1;
		using Indices = make_index_sequence<Range>;

	public:
		static constexpr auto Data = MakeEnumSequence<EnumType>(Indices{});
		static constexpr auto& Values = Data.first;
		static constexpr auto& Names = Data.second;

		static constexpr size_t GetCount() noexcept
		{
			size_t Count = 0;
			for (size_t i = 0; i < Range; ++i)
			{
				constexpr auto Value = static_cast<EnumType>(i + EnumRange<EnumType>::Min);
				if constexpr (IsValidEnum<EnumType, Value>())
				{
					++Count;
				}
			}
			return Count;
		}
	};
}

/**
 * @brief 컴파일타임 Enum Reflection API
 * 완전히 정적 데이터를 사용하여 런타임 메모리 할당 제거
 */
template <typename EnumType>
class TEnumReflector
{
public:
	using UnderlyingType = underlying_type_t<EnumType>;

	// enum 값을 문자열로 변환
	static constexpr const char* ToString(EnumType InValue) noexcept
	{
		return ToStringImpl(
			InValue,
			make_index_sequence<EnumReflection::EnumRange<EnumType>::Max - EnumReflection::EnumRange<EnumType>::Min + 1>
			{});
	}

	// 문자열을 enum 값으로 변환
	static constexpr TOptional<EnumType> FromString(const char* InName) noexcept
	{
		return FromStringImpl(
			InName, make_index_sequence<EnumReflection::EnumRange<EnumType>::Max - EnumReflection::EnumRange<
				EnumType>::Min + 1>{});
	}

	// FString 버전 (backward compatibility)
	static constexpr TOptional<EnumType> FromString(const FString& InName) noexcept
	{
		return FromString(InName.c_str());
	}

	// enum 값의 개수
	static constexpr size_t GetCount() noexcept
	{
		return CountValidEnums(
			make_index_sequence<EnumReflection::EnumRange<EnumType>::Max - EnumReflection::EnumRange<EnumType>::Min + 1>
			{});
	}

	// enum 값이 유효한지 확인 (컴파일타임)
	static constexpr bool IsValid(EnumType InValue) noexcept
	{
		return EnumReflection::IsValidEnum<EnumType, InValue>();
	}

private:
	// 컴파일타임 ToString 구현 (switch-case 생성)
	template <size_t... Is>
	static constexpr const char* ToStringImpl(EnumType InValue, index_sequence<Is...>) noexcept
	{
		const char* Result = "Unknown";
		(([&]
		{
			constexpr auto EnumValue = static_cast<EnumType>(Is + EnumReflection::EnumRange<EnumType>::Min);
			if constexpr (EnumReflection::IsValidEnum<EnumType, EnumValue>())
			{
				if (InValue == EnumValue)
				{
					Result = EnumReflection::GetEnumName<EnumType, EnumValue>();
				}
			}
		}()), ...);
		return Result;
	}

	// 컴파일타임 FromString 구현
	template <size_t... Is>
	static constexpr TOptional<EnumType> FromStringImpl(const char* InName, index_sequence<Is...>) noexcept
	{
		if (!InName)
		{
			return nullopt;
		}

		TOptional<EnumType> Result = nullopt;
		(([&]
		{
			constexpr auto EnumValue = static_cast<EnumType>(Is + EnumReflection::EnumRange<EnumType>::Min);
			if constexpr (EnumReflection::IsValidEnum<EnumType, EnumValue>())
			{
				const char* EnumName = EnumReflection::GetEnumName<EnumType, EnumValue>();
				if (StringEqual(InName, EnumName))
				{
					Result = EnumValue;
				}
			}
		}()), ...);
		return Result;
	}

	// 컴파일타임 개수 계산
	template <size_t... Is>
	static constexpr size_t CountValidEnums(index_sequence<Is...>) noexcept
	{
		size_t Count = 0;
		(([&]
		{
			constexpr auto EnumValue = static_cast<EnumType>(Is + EnumReflection::EnumRange<EnumType>::Min);
			if constexpr (EnumReflection::IsValidEnum<EnumType, EnumValue>())
			{
				++Count;
			}
		}()), ...);
		return Count;
	}

	// 문자열 비교 헬퍼
	static constexpr bool StringEqual(const char* InA, const char* InB) noexcept
	{
		if (!InA || !InB) return false;
		while (*InA && *InB && *InA == *InB)
		{
			++InA;
			++InB;
		}
		return *InA == *InB;
	}
};

// enum에 대해 사용할 수 있는 편의성 함수 전역 세팅
template <typename EnumType>
const char* EnumToString(EnumType InValue) noexcept
{
	return TEnumReflector<EnumType>::ToString(InValue);
}

template <typename EnumType>
TOptional<EnumType> StringToEnum(const FString& InName) noexcept
{
	return TEnumReflector<EnumType>::FromString(InName);
}

template <typename EnumType>
bool IsValidEnumValue(EnumType InValue) noexcept
{
	return TEnumReflector<EnumType>::IsValid(InValue);
}
