#include "pch.h"
#include "Render/UI/Widget/Public/ProjectileMovementComponentWidget.h"

IMPLEMENT_CLASS(UProjectileMovementComponentWidget, UWidget)

void UProjectileMovementComponentWidget::RenderWidget()
{
    ULevel* CurrentLevel = GWorld->GetLevel();

    if (!CurrentLevel)
    {
        ImGui::TextUnformatted("No Level Loaded");
        return;
    }

    UActorComponent* Component = GEditor->GetEditorModule()->GetSelectedComponent();
    if (!Component)
    {
        ImGui::TextUnformatted("No Object Selected");
        return;
    }

    UProjectileMovementComponent* ProjectileMovementComponent = Cast<UProjectileMovementComponent>(Component);
    if (!ProjectileMovementComponent) { return; }

    FVector Velocity = ProjectileMovementComponent->GetVelocity();
    ImGui::DragFloat3("Velocity", &Velocity.X);
    ProjectileMovementComponent->SetVelocity(Velocity);
    
    float MaxSpeed = ProjectileMovementComponent->GetMaxSpeed();
    ImGui::DragFloat("Max Speed", &MaxSpeed);
    ProjectileMovementComponent->SetMaxSpeed(MaxSpeed);
    
    float InitSpeed = ProjectileMovementComponent->GetInitialSpeed();
    ImGui::DragFloat("Init Speed", &InitSpeed);
    ProjectileMovementComponent->SetInitialSpeed(InitSpeed);
    
    float GravityScale = ProjectileMovementComponent->GetGravityScale();
    ImGui::DragFloat("Gravity Scale", &GravityScale);
    ProjectileMovementComponent->SetGravityScale(GravityScale);
    
    bool bRotationFollowsVelocity = ProjectileMovementComponent->GetRotationFollowsVelocity();
    ImGui::Checkbox("Rotation Follows Velocity", &bRotationFollowsVelocity);
    ProjectileMovementComponent->SetRotationFollowsVelocity(bRotationFollowsVelocity);
}
