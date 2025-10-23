#pragma once

using std::string_view; // 문자열 소유 없이 관찰만 할 수 있는 객체 클래스, 포인터와 문자열 길이 정보만 보유함
using std::runtime_error; // exception 상속 클래스, 표준 예외
using std::is_arithmetic_v; // 컴파일 타임에 산술 타입인지 판정해주는 도구
using std::is_same_v; // 컴파일 타임에 두 타입이 같은지 확인해주는 도구
using std::is_pointer_v; // 주어진 타입이 포인터 타입인지 컴파일 타임에 확인해주는 타입 특성
using std::decay_t; // 참조, 한정자, 배열, 함수 속성을 제거한 가장 기본적인 타입으로 변환해주는 alias
using std::hex; // 출력 스트림에서 정수 출력 방식을 8진수로 변경하기 위한 조정자
using std::oct; // 출력 스트림에서 정수 출력 방식을 16진수로 변경하기 위한 조정자
using std::uppercase; // 16진수 출력 시 알파벳을 대문자로 표시하는 조정자
using std::scientific; // 부동소수점 숫자를 과학적 표기법으로 출력하도록 형식을 조정하는 조정자
using std::ostringstream; // 온메모리 문자열에 텍스트 출력할 수 있게 해주는 클래스
using std::forward; // 값 종류를 유지하면서 다른 함수로 전달하는 함수 탬플릿

/**
 * @brief UE_LOG Parser Class
 * printf와 최대한 호환될 수 있도록 구현 시도
 */
class UELogParser
{
public:
	/**
	 * @brief 파싱 결과 구조체
	 */
	struct ParseResult
	{
		bool bSuccess;
		FString FormattedMessage;
		FString ErrorMessage;

		ParseResult(bool InSuccess = false) : bSuccess(InSuccess)
		{
		}
	};

	/**
	 * @brief 메인 파싱 함수
	 * @param FormatString printf 스타일 포맷 문자열
	 * @param InArgs 가변 인자들
	 * @return 파싱 결과
	 */
	template <typename... Args>
	static ParseResult Parse(string_view FormatString, Args&&... InArgs)
	{
		try
		{
			// 컴파일 타임에 인자 개수와 타입 검증
			constexpr size_t ArgumentsCount = sizeof...(Args);

			// 런타임에 포맷 지정자 개수 확인
			size_t FormatSpecifierCount = CountFormatSpecifiers(FormatString);

			if (FormatSpecifierCount != ArgumentsCount)
			{
				return CreateErrorResult(
					"포맷 지정자 개수 (" + to_string(FormatSpecifierCount) +
					") 와 인자 개수 (" + to_string(ArgumentsCount) + ") 가 일치하지 않습니다."
				);
			}

			// 재귀적 포맷팅 시작
			FString Result;
			ProcessFormat(FormatString, Result, forward<Args>(InArgs)...);

			ParseResult Success(true);
			Success.FormattedMessage = Result;
			return Success;
		}
		catch (const exception& Exception)
		{
			return CreateErrorResult("파싱 중 오류 발생: " + FString(Exception.what()));
		}
	}

private:
	/**
	 * @brief 포맷 지정자 개수를 세는 함수
	 * @param Format 포맷 문자열
	 * @return 포맷 지정자 개수
	 */
	static size_t CountFormatSpecifiers(string_view Format)
	{
		size_t Count = 0;
		size_t Position = 0;

		while ((Position = Format.find('%', Position)) != string_view::npos)
		{
			if (Position + 1 < Format.length())
			{
				char NextChar = Format[Position + 1];
				if (NextChar != '%') // %%는 리터럴 %이므로 카운트하지 않음
				{
					++Count;
				}
				else
				{
					++Position; // %%의 경우 두 번째 %를 건너뜀
				}
			}
			++Position;
		}

		return Count;
	}

	/**
	 * @brief 재귀 종료 조건 - 인자가 없는 경우
	 */
	static void ProcessFormat(string_view Format, FString& Result)
	{
		// 남은 포맷 문자열에 %가 있으면 오류
		if (Format.find('%') != string_view::npos)
		{
			throw runtime_error("처리되지 않은 포맷 지정자가 있습니다.");
		}
		Result += Format;
	}

