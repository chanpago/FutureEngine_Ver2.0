#include "pch.h"
#include "Render/UI/Widget/Public/PointLightComponentWidget.h"
#include "Component/Public/PointLightComponent.h"
#include "Editor/Public/Editor.h"
#include "Level/Public/Level.h"
#include "Component/Public/ActorComponent.h"
#include "ImGui/imgui.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Render/Renderer/Public/RenderResourceFactory.h"
#include "Render/RenderPass/Public/UpdateLightBufferPass.h"

IMPLEMENT_CLASS(UPointLightComponentWidget, UWidget)

void UPointLightComponentWidget::Initialize()
{
}

void UPointLightComponentWidget::Update()
{
    ULevel* CurrentLevel = GWorld->GetLevel();
    if (CurrentLevel)
    {
        UActorComponent* NewSelectedComponent = GEditor->GetEditorModule()->GetSelectedComponent();
        if (PointLightComponent != NewSelectedComponent)
        {
            PointLightComponent = Cast<UPointLightComponent>(NewSelectedComponent);
        }
    }
}

void UPointLightComponentWidget::RenderWidget()
{
    if (!PointLightComponent)
    {
        return;
    }

    ImGui::Separator();
    
    // 모든 입력 필드를 검은색으로 설정
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
	// 라이트 활성화 체크박스
    bool LightEnabled = PointLightComponent->GetLightEnabled();
    if (ImGui::Checkbox("Light Enabled", &LightEnabled))
    {
        PointLightComponent->SetLightEnabled(LightEnabled);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("라이트를 켜고 끕니다.\n끄면 조명 계산에서 제외됩니다.\n(Outliner O/X와는 별개)");
    }
    // Light Color
    FVector LightColor = PointLightComponent->GetLightColor();
    // Convert from 0-1 to 0-255 for display
    float LightColorRGB[3] = { LightColor.X * 255.0f, LightColor.Y * 255.0f, LightColor.Z * 255.0f };
    
    bool ColorChanged = false;
    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    
    float BoxWidth = 65.0f;  // Fixed width for each RGB box
    
    // R channel
    ImGui::SetNextItemWidth(BoxWidth);
    ImVec2 PosR = ImGui::GetCursorScreenPos();
    ColorChanged |= ImGui::DragFloat("##R", &LightColorRGB[0], 1.0f, 0.0f, 255.0f, "R: %.0f");
    ImVec2 SizeR = ImGui::GetItemRectSize();
    DrawList->AddLine(ImVec2(PosR.x + 5, PosR.y + 2), ImVec2(PosR.x + 5, PosR.y + SizeR.y - 2), IM_COL32(255, 0, 0, 255), 2.0f);
    
    ImGui::SameLine();
    
    // G channel
    ImGui::SetNextItemWidth(BoxWidth);
    ImVec2 PosG = ImGui::GetCursorScreenPos();
    ColorChanged |= ImGui::DragFloat("##G", &LightColorRGB[1], 1.0f, 0.0f, 255.0f, "G: %.0f");
    ImVec2 SizeG = ImGui::GetItemRectSize();
    DrawList->AddLine(ImVec2(PosG.x + 5, PosG.y + 2), ImVec2(PosG.x + 5, PosG.y + SizeG.y - 2), IM_COL32(0, 255, 0, 255), 2.0f);
    
    ImGui::SameLine();
    
    // B channel
    ImGui::SetNextItemWidth(BoxWidth);
    ImVec2 PosB = ImGui::GetCursorScreenPos();
    ColorChanged |= ImGui::DragFloat("##B", &LightColorRGB[2], 1.0f, 0.0f, 255.0f, "B: %.0f");
    ImVec2 SizeB = ImGui::GetItemRectSize();
    DrawList->AddLine(ImVec2(PosB.x + 5, PosB.y + 2), ImVec2(PosB.x + 5, PosB.y + SizeB.y - 2), IM_COL32(0, 0, 255, 255), 2.0f);
    
    ImGui::SameLine();
    
    // Color picker button
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
        // Convert back from 0-255 to 0-1
        LightColor.X = LightColorRGB[0] / 255.0f;
        LightColor.Y = LightColorRGB[1] / 255.0f;
        LightColor.Z = LightColorRGB[2] / 255.0f;
        PointLightComponent->SetLightColor(LightColor);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("라이트 필터 색입니다.\n이 색을 조절하면 실제 라이트의 강도가 조절되는 것과 같은 효과가 생까게 됩니다.");
    }

    // Intensity
    float Intensity = PointLightComponent->GetIntensity();
    if (ImGui::DragFloat("Intensity", &Intensity, 0.1f, 0.0f, 20.0f))
    {
        PointLightComponent->SetIntensity(Intensity);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("포인트 라이트 밝기\n범위: 0.0(꺼짐) ~ 20.0(최대)");
    }

    // Attenuation Radius
    float AttenuationRadius = PointLightComponent->GetAttenuationRadius();
    if (ImGui::DragFloat("Attenuation Radius", &AttenuationRadius, 0.1f, 0.0f, 1000.0f))
    {
        PointLightComponent->SetAttenuationRadius(AttenuationRadius);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("빛이 닿는 최대 거리입니다.\n이 반경에서 밝기는 0으로 떨어집니다.");
    }

    // Light Falloff Extent
    float DistanceFalloffExponent = PointLightComponent->GetDistanceFalloffExponent();
    if (ImGui::DragFloat("Distance Falloff Exponent", &DistanceFalloffExponent, 0.1f, 0.0f, 16.0f))
    {
        PointLightComponent->SetDistanceFalloffExponent(DistanceFalloffExponent);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("거리에 따라 밝기가 줄어드는 속도를 조절합니다.\n값이 클수록 감소가 더 급격합니다.");
    }

    ImGui::PopStyleColor(3);

    ImGui::Separator();

    // Shadow Settings Section
    if (ImGui::CollapsingHeader("Shadow Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

        // Shadow Resolution Scale
        float ShadowResolutionScale = PointLightComponent->GetShadowResolutionScale();
        if (ImGui::SliderFloat("Shadow Resolution Scale", &ShadowResolutionScale, 0.25f, 2.0f, "%.2f"))
        {
            PointLightComponent->SetShadowwResolutionScale(ShadowResolutionScale);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("섀도우맵 해상도 배율\n0.25 = 256x256, 0.5 = 512x512\n1.0 = 1024x1024 (기본)\n1.5 = 1536x1536, 2.0 = 2048x2048\n\n주의: 현재 구조상 1.0 이상은 클리핑 발생 가능!");
        }

        // Display current resolution
        const float baseResolution = 1024.0f;
        float currentResolution = baseResolution * ShadowResolutionScale;
        ImGui::Text("Current Shadow Resolution: %.0f x %.0f", currentResolution, currentResolution);

        // Warning if scale > 1.0
        if (ShadowResolutionScale > 1.0f)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Warning: Scale > 1.0 may cause clipping!");
            ImGui::TextWrapped("GPU texture is fixed at 1024x1024. Viewport larger than texture will be clipped.");
        }

        // Shadow Bias
        float ShadowBias = PointLightComponent->GetShadowBias();
        if (ImGui::DragFloat("Shadow Bias", &ShadowBias, 0.0001f, 0.0f, 0.01f, "%.4f"))
        {
            PointLightComponent->SetShadowBias(ShadowBias);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("섀도우 깊이 바이어스\n섀도우 아크네(그림자 얼룩) 방지\n기본값: 0.001");
        }

        // Shadow Slope Bias
        float ShadowSlopeBias = PointLightComponent->GetShadowSlopeBias();
        if (ImGui::DragFloat("Shadow Slope Bias", &ShadowSlopeBias, 0.1f, 0.0f, 10.0f, "%.2f"))
        {
            PointLightComponent->SetShadowSlopeBias(ShadowSlopeBias);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("섀도우 경사 바이어스\nPSM(Perspective Shadow Maps) 사용 시\n표면 각도에 따른 바이어스 조절");
        }

        // Shadow Sharpen
        float ShadowSharpen = PointLightComponent->GetShadowSharpen();
        if (ImGui::DragFloat("Shadow Sharpen", &ShadowSharpen, 0.01f, 0.0f, 1.0f, "%.2f"))
        {
            PointLightComponent->SetShadowSharpen(ShadowSharpen);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("섀도우 엣지 샤프닝\n그림자 경계를 더 선명하게");
        }

        ImGui::PopStyleColor(3);
    }

    ImGui::Separator();

    // Shadow Map Preview (Point Light Cube)
    if (ImGui::CollapsingHeader("Shadow Map View"))
    {
        auto& Renderer = URenderer::GetInstance();
        auto* DeviceResources = Renderer.GetDeviceResources();
        auto& Passes = Renderer.GetRenderPasses();
        FUpdateLightBufferPass* LightPass = Passes.empty() ? nullptr : dynamic_cast<FUpdateLightBufferPass*>(Passes[0]);

        if (!LightPass)
        {
            ImGui::TextColored(ImVec4(0.8f,0.5f,0.5f,1.0f), "Light buffer pass not found.");
        }
        else
        {
            uint32 CubeIdx = 0xFFFFFFFFu;
            if (!LightPass->GetPointCubeIndexCPU(PointLightComponent, CubeIdx))
            {
                ImGui::Text("No shadow baked for this point light (not shadowed or out of budget).");
            }
            else
            {
                // Controls
                static int Tile = 192; ImGui::SliderInt("Tile Size", &Tile, 64, 1024);
                static int Gap  = 8;   ImGui::SliderInt("Gap", &Gap, 0, 64);
                static float MinDepth = 0.0f; ImGui::SliderFloat("Min", &MinDepth, 0.0f, 0.99f, "%.2f");
                static float MaxDepth = 1.0f; ImGui::SliderFloat("Max", &MaxDepth, 0.01f, 1.0f, "%.2f");
                if (MinDepth > MaxDepth) MinDepth = MaxDepth;
                static float Gamma = 1.2f; ImGui::SliderFloat("Gamma", &Gamma, 0.2f, 3.0f, "%.2f");
                static bool Invert = false; ImGui::Checkbox("Invert", &Invert);

                // Prepare preview resources (mosaic 3x2)
                struct FDepthPreviewCB { float MinDepth; float MaxDepth; float Gamma; float Invert; };
                struct FPreviewResources
                {
                    ID3D11Texture2D* Tex = nullptr; ID3D11RenderTargetView* RTV = nullptr; ID3D11ShaderResourceView* SRV = nullptr;
                    ID3D11VertexShader* VS = nullptr; ID3D11PixelShader* PS = nullptr; ID3D11Buffer* CB = nullptr; ID3D11SamplerState* Sampler = nullptr;
                    int W = 0, H = 0;
                };
                static FPreviewResources R;

                auto Device = Renderer.GetDevice();
                auto DC     = Renderer.GetDeviceContext();

                const int Cols = 3, Rows = 2;
                const int Width = Cols * Tile + (Cols - 1) * Gap;
                const int Height = Rows * Tile + (Rows - 1) * Gap;

                auto DestroyPreview = [&]() {
                    SafeRelease(R.RTV); SafeRelease(R.SRV); SafeRelease(R.Tex);
                    SafeRelease(R.VS);  SafeRelease(R.PS);  SafeRelease(R.CB);
                    SafeRelease(R.Sampler);
                    R.W = R.H = 0;
                };

                if (R.W != Width || R.H != Height)
                {
                    DestroyPreview();
                    D3D11_TEXTURE2D_DESC desc = {}; desc.Width = Width; desc.Height = Height; desc.MipLevels = 1; desc.ArraySize = 1;
                    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; desc.SampleDesc.Count = 1; desc.Usage = D3D11_USAGE_DEFAULT;
                    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
                    if (SUCCEEDED(Device->CreateTexture2D(&desc, nullptr, &R.Tex)))
                    { Device->CreateRenderTargetView(R.Tex, nullptr, &R.RTV); Device->CreateShaderResourceView(R.Tex, nullptr, &R.SRV); R.W = Width; R.H = Height; }
                }

                if (!R.VS || !R.PS)
                { FRenderResourceFactory::CreateVertexShaderAndInputLayout(L"Asset/Shader/DepthPreview.hlsl", {}, &R.VS, nullptr, "DepthPreviewVS");
                  FRenderResourceFactory::CreatePixelShader(L"Asset/Shader/DepthPreview.hlsl", &R.PS, "DepthPreviewPS"); }
                if (!R.CB)
                { R.CB = FRenderResourceFactory::CreateConstantBuffer<FDepthPreviewCB>(); }
                if (!R.Sampler)
                { R.Sampler = FRenderResourceFactory::CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP); }

                if (R.RTV && R.VS && R.PS && R.CB && R.Sampler)
                {
                    FDepthPreviewCB cb = { MinDepth, MaxDepth, Gamma, Invert ? 1.0f : 0.0f };
                    FRenderResourceFactory::UpdateConstantBufferData(R.CB, cb);

                    float clear[4] = {0,0,0,1};
                    // Save old targets and viewport
                    ID3D11RenderTargetView* oldRTV = nullptr; ID3D11DepthStencilView* oldDSV = nullptr; DC->OMGetRenderTargets(1, &oldRTV, &oldDSV);
                    UINT oldVPCount = 1; D3D11_VIEWPORT oldVP{}; DC->RSGetViewports(&oldVPCount, &oldVP);

                    // Set destination RT & clear
                    DC->OMSetRenderTargets(1, &R.RTV, nullptr);
                    DC->ClearRenderTargetView(R.RTV, clear);
                    DC->IASetInputLayout(nullptr);
                    DC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                    DC->VSSetShader(R.VS, nullptr, 0);
                    DC->PSSetShader(R.PS, nullptr, 0);
                    DC->PSSetSamplers(0, 1, &R.Sampler);
                    DC->PSSetConstantBuffers(0, 1, &R.CB);

                    // Render 6 faces into a 3x2 mosaic
                    for (int face = 0; face < 6; ++face)
                    {
                        ID3D11ShaderResourceView* FaceSRV = nullptr;
                        if (!DeviceResources->CreatePointShadowFaceSRV(CubeIdx, (UINT)face, &FaceSRV) || !FaceSRV)
                            continue;

                        const int cx = face % Cols;
                        const int cy = face / Cols;
                        const float x0 = (float)(cx * (Tile + Gap));
                        const float y0 = (float)(cy * (Tile + Gap));
                        D3D11_VIEWPORT vpTile = { x0, y0, (float)Tile, (float)Tile, 0.0f, 1.0f };
                        DC->RSSetViewports(1, &vpTile);

                        DC->PSSetShaderResources(0, 1, &FaceSRV);
                        DC->Draw(3, 0);
                        ID3D11ShaderResourceView* nullSRV[1] = { nullptr }; DC->PSSetShaderResources(0, 1, nullSRV);
                        SafeRelease(FaceSRV);
                    }

                    // Restore old targets and viewport
                    DC->OMSetRenderTargets(1, &oldRTV, oldDSV);
                    DC->RSSetViewports(oldVPCount, &oldVP);
                    SafeRelease(oldRTV); SafeRelease(oldDSV);
                }

                // Show the processed mosaic preview
                ImGui::Image((ImTextureID)R.SRV, ImVec2((float)R.W, (float)R.H));
                ImGui::Text("Cube %u (faces 0..5)", CubeIdx);
            }
        }
    }

}

