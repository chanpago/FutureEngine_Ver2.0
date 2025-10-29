#include "pch.h"
#include "Render/UI/Widget/Public/ConsoleWidget.h"
#include "Render/UI/Overlay/Public/StatOverlay.h"
#include "Utility/Public/UELogParser.h"
#include "Utility/Public/ScopeCycleCounter.h"
#include "Level/Public/Level.h"

IMPLEMENT_SINGLETON_CLASS(UConsoleWidget, UWidget)

UConsoleWidget::UConsoleWidget() = default;

UConsoleWidget::~UConsoleWidget()
{
	CleanupSystemRedirect();
	ClearLog();
}

void UConsoleWidget::Initialize()
{
	// 초기화
	ClearLog();
	memset(InputBuf, 0, sizeof(InputBuf));
	HistoryPosition = -1;
	bIsAutoScroll = true;
	bIsScrollToBottom = false;

	// Stream Redirection 초기화
	ConsoleOutputBuffer = nullptr;
	ConsoleErrorBuffer = nullptr;
	OriginalConsoleOutput = nullptr;
	OriginalConsoleError = nullptr;

	AddLog(ELogType::Success, "ConsoleWindow: Game Console 초기화 성공");
	AddLog(ELogType::System, "ConsoleWindow: Logging System Ready");
}

/**
 * @brief StreamBuffer Override 함수
 * @param InCharacter classic type char
 * @return 입력 받은 문자
 */
int ConsoleStreamBuffer::overflow(int InCharacter)
{
	if (InCharacter != EOF)
	{
		Buffer += static_cast<char>(InCharacter);
		if (InCharacter == '\n')
		{
			if (Console)
			{
				Console->AddSystemLog(Buffer.c_str(), bIsError);
			}
			Buffer.clear();
		}
	}
	return InCharacter;
}

streamsize ConsoleStreamBuffer::xsputn(const char* InString, streamsize InCount)
{
	for (std::streamsize i = 0; i < InCount; ++i)
	{
		overflow(InString[i]);
	}
	return InCount;
}

void UConsoleWidget::RenderWidget()
{
	// 버튼 색상을 검은색으로 설정
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
	
	// 제어 버튼들
	if (ImGui::Button("Clear"))
	{
		ClearLog();
	}

	ImGui::SameLine();
	if (ImGui::Button("Copy"))
	{
		ImGui::LogToClipboard();
	}
	
	ImGui::PopStyleColor(3);

	// ImGui::SameLine();
	// ImGui::Checkbox("AutoScroll", &bIsAutoScroll);

	ImGui::Separator();

	// 로그 출력 영역 미리 예약
	const float ReservedHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
	if (ImGui::BeginChild("LogOutput", ImVec2(0, -ReservedHeight), ImGuiChildFlags_NavFlattened,
	                      ImGuiWindowFlags_HorizontalScrollbar))
	{
		// 로그 리스트 출력
		for (const auto& LogEntry : LogItems)
		{
			// ELogType을 기반으로 색상 결정
			ImVec4 Color = GetColorByLogType(LogEntry.Type);
			bool bShouldApplyColor = (LogEntry.Type != ELogType::Info);

			if (bShouldApplyColor)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, Color);
			}

			ImGui::TextUnformatted(LogEntry.Message.c_str());

			if (bShouldApplyColor)
			{
				ImGui::PopStyleColor();
			}
		}

		// Auto Scroll
		if (bIsScrollToBottom || (bIsAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
		{
			ImGui::SetScrollHereY(1.0f);
		}
		bIsScrollToBottom = false;
	}

	ImGui::EndChild();
	ImGui::Separator();

	// Input Command with History Navigation
	bool ReclaimFocus = false;
	ImGuiInputTextFlags InputFlags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll |
		ImGuiInputTextFlags_CallbackHistory;
	
	// 입력 필드 색상을 검은색으로 설정
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
	
	if (ImGui::InputText("Input", InputBuf, sizeof(InputBuf), InputFlags,
	                     [](ImGuiInputTextCallbackData* data) -> int
	                     {
		                     return static_cast<UConsoleWidget*>(data->UserData)->HandleHistoryCallback(data);
	                     }, this))
	{
		if (strlen(InputBuf) > 0)
		{
			ProcessCommand(InputBuf);
			strcpy_s(InputBuf, sizeof(InputBuf), "");
			ReclaimFocus = true;
			// 새로운 명령어 입력 후 히스토리 위치 초기화
			HistoryPosition = -1;
		}
	}
	
	ImGui::PopStyleColor(3);

	// 입력 포커스 설정
	ImGui::SetItemDefaultFocus();
	if (ReclaimFocus)
	{
		ImGui::SetKeyboardFocusHere(-1);
	}
}

