#pragma once
#include "Render/UI/Layout/Public/Splitter.h"

// Vertical splitter: Left(LT) / Right(RB)
class SSplitterV : public SSplitter
{
public:
    SSplitterV() { Orientation = EOrientation::Vertical; }
};
