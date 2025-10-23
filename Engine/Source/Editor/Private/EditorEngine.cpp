#include "pch.h"
#include "Core/Public/Object.h"
#include "Editor/Public/EditorEngine.h"
#include "Editor/Public/Editor.h"
#include "Level/Public/Level.h"
#include "Manager/Config/Public/ConfigManager.h"
#include "Manager/Path/Public/PathManager.h"


IMPLEMENT_CLASS(UEditorEngine, UObject)
UEditorEngine* GEditor = nullptr;
UWorld* GWorld = nullptr;
UEditorEngine::UEditorEngine()
{
    GEditor = this;
    UWorld* EditorWorld = NewObject<UWorld>(this);
    EditorWorld->SetWorldType(EWorldType::Editor);

    if (EditorWorld)
    {
        FWorldContext EditorContext;
        EditorContext.SetWorld(EditorWorld);
        WorldContexts.push_back(EditorContext); 

        GWorld = EditorWorld;
    }
    EditorModule = NewObject<UEditor>();
    
    FString LastSavedLevelPath = UConfigManager::GetInstance().GetLastSavedLevelPath();
    bool bSuccessLoad = LoadLevel(LastSavedLevelPath);
    if (!bSuccessLoad) { CreateNewLevel(); }
    EditorWorld->BeginPlay();
}

UEditorEngine::~UEditorEngine()
{
    if (IsPIESessionActive()) { EndPIE(); }

    for (auto WorldContext : WorldContexts)
    {
        if (WorldContext.World())
        {
            delete WorldContext.World();
        }
    }
    if (EditorModule)
    {
        delete EditorModule;
    }
    
    GWorld = nullptr;
}

void UEditorEngine::Tick(float DeltaSeconds)
{
    for (FWorldContext& Context : WorldContexts)
    {
        UWorld* World = Context.World();
        if (World)
        {
            if (World->GetWorldType() == EWorldType::Editor)
            {
                World->Tick(DeltaSeconds);
            }
            else if (World->GetWorldType() == EWorldType::PIE)
            {
                // PIE 상태가 Playing일 때만 틱을 실행
                if (PIEState == EPIEState::Playing)
                {
                    World->Tick(DeltaSeconds);
                }
            }
        }
    }

    if (EditorModule)
    {
        EditorModule->Update();
    }
}

bool UEditorEngine::IsPIESessionActive() const
{    
    for (const FWorldContext& Context : WorldContexts)
    {
        if (Context.World() && Context.GetType() == EWorldType::PIE)
        {
            return true;
        }
    }
    return false;
}

void UEditorEngine::StartPIE()
{
    if (PIEState != EPIEState::Stopped) { return; }
    PIEState = EPIEState::Playing;
    UWorld* EditorWorld = GetEditorWorldContext().World();
    if (!EditorWorld) { return; }

    UWorld* PIEWorld = Cast<UWorld>(EditorWorld->Duplicate());
    
    if (PIEWorld)
    {
        PIEWorld->SetWorldType(EWorldType::PIE);
        FWorldContext PIEContext;
        PIEContext.SetWorld(PIEWorld);
        WorldContexts.push_back(PIEContext); 
        GWorld = PIEWorld;

        PIEWorld->BeginPlay();
    }
}

void UEditorEngine::EndPIE()
{
    if (PIEState != EPIEState::Playing) { return; }
    PIEState = EPIEState::Stopped;
    FWorldContext* PIEContext = GetPIEWorldContext();
    if (PIEContext)
    {
        UWorld* PIEWorld = PIEContext->World();
        PIEWorld->EndPlay();
        delete PIEWorld;
        
        WorldContexts.erase(std::remove(WorldContexts.begin(), WorldContexts.end(), *PIEContext),WorldContexts.end());
    }
    
    GWorld = GetEditorWorldContext().World(); 
    GWorld->BeginPlay();
}

void UEditorEngine::PausePIE()
{
    if (PIEState != EPIEState::Playing) { return; }
    PIEState = EPIEState::Paused;
}

void UEditorEngine::ResumePIE()
{
    if (PIEState != EPIEState::Paused) { return; }
    PIEState = EPIEState::Playing;
}

bool UEditorEngine::LoadLevel(const FString& InFilePath)
{
    UE_LOG("GEditor: Loading Level: %s", InFilePath.data());
    
    // PIE 실행 시 PIE 종료 후 로직 실행
    if (IsPIESessionActive()) { EndPIE(); }
    return GetEditorWorldContext().World()->LoadLevel(path(InFilePath));
}

bool UEditorEngine::SaveCurrentLevel(const FString& InLevelName)
{
    UE_LOG("GEditor: Saving Level: %s", InLevelName.c_str());
    
    // PIE 실행 시 PIE 종료 후 로직 실행
    if (IsPIESessionActive()) { EndPIE(); }

    path FilePath = InLevelName;
    if (FilePath.empty())
    {
        FName CurrentLevelName = GetEditorWorldContext().World()->GetLevel()->GetName();
        FilePath = GenerateLevelFilePath(CurrentLevelName == FName::GetNone()? "Untitled" : CurrentLevelName.ToString());
    }

    UE_LOG("GEditor: 현재 레벨을 다음 경로에 저장합니다: %s", FilePath.string().c_str());

    try
    {
        bool bSuccess = GetEditorWorldContext().World()->SaveCurrentLevel(FilePath);
        if (bSuccess)
        {
            UConfigManager::GetInstance().SetLastUsedLevelPath(InLevelName);

            UE_LOG("GEditor: 레벨이 성공적으로 저장되었습니다");
        }
        else
        {
            UE_LOG("GEditor: 레벨을 저장하는 데에 실패했습니다");
        }
        return bSuccess;
    }
    catch (const exception& Exception)
    {
        UE_LOG("GEditor: 저장 과정에서 Exception 발생: %s", Exception.what());
        return false;
    }
}

bool UEditorEngine::CreateNewLevel(const FString& InLevelName)
{
    UE_LOG("GEditor: Create New Level: %s", InLevelName.c_str());
    
    // PIE 실행 시 PIE 종료 후 로직 실행
    if (IsPIESessionActive()) { EndPIE(); }
    GetEditorWorldContext().World()->CreateNewLevel(InLevelName);
    return true;
}

path UEditorEngine::GenerateLevelFilePath(const FString& InLevelName)
{
    path LevelDirectory = GetLevelDirectory();
    path FileName = InLevelName + ".Scene";
    return LevelDirectory / FileName;
}

path UEditorEngine::GetLevelDirectory()
{
    UPathManager& PathManager = UPathManager::GetInstance();
    return PathManager.GetWorldPath();
}

FWorldContext* UEditorEngine::GetPIEWorldContext()
{
    for (FWorldContext& Context : WorldContexts)
    {
        if (Context.World() && Context.GetType() == EWorldType::PIE)
        {
            return &Context; 
        }
    }
    return nullptr;
}

FWorldContext* UEditorEngine::GetActiveWorldContext()
{
    FWorldContext* PIEContext = GetPIEWorldContext();
    if (PIEContext)
    {
        return PIEContext;
    }
    
    if (!WorldContexts.empty()) { return &WorldContexts[0]; }

    return nullptr;
}
