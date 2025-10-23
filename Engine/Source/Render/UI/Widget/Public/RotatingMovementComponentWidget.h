#pragma once
#include "Render/UI/Widget/Public/Widget.h"

class URotatingMovementComponentWidget : public UWidget
{
    DECLARE_CLASS(URotatingMovementComponentWidget, UWidget)

public:
    void Initialize() override {}
    void Update() override {}
    void RenderWidget() override;
};
