#pragma once

namespace Render
{
	// Screen Size
	constexpr int32 INIT_SCREEN_WIDTH = 1920;
	constexpr int32 INIT_SCREEN_HEIGHT = 1080;
}

namespace Time
{
	// Time Sample Count
	constexpr int32 FPS_SAMPLE_COUNT = 60;
}

constexpr float PI = 3.141592f;
constexpr uint64 KILO = 1024;
constexpr uint64 MEGA = 1024 * 1024;
constexpr float MATH_EPSILON = 1e-6f;
constexpr float ToRad = PI / 180.0f;
constexpr float ToDeg = 180.0f / PI;

constexpr float CameraSpeed = 6.0f;
constexpr float KeySensitivityDegPerPixel = 0.05f;

constexpr int32 MAX_DYNAMIC_PRIMITIVES_SIZE = 256;