void UConsoleWidget::Update()
{
	// 필요한 경우 여기에 업데이트 로직 추가할 것
	// ImGui 위주라서 필요하지 않을 것으로 보이긴 함...
}

void UConsoleWidget::ClearLog()
{
	LogItems.clear();
}

/**
 * @brief ELogType에 따른 색상 반환
 */
ImVec4 UConsoleWidget::GetColorByLogType(ELogType InType)
{
	switch (InType)
	{
	case ELogType::Info:
		return {1.0f, 1.0f, 1.0f, 1.0f}; // 흰색
	case ELogType::Warning:
		return {1.0f, 1.0f, 0.4f, 1.0f}; // 노란색
	case ELogType::Error:
	case ELogType::TerminalError:
		return {1.0f, 0.4f, 0.4f, 1.0f}; // 빨간색
	case ELogType::Success:
	case ELogType::UELog:
		return {0.4f, 1.0f, 0.4f, 1.0f}; // 초록색
	case ELogType::System:
		return {0.7f, 0.7f, 0.7f, 1.0f}; // 회색
	case ELogType::Debug:
		return {0.4f, 0.6f, 1.0f, 1.0f}; // 파란색
	case ELogType::Terminal:
		return {0.6f, 0.8f, 1.0f, 1.0f}; // 하늘색
	case ELogType::Command:
		return {1.0f, 0.8f, 0.6f, 1.0f}; // 주황색
	default:
		return {1.0f, 1.0f, 1.0f, 1.0f}; // 기본 흰색
	}
}

/**
 * @brief 타입 없는 로그 작성 함수
 * 따로 LogType을 정의하지 않았다면, Info 타입의 로그로 추가한다
 * LogType을 뒤에 놓지 않는다면 기본값 설정이 불가능한데 다중 인자를 받고 있어 기본형을 이런 방식으로 처리
 */
void UConsoleWidget::AddLog(const char* fmt, ...)
{
	va_list args;

	// 가변 인자를 파라미터로 받는 Internal 함수 호출
	va_start(args, fmt);
	AddLogInternal(ELogType::Info, fmt, args);
	va_end(args);
}

/**
 * @brief 타입을 추가한 로그 작성 함수
 * 내부적으로 타입 없는 로그 함수와 동일하게 Internal 함수를 호출한다
 */
void UConsoleWidget::AddLog(ELogType InType, const char* fmt, ...)
{
	va_list args;

	// 가변 인자를 파라미터로 받는 Internal 함수 호출
	va_start(args, fmt);
	AddLogInternal(InType, fmt, args);
	va_end(args);
}

/**
 * @brief 로그를 내부적으로 처리하는 함수
 * 로그가 잘리는 현상을 방지하기 위해 동적 버퍼를 활용하여 로그를 입력 받음
 */
void UConsoleWidget::AddLogInternal(ELogType InType, const char* fmt, va_list InArguments)
{
	va_list ArgumentsCopy;

	// Get log length
	va_copy(ArgumentsCopy, InArguments);
	int LogLength = vsnprintf(nullptr, 0, fmt, ArgumentsCopy);
	va_end(ArgumentsCopy);

	// 필요한 크기만큼 동적 할당
	// malloc 대신 overloading 함수의 영향을 받을 수 있도록 new 할당 사용
	char* Buffer = new char[LogLength + 1];

	// Make full string
	va_copy(ArgumentsCopy, InArguments);
	(void)vsnprintf(Buffer, LogLength + 1, fmt, ArgumentsCopy);
	va_end(ArgumentsCopy);

	// 기본 Info 타입으로 log 추가
	FLogEntry LogEntry;
	LogEntry.Type = InType;

	// Log buffer 복사 후 제거
	LogEntry.Message = FString(Buffer);
	delete[] Buffer;

	LogItems.push_back(LogEntry);

	// Auto Scroll
	bIsScrollToBottom = true;
}

