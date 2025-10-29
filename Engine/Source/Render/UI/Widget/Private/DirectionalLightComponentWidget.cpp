#include "pch.h"
#include "Render/UI/Widget/Public/DirectionalLightComponentWidget.h"
#include "Component/Public/DirectionalLightComponent.h"
#include "Editor/Public/Editor.h"
#include "Level/Public/Level.h"
#include "Component/Public/ActorComponent.h"
#include "Render/Renderer/Public/Renderer.h"
#include "ImGui/imgui.h"

IMPLEMENT_CLASS(UDirectionalLightComponentWidget, UWidget)

void UDirectionalLightComponentWidget::Initialize()
{
}

void UDirectionalLightComponentWidget::Update()
{
    ULevel* CurrentLevel = GWorld->GetLevel();
    if (CurrentLevel)
    {
        UActorComponent* NewSelectedComponent = GEditor->GetEditorModule()->GetSelectedComponent();
        if (DirectionalLightComponent != NewSelectedComponent)
        {
            DirectionalLightComponent = Cast<UDirectionalLightComponent>(NewSelectedComponent);
        }
    }
}

void UDirectionalLightComponentWidget::RenderWidget()
{
    if (!DirectionalLightComponent)
    {
        return;
    }

    ImGui::Separator();
    
    // 모든 입력 필드를 검은색으로 설정
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
	// 라이트 활성화 체크박스
    bool LightEnabled = DirectionalLightComponent->GetLightEnabled();
    if (ImGui::Checkbox("Light Enabled", &LightEnabled))
    {
        DirectionalLightComponent->SetLightEnabled(LightEnabled);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("라이트를 켜고 끉니다.\n끄면 조명 계산에서 제외됩니다.\n(Outliner O/X와는 별개)");
    }
    
    // 그림자 캐스팅 체크박스
    bool bCastShadows = DirectionalLightComponent->GetCastShadows();
    if (ImGui::Checkbox("Cast Shadows (PSM)", &bCastShadows))
    {
        DirectionalLightComponent->SetCastShadows(bCastShadows);
        UE_LOG("bCastShadows Value : %i", bCastShadows);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("그림자 렌더링 방식 선택:\n"
                          "☐ OFF: Simple Ortho Shadow (LVP) - 안정적, 카메라 독립\n"
                          "☑ ON: PSM (Perspective Shadow Maps) - 카메라 종속, 높은 정밀도");
    }
    // Light Color
    FVector LightColor = DirectionalLightComponent->GetLightColor();
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
        DirectionalLightComponent->SetLightColor(LightColor);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("라이트 필터 색입니다.\n이 색을 조절하면 실제 라이트의 강도가 조절되는 것과 같은 효과가 생기게 됩니다.");
    }

    float Intensity = DirectionalLightComponent->GetIntensity();
    if (ImGui::DragFloat("Intensity", &Intensity, 0.1f, 0.0f, 20.0f))
    {
        DirectionalLightComponent->SetIntensity(Intensity);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("디렉셔널 라이트 밝기\n범위: 0.0(꺼짐) ~ 20.0(최대)");
    }
    
    ImGui::PopStyleColor(3);

    // Shadow Map Preview
    if (ImGui::CollapsingHeader("Shadow Map Preview"))
    {
        ID3D11ShaderResourceView* ShadowSRV = URenderer::GetInstance().GetDeviceResources()->GetDirectionalShadowMapSRV();
        if (ShadowSRV)
        {
            // 표시 크기와 스케일 조절 UI
            static int Size = 256;
            ImGui::SliderInt("Size", &Size, 64, 1024);
            ImGui::Image((ImTextureID)ShadowSRV, ImVec2((float)Size, (float)Size));
            ImGui::Text("Resolution: 2048 x 2048 (Depth)" );
        }
        else
        {
            ImGui::TextColored(ImVec4(0.8f,0.5f,0.5f,1.0f), "Shadow map SRV not available.");
        }

        ImGui::SeparatorText("Cascaded Shadow Maps");
        static int cascadePreviewIndex = 0;
        int numActiveCascades = 8;
        ImGui::SliderInt("Preview Index", &cascadePreviewIndex, 0, numActiveCascades - 1);
        
        ID3D11ShaderResourceView* cascadeSRV = URenderer::GetInstance().GetDeviceResources()->GetCascadedShadowMapSliceSRV(cascadePreviewIndex);
        if (cascadeSRV)
        {
            ImGui::Image((ImTextureID)cascadeSRV, ImVec2(256, 256));
            ImGui::Text("Resolution: 2048 x 2048 (Depth)");
        }
        else
        {
            ImGui::Text("Cascade SRV not available for index %d.", cascadePreviewIndex);
        }
    }


    ImGui::Separator();
}
