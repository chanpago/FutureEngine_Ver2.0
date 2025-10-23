#include "pch.h"
#include "Render/UI/Widget/Public/ActorTerminationWidget.h"
#include "Level/Public/Level.h"
#include "Manager/Input/Public/InputManager.h"

IMPLEMENT_CLASS(UActorTerminationWidget, UWidget)

void UActorTerminationWidget::Initialize()
{
	// Do Nothing Here
}

void UActorTerminationWidget::Update()
{
	// Do Nothing Here
}

void UActorTerminationWidget::RenderWidget()
{
	auto& InputManager = UInputManager::GetInstance();
	AActor* SelectedActor = GEditor->GetEditorModule()->GetSelectedActor();
	if (!SelectedActor)
	{
		return;
	}

	if (InputManager.IsKeyPressed(EKeyInput::Delete))
	{
		// 컴포넌트 선택시 컴포넌트 삭제를 우선
		if (ImGui::IsWindowFocused() && ActorDetailWidget)
		{
			if (UActorComponent* ActorComp = ActorDetailWidget->GetSelectedComponent())
			{
				DeleteSelectedComponent(SelectedActor, ActorComp);
			}
		}
		else
		{
			DeleteSelectedActor(SelectedActor);
		}
	}
}

/**
 * @brief Selected Actor 삭제 함수
 */
void UActorTerminationWidget::DeleteSelectedActor(AActor* InSelectedActor)
{
	UE_LOG("ActorTerminationWidget: 삭제를 위한 Actor Marking 시작");
	if (!InSelectedActor)
	{
		UE_LOG("ActorTerminationWidget: 삭제를 위한 Actor가 선택되지 않았습니다");
		return;
	}

	ULevel* CurrentLevel = GWorld->GetLevel();

	if (!CurrentLevel)
	{
		UE_LOG_ERROR("ActorTerminationWidget: No Current Level To Delete Actor From");
		return;
	}

	UE_LOG_INFO("ActorTerminationWidget: 선택된 Actor를 삭제를 위해 마킹 처리: %s",
	       InSelectedActor->GetName() == FName::GetNone() ? "UnNamed" : InSelectedActor->GetName().ToString().data());

	// 지연 삭제를 사용하여 안전하게 다음 틱에서 삭제
	GWorld->DestroyActor(InSelectedActor);
}

void UActorTerminationWidget::DeleteSelectedComponent(AActor* InSelectedActor, UActorComponent* InSelectedComponent)
{
	bool bSuccess = InSelectedActor->RemoveComponent(InSelectedComponent, true);
	if (bSuccess)
	{
		ActorDetailWidget->SetSelectedComponent(nullptr);
	}
}