/**
 * @brief 시스템 로그들을 처리하기 위한 멤버 함수
 * @param InText log text
 * @param bInIsError 에러 여부
 */
void UConsoleWidget::AddSystemLog(const char* InText, bool bInIsError)
{
	if (!InText || strlen(InText) == 0)
	{
		return;
	}

	FLogEntry LogEntry;

	if (bInIsError)
	{
		LogEntry.Message = "Error: " + FString(InText);
		LogEntry.Type = ELogType::Error;
	}
	else
	{
		LogEntry.Message = "System: " + FString(InText);
		LogEntry.Type = ELogType::System;
	}

	// 끝에 있는 개행 문자 제거
	if (!LogEntry.Message.empty() && LogEntry.Message.back() == '\n')
	{
		LogEntry.Message.pop_back();
	}

	LogItems.push_back(LogEntry);

	// Auto Scroll
	bIsScrollToBottom = true;
}

/**
 * @brief 명령어 히스토리 탐색 콜백 함수
 * @param InData ImGui InputText 콜백 데이터
 * @return 콜백 처리 결과 (0: 성공)
 */
int UConsoleWidget::HandleHistoryCallback(ImGuiInputTextCallbackData* InData)
{
	switch (InData->EventFlag)
	{
	case ImGuiInputTextFlags_CallbackHistory:
		{
			// 비어있는 히스토리는 처리하지 않음
			if (CommandHistory.empty())
			{
				return 0;
			}

			const int HistorySize = static_cast<int>(CommandHistory.size());
			int PreviousHistoryPos = HistoryPosition;

			if (InData->EventKey == ImGuiKey_UpArrow)
			{
				// 위 화살표: 이전 명령어로 이동
				if (HistoryPosition == -1)
				{
					// 처음에는 가장 최근 명령어로 이동
					HistoryPosition = HistorySize - 1;
				}
				else if (HistoryPosition > 0)
				{
					// 더 이전 명령어로 이동
					--HistoryPosition;
				}
			}
			else if (InData->EventKey == ImGuiKey_DownArrow)
			{
				// 아래 화살표: 다음 명령어로 이동
				if (HistoryPosition != -1)
				{
					++HistoryPosition;
					if (HistoryPosition >= HistorySize)
					{
						// 범위를 벗어나면 입력창을 비우고 현재 상태로 돌아감
						HistoryPosition = -1;
					}
				}
			}

			// 히스토리 위치가 변경되었을 때만 입력창 업데이트
			if (PreviousHistoryPos != HistoryPosition)
			{
				if (HistoryPosition >= 0 && HistoryPosition < HistorySize)
				{
					// 선택된 히스토리 명령어로 입력창 채우기
					const FString& SelectedCommand = CommandHistory[HistoryPosition];
					InData->DeleteChars(0, InData->BufTextLen);
					InData->InsertChars(0, SelectedCommand.c_str());
				}
				else
				{
					// HistoryPosition == -1: 입력창 비우기
					InData->DeleteChars(0, InData->BufTextLen);
				}
			}
		}
		break;

	default:
		break;
	}

	return 0;
}