	/**
	 * @brief 재귀적 포맷 처리 함수 - 메인 로직
	 * @param Format 남은 포맷 문자열
	 * @param Result 결과 문자열 참조
	 * @param FirstArg 첫 번째 인자
	 * @param RemainingArgs 나머지 인자들
	 */
	template <typename T, typename... Args>
	static void ProcessFormat(string_view Format, FString& Result,
	                          T&& FirstArg, Args&&... RemainingArgs)
	{
		// '%' 문자 찾기
		auto PercentPos = Format.find('%');

		if (PercentPos == string_view::npos)
		{
			// %가 없으면 남은 문자열을 모두 추가하고 인자가 남았는지 확인
			Result += Format;
			if constexpr (sizeof...(RemainingArgs) > 0)
			{
				throw runtime_error("처리되지 않은 인자가 남아있습니다.");
			}
			return;
		}

		// '%' 이전까지의 문자열을 결과에 추가
		Result += Format.substr(0, PercentPos);

		// 포맷 지정자 처리
		if (PercentPos + 1 >= Format.length())
		{
			throw runtime_error("불완전한 포맷 지정자입니다.");
		}

		char FormatSpecifier = Format[PercentPos + 1];

		// %% 처리 (리터럴 %)
		if (FormatSpecifier == '%')
		{
			Result += '%';
			// 인자를 소모하지 않고 다음 처리
			ProcessFormat(Format.substr(PercentPos + 2), Result,
			              forward<T>(FirstArg), forward<Args>(RemainingArgs)...);
			return;
		}

		// 실제 포맷 지정자 처리
		FString FormattedValue = FormatArgument(FirstArg, FormatSpecifier);
		Result += FormattedValue;

		// 재귀 호출 - 다음 인자들 처리
		ProcessFormat(Format.substr(PercentPos + 2), Result, forward<Args>(RemainingArgs)...);
	}

	/**
	 * @brief 개별 인자를 포맷하는 함수 - 타입별 특화
	 * @param Arg 포맷할 인자
	 * @param FormatSpec 포맷 지정자
	 * @return 포맷된 문자열
	 */
	template <typename T>
	static FString FormatArgument(T&& Arg, char FormatSpec)
	{
		using DecayedType = decay_t<T>;

		try
		{
			switch (FormatSpec)
			{
			case 'd':
			case 'i':
				return FormatInteger(forward<T>(Arg));

			case 'u':
				return FormatUnsigned(forward<T>(Arg));

			case 'x':
			case 'X':
				return FormatHex(forward<T>(Arg), FormatSpec == 'X');

			case 'o':
				return FormatOctal(forward<T>(Arg));

			case 'f':
			case 'F':
				return FormatFloat(forward<T>(Arg));

			case 'e':
			case 'E':
				return FormatScientific(forward<T>(Arg), FormatSpec == 'E');

			case 'g':
			case 'G':
				return FormatGeneral(forward<T>(Arg), FormatSpec == 'G');

			case 'c':
				return FormatChar(forward<T>(Arg));

			case 's':
				return FormatString(forward<T>(Arg));

			case 'p':
				return FormatPointer(forward<T>(Arg));

			default:
				throw runtime_error("지원되지 않는 포맷 지정자: %" + FString(1, FormatSpec));
			}
		}
		catch (const exception& Exception)
		{
			throw runtime_error("인자 포맷팅 실패 (%" + FString(1, FormatSpec) + "): " + Exception.what());
		}
	}

	// 타입별 포맷팅 함수들
	template <typename T>
	static FString FormatInteger(T&& InValue)
	{
		if constexpr (is_arithmetic_v<decay_t<T>>)
		{
			return to_string(static_cast<long long>(InValue));
		}
		else if constexpr (is_same_v<decay_t<T>, FString> ||
			is_same_v<decay_t<T>, const char*> ||
			is_same_v<decay_t<T>, char*>)
		{
			FString Str = ConvertToString(InValue);
			return to_string(stoll(Str));
		}
		else
		{
			throw runtime_error("정수로 변환할 수 없는 타입입니다.");
		}
	}

	template <typename T>
	static FString FormatUnsigned(T&& InValue)
	{
		if constexpr (is_arithmetic_v<decay_t<T>>)
		{
			return to_string(static_cast<unsigned long long>(InValue));
		}
		else
		{
			FString Str = ConvertToString(InValue);
			return to_string(stoull(Str));
		}
	}

	template <typename T>
	static FString FormatHex(T&& InValue, bool bInUppercase)
	{
		ostringstream Stream;
		if constexpr (is_arithmetic_v<decay_t<T>>)
		{
			Stream << hex;
			if (bInUppercase) Stream << uppercase;
			Stream << static_cast<unsigned long long>(InValue);
		}
		else
		{
			FString Str = ConvertToString(InValue);
			unsigned long long Val = stoull(Str);
			Stream << hex;
			if (bInUppercase) Stream << uppercase;
			Stream << Val;
		}
		return Stream.str();
	}

	template <typename T>
	static FString FormatOctal(T&& InValue)
	{
		ostringstream Stream;
		if constexpr (is_arithmetic_v<decay_t<T>>)
		{
			Stream << oct << static_cast<unsigned long long>(InValue);
		}
		else
		{
			FString Str = ConvertToString(InValue);
			Stream << oct << stoull(Str);
		}
		return Stream.str();
	}

