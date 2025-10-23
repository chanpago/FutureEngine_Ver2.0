#include "pch.h"
#include "Render/UI/Widget/Public/SpriteSelectionWidget.h"

#include "Level/Public/Level.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Component/Public/BillBoardComponent.h"

#include <climits>

#include "Texture/Public/Texture.h"

IMPLEMENT_CLASS(USpriteSelectionWidget, UWidget)

void USpriteSelectionWidget::Initialize()
{
	// Do Nothing Here
}

void USpriteSelectionWidget::Update()
{
	ULevel* CurrentLevel = GWorld->GetLevel();

	if (CurrentLevel)
	{
		AActor* NewSelectedActor = GEditor->GetEditorModule()->GetSelectedActor();

		// Update Current Selected Actor
		if (NewSelectedActor && SelectedActor != NewSelectedActor)
		{
			SelectedActor = NewSelectedActor;

			for (UActorComponent* Component : SelectedActor->GetOwnedComponents())
			{
				if (UBillBoardComponent* UUIDTextComponent = Cast<UBillBoardComponent>(Component))
					{ SelectedBillBoard = UUIDTextComponent; }
			}
		}
	}
}

void USpriteSelectionWidget::RenderWidget()
{
	if (!SelectedActor) { return; }
	
	ImGui::Separator();
	ImGui::Text("Select Sprite");

	ImGui::Spacing();
		
	static int CurrentItem = 0;

	// ���� ���ڿ� ���
	TArray<FString> Items;
	const TMap<FName, UTexture*>& TextureCache = UAssetManager::GetInstance().GetTextureCache();

	int32 Idx = 0;
	for (auto Itr = TextureCache.begin(); Itr != TextureCache.end(); ++Itr, Idx++)
	{
		if (Itr->first == SelectedBillBoard->GetSprite()->GetFilePath()) { CurrentItem = Idx; }

		Items.push_back(Itr->first.ToString());
	}

	sort(Items.begin(), Items.end());
	
	if (ImGui::BeginCombo("Sprite", Items[CurrentItem].c_str()))
	{
		for (int32 N = 0; N < Items.size(); N++)
		{
			bool bIsSelected = (CurrentItem == N);
			if (ImGui::Selectable(Items[N].c_str(), bIsSelected))
			{
				CurrentItem = N;
				SetSpriteOfActor(Items[CurrentItem]);
			}

			if (bIsSelected)
				ImGui::SetItemDefaultFocus(); // �⺻ ��Ŀ��
		}
		ImGui::EndCombo();
	}

	ImGui::Separator();

	WidgetNum = (WidgetNum + 1) % std::numeric_limits<uint32>::max();
}

void USpriteSelectionWidget::SetSpriteOfActor(FString NewSprite)
{
	if (!SelectedActor)
		return;
	if (!SelectedBillBoard)
		return;

	const TMap<FName, UTexture*>& TextureCache = UAssetManager::GetInstance().GetTextureCache();

	SelectedBillBoard->SetSprite(TextureCache.find(NewSprite)->second);
}