void UConsoleWidget::ProcessCommand(const char* InCommand)
{
	if (!InCommand || strlen(InCommand) == 0)
	{
		return;
	}

	// 명령 히스토리에 추가
	CommandHistory.push_back(FString(InCommand));
	HistoryPosition = -1;

	FString Input = InCommand;

	// UE_Log Parsing
	size_t StartPosition = Input.find("UE_LOG(");
	if (StartPosition != FString::npos && StartPosition == 0)
	{
		try
		{
			UELogParser::ParseResult Result = ParseUELogFromString(Input);

			if (Result.bSuccess)
			{
				// 파싱 성공
				FLogEntry LogEntry;
				LogEntry.Type = ELogType::UELog;
				LogEntry.Message = FString(Result.FormattedMessage);
				LogItems.push_back(LogEntry);
				bIsScrollToBottom = true;
			}
			else
			{
				// 파싱 실패
				FLogEntry ErrorEntry;
				ErrorEntry.Type = ELogType::Error;
				ErrorEntry.Message = "UELogParser: UE_LOG 파싱 오류: " + FString(Result.ErrorMessage);
				LogItems.push_back(ErrorEntry);
				bIsScrollToBottom = true;
			}
		}
		catch (const std::exception& e)
		{
			FLogEntry ErrorEntry;
			ErrorEntry.Type = ELogType::Error;
			ErrorEntry.Message = "UELogParser: 예외 발생: " + FString(e.what());
			LogItems.push_back(ErrorEntry);
			bIsScrollToBottom = true;
		}
		catch (...)
		{
			FLogEntry ErrorEntry;
			ErrorEntry.Type = ELogType::Error;
			ErrorEntry.Message = "UELogParser: 알 수 없는 오류가 발생했습니다.";
			LogItems.push_back(ErrorEntry);
			bIsScrollToBottom = true;
		}
	}

	// Clear 명령어 입력
	else if (FString CommandLower = InCommand;
		std::transform(CommandLower.begin(), CommandLower.end(), CommandLower.begin(), ::tolower),
		CommandLower == "clear")
	{
		ClearLog();
	}

	// Stat 명령어 처리
	else if (FString CommandLower = InCommand;
		std::transform(CommandLower.begin(), CommandLower.end(), CommandLower.begin(), ::tolower),
		CommandLower.length() > 5 && CommandLower.substr(0, 5) == "stat ")
	{
		FString StatCommand = CommandLower.substr(5);
		HandleStatCommand(StatCommand);
	}

	// Help 명령어 입력
	else if (FString CommandLower = InCommand;
		std::transform(CommandLower.begin(), CommandLower.end(), CommandLower.begin(), ::tolower),
		CommandLower == "help")
	{
		AddLog(ELogType::System, "Available Commands:");
		AddLog(ELogType::Info, "  CLEAR - Clear The Console");
		AddLog(ELogType::Info, "  HELP - Show This Help");
		AddLog(ELogType::Info, "  STAT FPS - Show FPS overlay");
		AddLog(ELogType::Info, "  STAT MEMORY - Show memory overlay");
		AddLog(ELogType::Info, "  STAT PICK - Show picking performance overlay");
		AddLog(ELogType::Info, "  STAT SHADOW - Show shadow overlay");
		AddLog(ELogType::Info, "  STAT NONE - Hide all overlays");
		AddLog(ELogType::Info, "  UE_LOG(\"String with format\", Args...) - Enhanced printf Formatting");
		AddLog(ELogType::Debug, "    기본 예제: UE_LOG(\"Hello World %%d\", 2025)");
		AddLog(ELogType::Debug, "    문자열: UE_LOG(\"User: %%s\", \"John\")");
		AddLog(ELogType::Debug, "    혼합형: UE_LOG(\"Player %%s has %%d points\", \"Alice\", 1500)");
		AddLog(ELogType::Debug, "    다중 인자: UE_LOG(\"Score: %%d, Lives: %%d\", 2500, 3)");
		AddLog(ELogType::Info, "");
		AddLog(ELogType::System, "Camera Controls:");
		AddLog(ELogType::Info, "  우클릭 + WASD - 카메라 이동");
		AddLog(ELogType::Info, "  우클릭 + Q/E - 위/아래 이동");
		AddLog(ELogType::Info, "  우클릭 + 마우스 이동 - 카메라 회전");
		AddLog(ELogType::Success, "  우클릭 + 마우스 휠 - 이동속도 조절 (20 ~ 50)");
		AddLog(ELogType::Info, "");
		AddLog(ELogType::System, "Terminal Commands:");
		AddLog(ELogType::Info, "  Any Windows command will be executed directly");
	}
	else if (FString CommandLower = InCommand;
		std::transform(CommandLower.begin(), CommandLower.end(), CommandLower.begin(), ::tolower),
		CommandLower.length() > 14 && CommandLower.substr(0, 14) == "shadow_filter ")
	{
		// "shadow_filter " 다음의 문자열(vsm, pcf, none)을 추출합니다.
		FString FilterType = CommandLower.substr(14);
		HandleShadowFilterCommand(FilterType);
	}
	else
	{
		// 실제 터미널 명령어 실행
		ExecuteTerminalCommand(InCommand);
	}

	// 스크롤 하단으로 이동
	bIsScrollToBottom = true;
}