	template <typename T>
	static FString FormatFloat(T&& InValue)
	{
		if constexpr (is_arithmetic_v<decay_t<T>>)
		{
			return to_string(static_cast<double>(InValue));
		}
		else
		{
			FString Str = ConvertToString(InValue);
			return to_string(stod(Str));
		}
	}

	template <typename T>
	static FString FormatScientific(T&& InValue, bool bInUppercase)
	{
		ostringstream Stream;
		Stream << scientific;
		if (bInUppercase) Stream << uppercase;

		if constexpr (is_arithmetic_v<decay_t<T>>)
		{
			Stream << static_cast<double>(InValue);
		}
		else
		{
			FString Str = ConvertToString(InValue);
			Stream << stod(Str);
		}
		return Stream.str();
	}

	template <typename T>
	static FString FormatGeneral(T&& InValue, bool bInUppercase)
	{
		ostringstream Stream;
		if (bInUppercase) Stream << uppercase;

		if constexpr (is_arithmetic_v<decay_t<T>>)
		{
			double Val = static_cast<double>(InValue);
			// %g 형식: 지수가 -4보다 작거나 정밀도보다 크거나 같으면 %e, 아니면 %f
			if (abs(Val) < 1e-4 || abs(Val) >= 1e6)
			{
				Stream << scientific << Val;
			}
			else
			{
				Stream << Val;
			}
		}
		else
		{
			FString Str = ConvertToString(InValue);
			double Val = stod(Str);
			if (abs(Val) < 1e-4 || abs(Val) >= 1e6)
			{
				Stream << scientific << Val;
			}
			else
			{
				Stream << Val;
			}
		}
		return Stream.str();
	}

	template <typename T>
	static FString FormatChar(T&& InValue)
	{
		if constexpr (is_arithmetic_v<decay_t<T>>)
		{
			return FString(1, static_cast<char>(InValue));
		}
		else
		{
			FString Str = ConvertToString(InValue);
			if (!Str.empty())
			{
				return FString(1, Str[0]);
			}
			return "";
		}
	}

	template <typename T>
	static FString FormatString(T&& InValue)
	{
		return ConvertToString(forward<T>(InValue));
	}

	template <typename T>
	static FString FormatPointer(T&& InValue);

	template <typename T>
	static FString ConvertToString(T&& InValue);

	/**
	 * @brief 문자열에서 따옴표 제거
	 */
	static FString RemoveQuotes(const FString& InString)
	{
		if (InString.length() >= 2 && InString.front() == '"' && InString.back() == '"')
		{
			return InString.substr(1, InString.length() - 2);
		}
		return InString;
	}

public:
	/**
	 * @brief 에러 결과 생성
	 */
	static ParseResult CreateErrorResult(const FString& Error)
	{
		ParseResult Result(false);
		Result.ErrorMessage = Error;
		return Result;
	}
};

/**
 * @brief 외부 인터페이스 함수 선언
 * ConsoleWidget에서 UE_LOG 문자열을 파싱하기 위한 함수
 * @param InString UE_LOG 문자열 (예: "UE_LOG(\"Hello %d %s\", 123, \"World\")")
 * @return 파싱 결과
 */
UELogParser::ParseResult ParseUELogFromString(const FString& InString);

/**
 * @brief 다양한 타입을 문자열로 변환하는 범용 함수
 */
template <typename T>
FString UELogParser::ConvertToString(T&& InValue)
{
	using DecayedType = decay_t<T>;

	if constexpr (is_same_v<DecayedType, FString>)
	{
		return RemoveQuotes(InValue);
	}
	else if constexpr (is_same_v<DecayedType, const char*> || is_same_v<DecayedType, char*>)
	{
		return RemoveQuotes(FString(InValue));
	}
	else if constexpr (is_arithmetic_v<DecayedType>)
	{
		return to_string(InValue);
	}
	else
	{
		// 기타 타입들에 대해서는 스트림을 사용
		ostringstream Stream;
		Stream << InValue;
		return Stream.str();
	}
}

template <typename T>
FString UELogParser::FormatPointer(T&& InValue)
{
	ostringstream Stream;

	if constexpr (is_pointer_v<decay_t<T>>)
	{
		Stream << "0x" << hex << reinterpret_cast<uintptr_t>(InValue);
	}
	else if constexpr (is_arithmetic_v<decay_t<T>>)
	{
		Stream << "0x" << hex << static_cast<uintptr_t>(InValue);
	}
	else
	{
		Stream << "0x" << hex << reinterpret_cast<uintptr_t>(&InValue);
	}

	return Stream.str();
}
