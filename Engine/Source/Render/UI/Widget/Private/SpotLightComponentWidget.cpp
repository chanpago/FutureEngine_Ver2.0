#include "pch.h"
#include "Render/UI/Widget/Public/SpotLightComponentWidget.h"
#include "Component/Public/SpotLightComponent.h"
#include "Editor/Public/Editor.h"
#include "Level/Public/Level.h"
#include "Component/Public/ActorComponent.h"
#include "ImGui/imgui.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Render/Renderer/Public/RenderResourceFactory.h"
#include "Manager/UI/Public/ViewportManager.h"
#include "Editor/Public/Camera.h"
#include "Editor/Public/Editor.h"
#include "Render/UI/Viewport/Public/ViewportClient.h"

IMPLEMENT_CLASS(USpotLightComponentWidget, UWidget)

void USpotLightComponentWidget::Initialize()
{
}

void USpotLightComponentWidget::Update()
{
    ULevel* CurrentLevel = GWorld->GetLevel();
    if (CurrentLevel)
    {
        UActorComponent* NewSelectedComponent = GEditor->GetEditorModule()->GetSelectedComponent();
        if (SpotLightComponent != NewSelectedComponent)
        {
            SpotLightComponent = Cast<USpotLightComponent>(NewSelectedComponent);
        }
    }
}