void UConsoleWidget::HandleStatCommand(const FString& StatCommand)
{
	auto& StatOverlay = UStatOverlay::GetInstance();

	if (StatCommand == "fps")
	{
		StatOverlay.ShowFPS();
		AddLog(ELogType::Success, "FPS overlay");
	}
	else if (StatCommand == "memory")
	{
		StatOverlay.ShowMemory();
		AddLog(ELogType::Success, "Memory overlay");
	}
	else if (StatCommand == "pick" || StatCommand == "picking")
	{
		StatOverlay .ShowPicking();
		AddLog(ELogType::Success, "Picking overlay");
	}
	else if (StatCommand == "time")
	{
		StatOverlay.ShowTime();
		AddLog(ELogType::Success, "Time overlay");
	}
	else if (StatCommand == "decal")
	{
		StatOverlay.ShowDecal();
		AddLog(ELogType::Success, "Decal overlay");
	}
	else if (StatCommand == "shadow")
	{
		StatOverlay.ShowShadow();
		AddLog(ELogType::Success, "Shadow overlay");
	}
	else if (StatCommand == "all")
	{
		StatOverlay.ShowAll();
		AddLog(ELogType::Success, "All overlays");
	}
	else if (StatCommand == "none")
	{
		StatOverlay.ShowAll();
		AddLog(ELogType::Success, "All overlays disabled");
	}
	else
	{
		AddLog(ELogType::Error, "Unknown stat command: %s", StatCommand.c_str());
		AddLog(ELogType::Info, "Available: fps, memory, pick, decal, shadow, none");
	}
}

void UConsoleWidget::HandleShadowFilterCommand(const FString& FilterType)
{
	ULevel* CurrentLevel = (GWorld ? GWorld->GetLevel() : nullptr);
	if (!CurrentLevel)
	{
		AddLog(ELogType::Error, "Cannot change shadow filter: No level loaded.");
		return;
	}

	if (FilterType == "vsm")
	{
		CurrentLevel->SetShadowFilterType(EShadowFilterType::VSM);
		AddLog(ELogType::Success, "Shadow filter set to: VSM");
	}
	else if (FilterType == "pcf")
	{
		CurrentLevel->SetShadowFilterType(EShadowFilterType::PCF);
		AddLog(ELogType::Success, "Shadow filter set to: PCF");
	}
	else if (FilterType == "none")
	{
		CurrentLevel->SetShadowFilterType(EShadowFilterType::None);
		AddLog(ELogType::Success, "Shadow filter turned off (Hard Shadows).");
	}
	else
	{
		AddLog(ELogType::Error, "Unknown shadow filter type: '%s'", FilterType.c_str());
		AddLog(ELogType::Info,  "Available types: vsm, pcf, none");
	}
}

/**
 * @brief 실제 터미널 명령어를 실행하고 결과를 콘솔에 표시하는 함수
 * @param InCommand 실행할 터미널 명령어
 */
