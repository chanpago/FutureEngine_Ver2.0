#include "pch.h"
#include "Render/UI/Widget/Public/DirectionalLightComponentWidget.h"
#include "Component/Public/DirectionalLightComponent.h"
#include "Editor/Public/Editor.h"
#include "Level/Public/Level.h"
#include "Component/Public/ActorComponent.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Manager/UI/Public/ViewportManager.h"
#include "Editor/Public/Camera.h"
#include "Manager/UI/Public/ViewportManager.h"
#include "Render/UI/Viewport/Public/ViewportClient.h"
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

    // Override camera with light's perspective (orientation only) + auto-restore
    static bool bViewFromLight = false;
    static bool bPrevViewFromLight = false;
    struct FSavedCam { bool Has=false; ECameraType Type; FVector Loc; FVector Rot; float Fov; float NearZ; float FarZ; };
    static FSavedCam Saved{};
    bool toggled = ImGui::Checkbox("View From Light (Camera)", &bViewFromLight);
    UViewportManager& VM = UViewportManager::GetInstance();
    int32 active = VM.GetActiveIndex();
    auto& clients = VM.GetClients();
    UCamera* Cam = (active >= 0 && active < (int)clients.size() && clients[active]) ? clients[active]->GetCamera() : nullptr;

    if (toggled)
    {
        if (bViewFromLight && Cam)
        {
            // Save state on enable
            Saved.Has = true;
            Saved.Type = Cam->GetCameraType();
            Saved.Loc  = Cam->GetLocation();
            Saved.Rot  = Cam->GetRotation();
            Saved.Fov  = Cam->GetFovY();
            Saved.NearZ= Cam->GetNearZ();
            Saved.FarZ = Cam->GetFarZ();
            // Hide transform gizmo while in light POV
            GEditor->GetEditorModule()->SetGizmoVisible(false);
        }
        else if (!bViewFromLight && Saved.Has && Cam)
        {
            // Restore on disable
            Cam->SetCameraType(Saved.Type);
            Cam->SetLocation(Saved.Loc);
            Cam->SetRotation(Saved.Rot);
            Cam->SetFovY(Saved.Fov);
            Cam->SetNearZ(Saved.NearZ);
            Cam->SetFarZ(Saved.FarZ);
            Saved.Has = false;
            // Show gizmo again
            GEditor->GetEditorModule()->SetGizmoVisible(true);
        }
        bPrevViewFromLight = bViewFromLight;
    }

    // Failsafe: if override is off but we still have a saved camera and a valid camera appears, restore now
    if (!bViewFromLight && Saved.Has && Cam)
    {
        Cam->SetCameraType(Saved.Type);
        Cam->SetLocation(Saved.Loc);
        Cam->SetRotation(Saved.Rot);
        Cam->SetFovY(Saved.Fov);
        Cam->SetNearZ(Saved.NearZ);
        Cam->SetFarZ(Saved.FarZ);
        Saved.Has = false;
        GEditor->GetEditorModule()->SetGizmoVisible(true);
    }

    if (bViewFromLight && Cam)
    {
        // Follow light orientation every frame
        FVector dir = DirectionalLightComponent->GetForwardVector().GetNormalized();
        const float yawDeg = std::atan2f(dir.Y, dir.X) * ToDeg;
        const float pitchDeg = std::atan2f(dir.Z, std::sqrtf(dir.X * dir.X + dir.Y * dir.Y)) * ToDeg;
        Cam->SetCameraType(ECameraType::ECT_Perspective);
        Cam->SetRotation(FVector(0.0f, pitchDeg, yawDeg));
    }

    // Shadow Settings Section
    if (ImGui::CollapsingHeader("Shadow Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

        // Shadow Resolution Scale
        float ShadowResolutionScale = DirectionalLightComponent->GetShadowResolutionScale();
        if (ImGui::SliderFloat("Shadow Resolution Scale", &ShadowResolutionScale, 0.25f, 4.0f, "%.2f"))
        {
            DirectionalLightComponent->SetShadowwResolutionScale(ShadowResolutionScale);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(
                "Shadow resolution scale using 3-Tier system:\n\n"
                "Low Tier  (1024x1024): Scale 0.25 ~ 0.75\n"
                "Mid Tier  (2048x2048): Scale 0.76 ~ 1.5\n"
                "High Tier (4096x4096): Scale 1.51 ~ 4.0\n\n"
                "Directional light uses higher resolutions\n"
                "to cover larger scene areas."
            );
        }

        // Determine which tier this light would be in
        const char* tierName;
        int tierResolution;
        ImVec4 tierColor;

        if (ShadowResolutionScale <= 0.75f)
        {
            tierName = "Low Tier";
            tierResolution = 1024;
            tierColor = ImVec4(0.5f, 0.8f, 1.0f, 1.0f); // Light blue
        }
        else if (ShadowResolutionScale <= 1.5f)
        {
            tierName = "Mid Tier";
            tierResolution = 2048;
            tierColor = ImVec4(0.5f, 1.0f, 0.5f, 1.0f); // Light green
        }
        else
        {
            tierName = "High Tier";
            tierResolution = 4096;
            tierColor = ImVec4(1.0f, 0.8f, 0.3f, 1.0f); // Gold
        }

        // Display tier information
        ImGui::Text("Resolution Tier: ");
        ImGui::SameLine();
        ImGui::TextColored(tierColor, "%s", tierName);
        ImGui::Text("Actual Shadow Resolution: %d x %d", tierResolution, tierResolution);

        // Shadow Bias
        float ShadowBias = DirectionalLightComponent->GetShadowBias();
        if (ImGui::DragFloat("Shadow Bias", &ShadowBias, 0.0001f, 0.0f, 0.01f, "%.4f"))
        {
            DirectionalLightComponent->SetShadowBias(ShadowBias);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("섀도우 깊이 바이어스\n섀도우 아크네(그림자 얼룩) 방지\n기본값: 0.001");
        }

        // Shadow Slope Bias
        float ShadowSlopeBias = DirectionalLightComponent->GetShadowSlopeBias();
        if (ImGui::DragFloat("Shadow Slope Bias", &ShadowSlopeBias, 0.1f, 0.0f, 10.0f, "%.2f"))
        {
            DirectionalLightComponent->SetShadowSlopeBias(ShadowSlopeBias);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("섀도우 경사 바이어스\nPSM(Perspective Shadow Maps) 사용 시\n표면 각도에 따른 바이어스 조절");
        }

        // Shadow Sharpen
        float ShadowSharpen = DirectionalLightComponent->GetShadowSharpen();
        if (ImGui::DragFloat("Shadow Sharpen", &ShadowSharpen, 0.01f, 0.0f, 1.0f, "%.2f"))
        {
            DirectionalLightComponent->SetShadowSharpen(ShadowSharpen);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("섀도우 엣지 샤프닝\n그림자 경계를 더 선명하게");
        }

        ImGui::PopStyleColor(3);
    }

    ImGui::Separator();


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