void USpotLightComponentWidget::RenderWidget()
{
    if (!SpotLightComponent)
    {
        return;
    }

    ImGui::Separator();
    
    // 모든 입력 필드를 검은색으로 설정
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
	// 라이트 활성화 체크박스
    bool LightEnabled = SpotLightComponent->GetLightEnabled();
    if (ImGui::Checkbox("Light Enabled", &LightEnabled))
    {
        SpotLightComponent->SetLightEnabled(LightEnabled);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("라이트를 켜고 끕니다.\n끄면 조명 계산에서 제외됩니다.\n(Outliner O/X와는 별개)");
    }
    // Light Color
    FVector LightColor = SpotLightComponent->GetLightColor();
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
        SpotLightComponent->SetLightColor(LightColor);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("라이트 필터 색입니다.\n이 색을 조절하면 실제 라이트의 강도가 조절되는 것과 같은 효과가 생기게 됩니다.");
    }

    // Intensity
    float Intensity = SpotLightComponent->GetIntensity();
    if (ImGui::DragFloat("Intensity", &Intensity, 0.1f, 0.0f, 20.0f))
    {
        SpotLightComponent->SetIntensity(Intensity);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("스포트라이트 밝기\n범위: 0.0(꺼짐) ~ 20.0(최대)");
    }

    // Distance Falloff Exponent
    float DistanceFalloffExponent = SpotLightComponent->GetDistanceFalloffExponent();
    if (ImGui::DragFloat("Distance Falloff Exponent", &DistanceFalloffExponent, 0.1f, 2.0f, 16.0f))
    {
        SpotLightComponent->SetDistanceFalloffExponent(DistanceFalloffExponent);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("거리에 따라 밝기가 줄어드는 속도를 조절합니다.\n값이 클수록 감소가 더 급격합니다.");
    }

    // Angle Falloff Exponent
    float AngleFalloffExponent = SpotLightComponent->GetAngleFalloffExponent();
    if (ImGui::DragFloat("Angle Falloff Exponent", &AngleFalloffExponent, 0.5f, 1.0f, 128.0f))
    {
        SpotLightComponent->SetAngleFalloffExponent(AngleFalloffExponent);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("내부 원뿔에서 외부 원뿔로 밝기가 바뀌는 부드러움을 조절합니다.\n값이 클수록 가장자리가 더 또렷해집니다.");
    }

    // Attenuation Radius
    float AttenuationRadius = SpotLightComponent->GetAttenuationRadius();
    if (ImGui::DragFloat("Attenuation Radius", &AttenuationRadius, 0.1f, 0.0f, 1000.0f))
    {
        SpotLightComponent->SetAttenuationRadius(AttenuationRadius);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("빛이 닿는 최대 거리입니다.\n이 반경에서 밝기는 0으로 떨어집니다.");
    }

    // Outer Cone Angle
    float OuterAngleDegrees = SpotLightComponent->GetOuterConeAngle() * ToDeg;
    if (ImGui::DragFloat("Outer Cone Angle (deg)", &OuterAngleDegrees, 1.0f, 0.0f, 89.0f))
    {
        SpotLightComponent->SetOuterAngle(OuterAngleDegrees * ToRad);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("스포트라이트 원뿔의 바깥쪽 가장자리 각도입니다.\n이 각도 바깥은 완전히 어둡습니다.");
    }

    // Inner Cone Angle
    float InnerAngleDegrees = SpotLightComponent->GetInnerConeAngle() * ToDeg;
    if (ImGui::DragFloat("Inner Cone Angle (deg)", &InnerAngleDegrees, 1.0f, 0.0f, OuterAngleDegrees))
    {
        SpotLightComponent->SetInnerAngle(InnerAngleDegrees * ToRad);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("스포트라이트 원뿔의 안쪽 가장자리 각도입니다.\n이 각도 안쪽은 최대 밝기입니다.");
    }
    
    // Shadow parameters
    float depthBias = SpotLightComponent->GetBias();
    if (ImGui::DragFloat("Shadow Bias", &depthBias, 0.0001f, 0.0f, 0.02f, "%.5f"))
    {
        SpotLightComponent->SetBias(depthBias);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("그림자 깊이 바이어스(아크네 방지)");
    }

    float slopeBias = SpotLightComponent->GetSlopeBias();
    if (ImGui::DragFloat("Slope Bias", &slopeBias, 0.0005f, 0.0f, 0.2f, "%.5f"))
    {
        SpotLightComponent->SetSlopeBias(slopeBias);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("경사(법선) 기반 추가 바이어스");
    }

    float sharpen = SpotLightComponent->GetSharpen();
    if (ImGui::DragFloat("Sharpen", &sharpen, 0.05f, 0.0f, 3.0f, "%.2f"))
    {
        SpotLightComponent->SetSharpen(sharpen);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("그림자 경계 선명도");
    }

    ImGui::PopStyleColor(3);
    
    // Cast Shadows (PSM) Checkbox
    ImGui::Separator();
    bool bCastShadows = SpotLightComponent->GetCastShadows();
    if (ImGui::Checkbox("Cast Shadows (PSM)", &bCastShadows))
    {
        SpotLightComponent->SetCastShadows(bCastShadows);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("그림자 활성화 (논문 기반 PSM 사용)\nON: PSM (Perspective Shadow Maps) - 카메라 post-perspective 공간에서 생성\nOFF: Standard Shadow - 일반 spotlight shadow");
    }

    // Override camera with light's perspective (position + orientation + FOV) + auto-restore
    static bool bViewFromSpot = false;
    static bool bPrevViewFromSpot = false;
    struct FSavedCam { bool Has=false; ECameraType Type; FVector Loc; FVector Rot; float Fov; float NearZ; float FarZ; };
    static FSavedCam Saved{};
    bool toggledFollow = ImGui::Checkbox("View From Light (Camera)", &bViewFromSpot);
    UViewportManager& VM2 = UViewportManager::GetInstance();
    int32 active2 = VM2.GetActiveIndex();
    auto& clients2 = VM2.GetClients();
    UCamera* Cam2 = (active2 >= 0 && active2 < (int)clients2.size() && clients2[active2]) ? clients2[active2]->GetCamera() : nullptr;

    if (toggledFollow)
    {
        if (bViewFromSpot && Cam2)
        {
            Saved.Has = true;
            Saved.Type = Cam2->GetCameraType();
            Saved.Loc  = Cam2->GetLocation();
            Saved.Rot  = Cam2->GetRotation();
            Saved.Fov  = Cam2->GetFovY();
            Saved.NearZ= Cam2->GetNearZ();
            Saved.FarZ = Cam2->GetFarZ();
            GEditor->GetEditorModule()->SetGizmoVisible(false);
        }
        else if (!bViewFromSpot && Saved.Has && Cam2)
        {
            Cam2->SetCameraType(Saved.Type);
            Cam2->SetLocation(Saved.Loc);
            Cam2->SetRotation(Saved.Rot);
            Cam2->SetFovY(Saved.Fov);
            Cam2->SetNearZ(Saved.NearZ);
            Cam2->SetFarZ(Saved.FarZ);
            Saved.Has = false;
            GEditor->GetEditorModule()->SetGizmoVisible(true);
        }
        bPrevViewFromSpot = bViewFromSpot;
    }

    // Failsafe: if override is off but we still have a saved camera and a valid camera appears, restore now
    if (!bViewFromSpot && Saved.Has && Cam2)
    {
        Cam2->SetCameraType(Saved.Type);
        Cam2->SetLocation(Saved.Loc);
        Cam2->SetRotation(Saved.Rot);
        Cam2->SetFovY(Saved.Fov);
        Cam2->SetNearZ(Saved.NearZ);
        Cam2->SetFarZ(Saved.FarZ);
        Saved.Has = false;
        GEditor->GetEditorModule()->SetGizmoVisible(true);
    }

    if (bViewFromSpot && Cam2)
    {
        // Derive yaw/pitch from the light's forward vector to avoid basis/euler mismatches
        const FVector eye = SpotLightComponent->GetWorldLocation();
        const FVector dir = SpotLightComponent->GetForwardVector().GetNormalized();
        const float yawDeg = std::atan2f(dir.Y, dir.X) * ToDeg;
        // Camera’s pitch sign is opposite of our derived world-space pitch; invert to avoid upside-down view
        const float pitchDeg = -std::atan2f(dir.Z, std::sqrtf(dir.X * dir.X + dir.Y * dir.Y)) * ToDeg;

        Cam2->SetCameraType(ECameraType::ECT_Perspective);
        Cam2->SetLocation(eye);
        Cam2->SetRotation(FVector(0.0f, pitchDeg, yawDeg));
        const float fovDeg = SpotLightComponent->GetOuterConeAngle() * 2.0f * ToDeg;
        Cam2->SetFovY(fovDeg);
        Cam2->SetNearZ(0.05f);
        Cam2->SetFarZ(std::max(SpotLightComponent->GetAttenuationRadius(), 10.0f));
    }

    // Shadow Map Preview
    if (ImGui::CollapsingHeader("Shadow Map Preview"))
    {
        ID3D11ShaderResourceView* ShadowSRV = URenderer::GetInstance().GetDeviceResources()->GetSpotShadowMapSRV();
        if (ShadowSRV)
        {
            // Controls
            static int Size = 256;
            ImGui::SliderInt("Size", &Size, 64, 1024);
            static float MinDepth = 0.99f;
            static float MaxDepth = 1.0f;
            static float Gamma    = 1.2f;
            static bool  Invert   = false;
            ImGui::SliderFloat("Min", &MinDepth, 0.0f, 0.99f, "%.2f");
            ImGui::SliderFloat("Max", &MaxDepth, 0.01f, 1.0f, "%.2f");
            if (MinDepth > MaxDepth) MinDepth = MaxDepth;
            ImGui::SliderFloat("Gamma", &Gamma, 0.2f, 3.0f, "%.2f");
            ImGui::Checkbox("Invert", &Invert);

            // Lazy-create preview resources
            struct FDepthPreviewCB { float MinDepth; float MaxDepth; float Gamma; float Invert; };
            struct FPreviewResources
            {
                ID3D11Texture2D* Tex = nullptr;
                ID3D11RenderTargetView* RTV = nullptr;
                ID3D11ShaderResourceView* SRV = nullptr;
                ID3D11VertexShader* VS = nullptr;
                ID3D11PixelShader* PS = nullptr;
                ID3D11Buffer* CB = nullptr;
                ID3D11SamplerState* Sampler = nullptr;
                int W = 0, H = 0;
            };
            static FPreviewResources R;

            auto Device = URenderer::GetInstance().GetDevice();
            auto DC     = URenderer::GetInstance().GetDeviceContext();

            auto DestroyPreview = [&]() {
                SafeRelease(R.RTV); SafeRelease(R.SRV); SafeRelease(R.Tex);
                SafeRelease(R.VS);  SafeRelease(R.PS);  SafeRelease(R.CB);
                SafeRelease(R.Sampler);
                R.W = R.H = 0;
            };

            // Recreate texture if size changed
            if (R.W != Size || R.H != Size)
            {
                DestroyPreview();
                D3D11_TEXTURE2D_DESC desc = {};
                desc.Width = Size; desc.Height = Size;
                desc.MipLevels = 1; desc.ArraySize = 1;
                desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                desc.SampleDesc.Count = 1;
                desc.Usage = D3D11_USAGE_DEFAULT;
                desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
                if (SUCCEEDED(Device->CreateTexture2D(&desc, nullptr, &R.Tex)))
                {
                    Device->CreateRenderTargetView(R.Tex, nullptr, &R.RTV);
                    Device->CreateShaderResourceView(R.Tex, nullptr, &R.SRV);
                    R.W = R.H = Size;
                }
            }

            // Create shaders/CB/sampler once
            if (!R.VS || !R.PS)
            {
                FRenderResourceFactory::CreateVertexShaderAndInputLayout(L"Asset/Shader/DepthPreview.hlsl", {}, &R.VS, nullptr, "DepthPreviewVS");
                FRenderResourceFactory::CreatePixelShader(L"Asset/Shader/DepthPreview.hlsl", &R.PS, "DepthPreviewPS");
            }
            if (!R.CB)
            {
                R.CB = FRenderResourceFactory::CreateConstantBuffer<FDepthPreviewCB>();
            }
            if (!R.Sampler)
            {
                R.Sampler = FRenderResourceFactory::CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP);
            }

            // Render to preview texture
            if (R.RTV && R.VS && R.PS && R.CB && R.Sampler)
            {
                // Update constants
                FDepthPreviewCB cb = { MinDepth, MaxDepth, Gamma, Invert ? 1.0f : 0.0f };
                FRenderResourceFactory::UpdateConstantBufferData(R.CB, cb);

                // Set RT
                float clear[4] = {0,0,0,1};
                // Save old targets and viewport
                ID3D11RenderTargetView* oldRTV = nullptr;
                ID3D11DepthStencilView* oldDSV = nullptr;
                DC->OMGetRenderTargets(1, &oldRTV, &oldDSV);
                UINT oldVPCount = 1; D3D11_VIEWPORT oldVP{}; DC->RSGetViewports(&oldVPCount, &oldVP);

                DC->OMSetRenderTargets(1, &R.RTV, nullptr);
                DC->ClearRenderTargetView(R.RTV, clear);

                // Set viewport
                D3D11_VIEWPORT vp = { 0, 0, (float)R.W, (float)R.H, 0.0f, 1.0f };
                DC->RSSetViewports(1, &vp);

                // Bind pipeline
                DC->IASetInputLayout(nullptr);
                DC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                DC->VSSetShader(R.VS, nullptr, 0);
                DC->PSSetShader(R.PS, nullptr, 0);
                DC->PSSetSamplers(0, 1, &R.Sampler);
                DC->PSSetShaderResources(0, 1, &ShadowSRV);
                DC->PSSetConstantBuffers(0, 1, &R.CB);

                // Draw fullscreen triangle
                DC->Draw(3, 0);

                // Unbind SRV from PS slot 0 to avoid warnings
                ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
                DC->PSSetShaderResources(0, 1, nullSRV);

                // Restore old targets and viewport
                DC->OMSetRenderTargets(1, &oldRTV, oldDSV);
                DC->RSSetViewports(oldVPCount, &oldVP);
                SafeRelease(oldRTV);
                SafeRelease(oldDSV);
            }

            // Show the processed preview
            ImGui::Image((ImTextureID)R.SRV, ImVec2((float)Size, (float)Size));
            ImGui::Text("Resolution: 1024 x 1024 (Depth)" );
        }
        else
        {
            ImGui::TextColored(ImVec4(0.8f,0.5f,0.5f,1.0f), "Shadow map SRV not available.");
        }
    }

    ImGui::Separator();
}