void UConsoleWidget::ExecuteTerminalCommand(const char* InCommand)
{
	if (!InCommand || strlen(InCommand) == 0)
	{
		return;
	}

	AddLog(ELogType::UELog, ("Terminal: Execute Command: " + FString(InCommand)).c_str());

	try
	{
		FString FullCommand = "powershell /c " + FString(InCommand);

		// Prepare Pipe for Create Process
		HANDLE PipeReadHandle, PipeWriteHandle;

		SECURITY_ATTRIBUTES SecurityAttribute = {};
		SecurityAttribute.nLength = sizeof(SECURITY_ATTRIBUTES);
		SecurityAttribute.bInheritHandle = TRUE;
		SecurityAttribute.lpSecurityDescriptor = nullptr;

		if (!CreatePipe(&PipeReadHandle, &PipeWriteHandle, &SecurityAttribute, 0))
		{
			AddLog("Terminal: Error: Failed To Create Pipe For Command Execution.");
			return;
		}

		// Set STARTUPINFO
		STARTUPINFOA StartUpInfo = {};
		StartUpInfo.cb = sizeof(STARTUPINFOA);
		StartUpInfo.dwFlags |= STARTF_USESTDHANDLES;
		StartUpInfo.hStdInput = nullptr;

		// stderr & stdout을 동일 파이프에서 처리
		StartUpInfo.hStdError = PipeWriteHandle;
		StartUpInfo.hStdOutput = PipeWriteHandle;

		PROCESS_INFORMATION ProcessInformation = {};

		// lpCommandLine Param Setting
		TArray<char> CommandLine(FullCommand.begin(), FullCommand.end());
		CommandLine.push_back('\0');

		// Create Process
		if (!CreateProcessA(nullptr, CommandLine.data(), nullptr,
		                    nullptr, TRUE, CREATE_NO_WINDOW,
		                    nullptr, nullptr, &StartUpInfo, &ProcessInformation))
		{
			AddLog("ConsoleWindow: Error: Failed To Execute Command: %s", InCommand);
			CloseHandle(PipeReadHandle);
			CloseHandle(PipeWriteHandle);
			return;
		}

		// 부모 프로세스에서는 쓰기 핸들이 불필요하므로 바로 닫도록 처리
		CloseHandle(PipeWriteHandle);

		// Read Execution Process With Timeout
		char Buffer[256];
		DWORD ReadDoubleWord;
		FString Output;
		bool bHasOutput = false;

		DWORD StartTime = GetTickCount();
		const DWORD TimeoutMsec = 2000;
		while (true)
		{
			// 타임아웃 체크
			if (GetTickCount() - StartTime > TimeoutMsec)
			{
				// 프로세스가 아직 실행 중이면 강제 종료
				DWORD ProcessExitCode;
				if (GetExitCodeProcess(ProcessInformation.hProcess, &ProcessExitCode) && ProcessExitCode ==
					STILL_ACTIVE)
				{
					TerminateProcess(ProcessInformation.hProcess, 1);
				}
				AddLog("ConsoleWindow: Error: Reading Timeout");
				break;
			}

			// 프로세스가 아직 실행 중인지 확인
			DWORD ProcessExitCode;
			if (GetExitCodeProcess(ProcessInformation.hProcess, &ProcessExitCode) && ProcessExitCode != STILL_ACTIVE)
			{
				// 프로세스가 종료되었으면 남은 모든 데이터를 읽고 종료
				DWORD AvailableBytes = 0;
				while (PeekNamedPipe(PipeReadHandle, nullptr, 0, nullptr, &AvailableBytes, nullptr) && AvailableBytes >
					0)
				{
					if (ReadFile(PipeReadHandle, Buffer, sizeof(Buffer) - 1,
					             &ReadDoubleWord, nullptr) && ReadDoubleWord > 0)
					{
						Buffer[ReadDoubleWord] = '\0';
						Output += Buffer;
						bHasOutput = true;
					}
					else
					{
						break; // ReadFile 실패 시 종료
					}
				}
				break;
			}

			// 파이프에 데이터가 있는지 확인
			DWORD AvailableBytes = 0;
			if (!PeekNamedPipe(PipeReadHandle, nullptr, 0, nullptr, &AvailableBytes, nullptr) || AvailableBytes == 0)
			{
				// 데이터가 없으면 잠시 대기
				Sleep(100);
				continue;
			}

			// 데이터 읽기
			if (ReadFile(PipeReadHandle, Buffer, sizeof(Buffer) - 1, &ReadDoubleWord, nullptr) && ReadDoubleWord > 0)
			{
				Buffer[ReadDoubleWord] = '\0'; // Null 문자로 String 끝 세팅
				Output += Buffer;
				bHasOutput = true;
				StartTime = GetTickCount();
			}
			else
			{
				// ReadFile 실패시 종료
				break;
			}
		}

		// Process 종료 대기
		DWORD WaitResult = WaitForSingleObject(ProcessInformation.hProcess, 2000);

		// 종료 코드 확인
		DWORD ExitCode = 0;
		if (WaitResult == WAIT_TIMEOUT)
		{
			// 타임아웃 발생 시 프로세스 강제 종료
			TerminateProcess(ProcessInformation.hProcess, 1);
			AddLog("ConsoleWindow: Error: Command Timeout & Terminated");
			ExitCode = 1;
		}
		else
		{
			GetExitCodeProcess(ProcessInformation.hProcess, &ExitCode);
		}

		// Release Handle
		CloseHandle(PipeReadHandle);
		CloseHandle(ProcessInformation.hProcess);
		CloseHandle(ProcessInformation.hThread);

		// Print Result
		if (bHasOutput)
		{
			FString Utf8Output = ConvertCP949ToUTF8(Output.c_str());

			std::istringstream Stream(Utf8Output);
			FString Line;
			while (std::getline(Stream, Line))
			{
				// istringstream이 \r을 남길 수 있으므로 제거
				if (!Line.empty() && Line.back() == '\r')
				{
					Line.pop_back();
				}
				if (!Line.empty())
				{
					AddLog("%s", Line.c_str());
				}
			}
		}
		else if (ExitCode == 0)
		{
			// 출력은 없지만 성공적으로 실행된 경우
			AddLog("Terminal: Command 실행 성공 (No Output)");
		}

		// 에러 코드가 있는 경우 표시
		if (ExitCode != 0)
		{
			AddLog("Terminal: Error: Command failed with exit code: %d", ExitCode);
		}
	}
	catch (const exception& Exception)
	{
		AddLog("Terminal: Error: Exception occurred: %s", Exception.what());
	}
	catch (...)
	{
		AddLog("Terminal: Error: Unknown Error Occurred While Executing Command");
	}
}

