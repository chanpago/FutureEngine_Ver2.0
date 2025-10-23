#include "pch.h"
#include "Render/UI/Widget/Public/AmbientLightComponentWidget.h"
#include "Component/Public/AmbientLightComponent.h"

IMPLEMENT_CLASS(UAmbientLightComponentWidget, UWidget)

void UAmbientLightComponentWidget::Update()
{
	ULevel* CurrentLevel = GWorld->GetLevel();
	if (CurrentLevel)
	{
		UActorComponent* NewSelectedComponent = GEditor->GetEditorModule()->GetSelectedComponent();
		if (AmbientLightComponent != NewSelectedComponent)
		{
			AmbientLightComponent = Cast<UAmbientLightComponent>(NewSelectedComponent);
		}
	}
}

void UAmbientLightComponentWidget::RenderWidget()
{
	if (!AmbientLightComponent)
	{
		return;
	}
	
	// 모든 입력 필드를 검은색으로 설정
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
	// 라이트 활성화 체크박스
	bool LightEnabled = AmbientLightComponent->GetLightEnabled();
	if (ImGui::Checkbox("Light Enabled", &LightEnabled))
	{
		AmbientLightComponent->SetLightEnabled(LightEnabled);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("라이트를 켜고 끕니다.\n끄면 조명 계산에서 제외됩니다.\n(Outliner O/X와는 별개)");
	}
	// Light Color
	FVector LightColor = AmbientLightComponent->GetLightColor();
	float LightColorRGB[3] = { LightColor.X * 255.0f, LightColor.Y * 255.0f, LightColor.Z * 255.0f };
	
	bool ColorChanged = false;
	ImDrawList* DrawList = ImGui::GetWindowDrawList();
	float BoxWidth = 65.0f;
	
	ImGui::SetNextItemWidth(BoxWidth);
	ImVec2 PosR = ImGui::GetCursorScreenPos();
	ColorChanged |= ImGui::DragFloat("##R", &LightColorRGB[0], 1.0f, 0.0f, 255.0f, "R: %.0f");
	ImVec2 SizeR = ImGui::GetItemRectSize();
	DrawList->AddLine(ImVec2(PosR.x + 5, PosR.y + 2), ImVec2(PosR.x + 5, PosR.y + SizeR.y - 2), IM_COL32(255, 0, 0, 255), 2.0f);
	ImGui::SameLine();
	
	ImGui::SetNextItemWidth(BoxWidth);
	ImVec2 PosG = ImGui::GetCursorScreenPos();
	ColorChanged |= ImGui::DragFloat("##G", &LightColorRGB[1], 1.0f, 0.0f, 255.0f, "G: %.0f");
	ImVec2 SizeG = ImGui::GetItemRectSize();
	DrawList->AddLine(ImVec2(PosG.x + 5, PosG.y + 2), ImVec2(PosG.x + 5, PosG.y + SizeG.y - 2), IM_COL32(0, 255, 0, 255), 2.0f);
	ImGui::SameLine();
	
	ImGui::SetNextItemWidth(BoxWidth);
	ImVec2 PosB = ImGui::GetCursorScreenPos();
	ColorChanged |= ImGui::DragFloat("##B", &LightColorRGB[2], 1.0f, 0.0f, 255.0f, "B: %.0f");
	ImVec2 SizeB = ImGui::GetItemRectSize();
	DrawList->AddLine(ImVec2(PosB.x + 5, PosB.y + 2), ImVec2(PosB.x + 5, PosB.y + SizeB.y - 2), IM_COL32(0, 0, 255, 255), 2.0f);
	ImGui::SameLine();
	
	float LightColor01[3] = { LightColorRGB[0] / 255.0f, LightColorRGB[1] / 255.0f, LightColorRGB[2] / 255.0f };
	if (ImGui::ColorEdit3("Light Color", LightColor01, ImGuiColorEditFlags_NoInputs))
	{
		LightColorRGB[0] = LightColor01[0] * 255.0f;
		LightColorRGB[1] = LightColor01[1] * 255.0f;
		LightColorRGB[2] = LightColor01[2] * 255.0f;
		ColorChanged = true;
	}
	
	if (ColorChanged)
	{
		LightColor.X = LightColorRGB[0] / 255.0f;
		LightColor.Y = LightColorRGB[1] / 255.0f;
		LightColor.Z = LightColorRGB[2] / 255.0f;
		AmbientLightComponent->SetLightColor(LightColor);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("환경광의 색상입니다.\n씬 전체에 균일한 기본 조명을 제공합니다.");
	}

	float Intensity = AmbientLightComponent->GetIntensity();
	if (ImGui::DragFloat("Intensity", &Intensity, 0.1f, 0.0f, 20.0f))
	{
		AmbientLightComponent->SetIntensity(Intensity);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("환경광 밝기\n범위: 0.0(꺼짐) ~ 20.0(최대)");
	}
	
	ImGui::PopStyleColor(3);

	ImGui::Separator();

}
