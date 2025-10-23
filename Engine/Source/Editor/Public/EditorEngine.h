#pragma once
#include "Core/Public/Object.h"
#include "Level/Public/World.h"

class UEditor;
/**
 * @brief UWorld 인스턴스와 그에 대한 정보를 담는 구조체
 */
struct FWorldContext
{
private:
    UWorld* WorldPtr = nullptr;
public:
    UWorld* World() const { return WorldPtr; }
    EWorldType GetType() const { return WorldPtr ? WorldPtr->GetWorldType() : EWorldType::Game; } 
    void SetWorld(UWorld* InWorld) { WorldPtr = InWorld; }
    bool operator==(const FWorldContext& Other) const { return WorldPtr == Other.WorldPtr; }
};

enum class EPIEState
{
    Stopped,
    Playing,
    Paused
};

/**
 * @brief 에디터의 최상위 제어 객체, World들을 관리
 * ClientApp 이외의 곳에서 별도로 생성하거나, GEditor 교체 절대 금지
 */
UCLASS()
class UEditorEngine final : public UObject
{
    GENERATED_BODY()
    DECLARE_CLASS(UEditorEngine, UObject)

public:
    UEditorEngine();
    ~UEditorEngine();
    
    /**
     * @brief WorldContext를 순회하며 World의 Tick을 처리, EditorModule Update
     */
    void Tick(float DeltaSeconds);

// World Management
    /**
     * @brief 기본으로 가지고 있는 가장 메인이 되는 에디터 메인 월드 컨텍스트 반환
     */
    FWorldContext& GetEditorWorldContext() { return WorldContexts[0]; }
    /**
     * @brief PIE가 활성화되어 있는지 확인
     */
    bool IsPIESessionActive() const;
    EPIEState GetPIEState() const { return PIEState; }
    /**
     * brief 에디터 월드를 복제해 PIE 시작
     */
    void StartPIE();
    /**
     * @brief PIE 종료하고 에디터 월드로 돌아감
     */
    void EndPIE();
    /**
     * @brief PIE 일시정지
     */
    void PausePIE();
    /**
     * @brief PIE 재개
     */
    void ResumePIE();

    // Level Management
    /**
     * @brief 경로의 파일을 불러와서 현재 Editor 월드의 Level 교체 
     */
    bool LoadLevel(const FString& InFilePath); 
    
    /**
     * @brief 현재 Editor 월드의 레벨을 파일로 저장
     */
    bool SaveCurrentLevel(const FString& InLevelName = "Untitled");
    
    /**
     * @brief 현재 Editor 월드에 새 레벨 변경
     */
	bool CreateNewLevel(const FString& InLevelName = "Untitled");
    static std::filesystem::path GetLevelDirectory();
    static std::filesystem::path GenerateLevelFilePath(const FString& InLevelName);

    /**
     * @brief 에디터 UI/상호작용 담당하는 UEditor 반환
     */
    UEditor* GetEditorModule() const { return EditorModule; }

private:
    // PIE 월드의 FWorldContext를 찾아서 반환
    FWorldContext* GetPIEWorldContext();
    // 현재 PIE 세션 중인지 확인하고, 그렇다면 현재 WorldContext를 반환
    FWorldContext* GetActiveWorldContext();
    
    EPIEState PIEState = EPIEState::Stopped;
    TArray<FWorldContext> WorldContexts;
    UEditor* EditorModule;
};

// UEditorEngine의 전역 인스턴스 포인터
extern UEditorEngine* GEditor; 
// 현재 활성화된 UWorld 포인터 
extern UWorld* GWorld;