/**
 * @brief System output redirection implementation
 */
void UConsoleWidget::InitializeSystemRedirect()
{
	try
	{
		// 유효성 체크
		if (OriginalConsoleOutput != nullptr || OriginalConsoleError != nullptr)
		{
			return;
		}

		if (!cout.rdbuf() || !cerr.rdbuf())
		{
			AddLog(ELogType::Error, "ConsoleWindow: cout/cerr streams are not available");
			return;
		}

		// Save Original Stream Buffers
		OriginalConsoleOutput = cout.rdbuf();
		OriginalConsoleError = cerr.rdbuf();

		// Create Custom Stream Buffers
		ConsoleOutputBuffer = new ConsoleStreamBuffer(this, false);
		ConsoleErrorBuffer = new ConsoleStreamBuffer(this, true);

		// Redirect ConsoleOutput and ConsoleError To Our Custom Buffers
		cout.rdbuf(ConsoleOutputBuffer);
		cerr.rdbuf(ConsoleErrorBuffer);

		AddLog(ELogType::System, "ConsoleWindow: Console Output Redirection Initialized");
	}
	catch (const exception& Exception)
	{
		AddLog(ELogType::Error, "ConsoleWindow: Failed To Initialize Console Output Redirection: %s", Exception.what());
	}
	catch (...)
	{
		AddLog(ELogType::Error, "ConsoleWindow: Failed To Initialize Console Output Redirection: Unknown Error");
	}
}

void UConsoleWidget::CleanupSystemRedirect()
{
	try
	{
		// Restore Original Stream Buffers
		if (OriginalConsoleOutput)
		{
			cout.rdbuf(OriginalConsoleOutput);
			OriginalConsoleOutput = nullptr;
		}

		if (OriginalConsoleError)
		{
			cerr.rdbuf(OriginalConsoleError);
			OriginalConsoleError = nullptr;
		}

		// Delete Custom Stream Buffers
		if (ConsoleOutputBuffer)
		{
			delete ConsoleOutputBuffer;
			ConsoleOutputBuffer = nullptr;
		}

		if (ConsoleErrorBuffer)
		{
			delete ConsoleErrorBuffer;
			ConsoleErrorBuffer = nullptr;
		}
	}
	catch (...)
	{
		// Ignore Cleanup Errors
	}
}
