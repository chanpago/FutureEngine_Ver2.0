#include "pch.h"
#include "Editor/Public/Camera.h"
#include "Manager/Input/Public/InputManager.h"
#include "Manager/Time/Public/TimeManager.h"
#include "Manager/Config/Public/ConfigManager.h"

#include "Component/Public/PrimitiveComponent.h"
#include "Level/Public/Level.h"

UCamera::UCamera() :
	CameraConstants(FCameraConstants()),
	RelativeLocation(FVector(-15.0f, 0.f, 10.0f)), RelativeRotation(FVector(0, 0, 0)),
	FovY(90.f), Aspect(float(Render::INIT_SCREEN_WIDTH) / Render::INIT_SCREEN_HEIGHT),
	NearZ(0.1f), FarZ(1000.0f), OrthoWidth(90.0f), CameraType(ECameraType::ECT_Perspective)
{
	CurrentMoveSpeed = UConfigManager::GetInstance().GetCameraSensitivity();
}

UCamera::~UCamera()
{
	UConfigManager::GetInstance().SetCameraSensitivity(CurrentMoveSpeed);
}

FVector UCamera::UpdateInput()
{
	FVector MovementDelta = FVector::Zero(); // 마우스의 변화량을 반환할 객체
	const UInputManager& Input = UInputManager::GetInstance();

	/**
	 * @brief 마우스 우클릭을 하고 있는 동안 카메라 제어가 가능합니다.
	 */
	if (Input.IsKeyDown(EKeyInput::MouseRight))
	{
		/**
		 * @brief W, A, S, D 는 각각 카메라의 상, 하, 좌, 우 이동을 담당합니다.
		 */
		FVector Direction = FVector::Zero();

		if (Input.IsKeyDown(EKeyInput::A)) { Direction += -Right * 2; }
		if (Input.IsKeyDown(EKeyInput::D)) { Direction += Right * 2; }
		if (Input.IsKeyDown(EKeyInput::W)) { Direction += Forward * 2; }
		if (Input.IsKeyDown(EKeyInput::S)) { Direction += -Forward * 2; }
		if (Input.IsKeyDown(EKeyInput::Q)) { Direction += -Up * 2; }
		if (Input.IsKeyDown(EKeyInput::E)) { Direction += Up * 2; }
		if (Direction.LengthSquared() > MATH_EPSILON)
		{
			Direction.Normalize();
		}
		RelativeLocation += Direction * CurrentMoveSpeed * DT;
		MovementDelta = Direction * CurrentMoveSpeed * DT;

		// 오른쪽 마우스 버튼 + 마우스 휠로 카메라 이동속도 조절
		float WheelDelta = Input.GetMouseWheelDelta();
		if (WheelDelta != 0.0f)
		{
			// 휠 위로 돌리면 속도 증가, 아래로 돌리면 속도 감소
			AdjustMoveSpeed(WheelDelta * SPEED_ADJUST_STEP);
		}

		/**
		* @brief 마우스 위치 변화량을 감지하여 카메라의 회전을 담당합니다.
		* 원근 투영 모드를 적용한 카메라만 회전이 가능합니다.
		*/
		if (CameraType == ECameraType::ECT_Perspective)
		{
			const FVector MouseDelta = UInputManager::GetInstance().GetMouseDelta();
			RelativeRotation.Z += MouseDelta.X * KeySensitivityDegPerPixel * 2;
			RelativeRotation.Y += MouseDelta.Y * KeySensitivityDegPerPixel * 2;
			MovementDelta = FVector::Zero(); // 원근 투영 모드는 반환할 필요가 없음
		}


		// Yaw 래핑(값이 무한히 커지지 않도록)
		if (RelativeRotation.Z > 180.0f) RelativeRotation.Z -= 360.0f;
		if (RelativeRotation.Z < -180.0f) RelativeRotation.Z += 360.0f;

		// Pitch 클램프(짐벌 플립 방지)
		if (RelativeRotation.Y > 89.0f)  RelativeRotation.Y = 89.0f;
		if (RelativeRotation.Y < -89.0f) RelativeRotation.Y = -89.0f;
	}

	return MovementDelta;
}

void UCamera::Update(const D3D11_VIEWPORT& InViewport)
{
	// 입력이 활성화되어 있으면 입력 처리
	
	if (bInputEnabled)
	{
		UpdateInput();
	}
	
	const FMatrix RotationMatrix = FMatrix::RotationMatrix(FVector::GetDegreeToRadian(RelativeRotation));
	const FVector4 Forward4 = FVector4::ForwardVector() * RotationMatrix;
	const FVector4 WorldUp4 = FVector4::UpVector() * RotationMatrix;
	const FVector WorldUp = { WorldUp4.X, WorldUp4.Y, WorldUp4.Z };

	Forward = FVector(Forward4.X, Forward4.Y, Forward4.Z);
	Forward.Normalize();
	Right = WorldUp.Cross(Forward);
	Right.Normalize();
	Up = Forward.Cross(Right);
	Up.Normalize();

	// 종횡비 갱신
	if (InViewport.Width > 0.f && InViewport.Height > 0.f)
	{
		SetAspect(InViewport.Width / InViewport.Height);
	}

	switch (CameraType)
	{
	case ECameraType::ECT_Perspective:
		UpdateMatrixByPers();
		break;
	case ECameraType::ECT_Orthographic:
		UpdateMatrixByOrth();
		break;
	}

	// 카메라가 업데이트할 때마다 Cull한다.
	// 카메라가 업데이트하지 않으면 Culling을 갱신할 이유가 없다.
    ULevel* CurrentLevel = GWorld->GetLevel();
    if (CurrentLevel)
    {
        ViewVolumeCuller.Cull(
            CurrentLevel->GetStaticOctree(),
            CurrentLevel->GetDynamicPrimitives(),
            CameraConstants
        );
    }
}

void UCamera::UpdateMatrixByPers()
{
	/**
	 * @brief View 행렬 연산
	 */
	FMatrix T = FMatrix::TranslationMatrixInverse(RelativeLocation);
	FMatrix R = FMatrix(Right, Up, Forward);
	R = R.Transpose();
	CameraConstants.View = T * R;

	/**
	 * @brief Projection 행렬 연산
	 * 원근 투영 행렬 (HLSL에서 row-major로 mul(p, M) 일관성 유지)
	 * f = 1 / tan(fovY/2)
	 */
	const float RadianFovY = FVector::GetDegreeToRadian(FovY);
	const float F = 1.0f / std::tanf(RadianFovY * 0.5f);

	FMatrix P = FMatrix::Identity();
	// | f/aspect   0        0         0 |
	// |    0       f        0         0 |
	// |    0       0   zf/(zf-zn)     1 |
	// |    0       0  -zn*zf/(zf-zn)  0 |
	P.Data[0][0] = F / Aspect;
	P.Data[1][1] = F;
	P.Data[2][2] = FarZ / (FarZ - NearZ);
	P.Data[2][3] = 1.0f;
	P.Data[3][2] = (-NearZ * FarZ) / (FarZ - NearZ);
	P.Data[3][3] = 0.0f;

	CameraConstants.Projection = P;
	
	CameraConstants.ViewWorldLocation = RelativeLocation;
	CameraConstants.NearClip = NearZ;
	CameraConstants.FarClip = FarZ;
}

void UCamera::UpdateMatrixByOrth()
{
	/**
	 * @brief View 행렬 연산
	 */
	FMatrix T = FMatrix::TranslationMatrixInverse(RelativeLocation);
	FMatrix R = FMatrix(Right, Up, Forward);
	R = R.Transpose();
	CameraConstants.View = T * R;
	/*FMatrix T = FMatrix::TranslationMatrixInverse(RelativeLocation);
	FMatrix R = FMatrix::RotationMatrixInverse(FVector::GetDegreeToRadian(RelativeRotation));
	ViewProjConstants.View = T * R;*/

	/**
	 * @brief Projection 행렬 연산
	 */
	const float OrthoHeight = OrthoWidth / Aspect;
	const float Left = -OrthoWidth * 0.5f;
	const float Right = OrthoWidth * 0.5f;
	const float Bottom = -OrthoHeight * 0.5f;
	const float Top = OrthoHeight * 0.5f;

	FMatrix P = FMatrix::Identity();
	P.Data[0][0] = 2.0f / (Right - Left);
	P.Data[1][1] = 2.0f / (Top - Bottom);
	P.Data[2][2] = 1.0f / (FarZ - NearZ);
	P.Data[3][0] = -(Right + Left) / (Right - Left);
	P.Data[3][1] = -(Top + Bottom) / (Top - Bottom);
	P.Data[3][2] = -NearZ / (FarZ - NearZ);
	P.Data[3][3] = 1.0f;
	CameraConstants.Projection = P;
	
	CameraConstants.ViewWorldLocation = RelativeLocation;
	CameraConstants.NearClip = NearZ;
	CameraConstants.FarClip = FarZ;
}

const FCameraConstants UCamera::GetFViewProjConstantsInverse() const
{
	/*
	* @brief View^(-1) = R * T
	*/
	FCameraConstants Result = {};
	//FMatrix R = FMatrix::RotationMatrix(FVector::GetDegreeToRadian(RelativeRotation));
	FMatrix R = FMatrix(Right, Up, Forward);
	FMatrix T = FMatrix::TranslationMatrix(RelativeLocation);
	Result.View = R * T;

	if (CameraType == ECameraType::ECT_Orthographic)
	{
		const float OrthoHeight = OrthoWidth / Aspect;
		const float Left = -OrthoWidth * 0.5f;
		const float Right = OrthoWidth * 0.5f;
		const float Bottom = -OrthoHeight * 0.5f;
		const float Top = OrthoHeight * 0.5f;

		FMatrix P = FMatrix::Identity();
		// A^{-1} (대각)
		P.Data[0][0] = (Right - Left) * 0.5f;  // (r-l)/2
		P.Data[1][1] = (Top - Bottom) * 0.5f; // (t-b)/2
		P.Data[2][2] = (FarZ - NearZ);               // (zf-zn)
		// -b A^{-1} (마지막 행의 x,y,z)
		P.Data[3][0] = (Right + Left) * 0.5f;   // (r+l)/2
		P.Data[3][1] = (Top + Bottom) * 0.5f; // (t+b)/2
		P.Data[3][2] = NearZ;                      // zn
		P.Data[3][3] = 1.0f;
		Result.Projection = P;
	}
	else if ((CameraType == ECameraType::ECT_Perspective))
	{
		const float FovRadian = FVector::GetDegreeToRadian(FovY);
		const float F = 1.0f / std::tanf(FovRadian * 0.5f);
		FMatrix P = FMatrix::Identity();
		// | aspect/F   0      0         0 |
		// |    0      1/F     0         0 |
		// |    0       0      0   -(zf-zn)/(zn*zf) |
		// |    0       0      1        zf/(zn*zf)  |
		P.Data[0][0] = Aspect / F;
		P.Data[1][1] = 1.0f / F;
		P.Data[2][2] = 0.0f;
		P.Data[2][3] = -(FarZ - NearZ) / (NearZ * FarZ);
		P.Data[3][2] = 1.0f;
		P.Data[3][3] = FarZ / (NearZ * FarZ);
		Result.Projection = P;
	}

	Result.ViewWorldLocation = RelativeLocation;
	Result.NearClip = NearZ;
	Result.FarClip = FarZ;
	return Result;
}

FRay UCamera::ConvertToWorldRay(float NdcX, float NdcY) const
{
	/* *
	 * @brief 반환할 타입의 객체 선언
	 */
	FRay Ray = {};

	const FCameraConstants& ViewProjMatrix = GetFViewProjConstantsInverse();

	/* *
	 * @brief NDC 좌표 정보를 행렬로 변환합니다.
	 */
	const FVector4 NdcNear(NdcX, NdcY, 0.0f, 1.0f);
	const FVector4 NdcFar(NdcX, NdcY, 1.0f, 1.0f);

	/* *
	 * @brief Projection 행렬을 View 행렬로 역투영합니다.
	 * Model -> View -> Projection -> NDC
	 */
	const FVector4 ViewNear = MultiplyPointWithMatrix(NdcNear, ViewProjMatrix.Projection);
	const FVector4 ViewFar = MultiplyPointWithMatrix(NdcFar, ViewProjMatrix.Projection);

	/* *
	 * @brief View 행렬을 World 행렬로 역투영합니다.
	 * Model -> View -> Projection -> NDC
	 */
	const FVector4 WorldNear = MultiplyPointWithMatrix(ViewNear, ViewProjMatrix.View);
	const FVector4 WorldFar = MultiplyPointWithMatrix(ViewFar, ViewProjMatrix.View);

	/* *
	 * @brief 카메라의 월드 좌표를 추출합니다.
	 * Row-major 기준, 마지막 행 벡터는 위치 정보를 가지고 있음
	 */
	const FVector4 CameraPosition(
		ViewProjMatrix.View.Data[3][0],
		ViewProjMatrix.View.Data[3][1],
		ViewProjMatrix.View.Data[3][2],
		ViewProjMatrix.View.Data[3][3]);

	if (CameraType == ECameraType::ECT_Perspective)
	{
		FVector4 DirectionVector = WorldFar - CameraPosition;
		DirectionVector.Normalize();

		Ray.Origin = CameraPosition;
		Ray.Direction = DirectionVector;
	}
	else if (CameraType == ECameraType::ECT_Orthographic)
	{
		FVector4 DirectionVector = WorldFar - WorldNear;
		DirectionVector.Normalize();

		Ray.Origin = WorldNear;
		Ray.Direction = DirectionVector;
	}

	return Ray;
}

FVector UCamera::CalculatePlaneNormal(const FVector4& Axis)
{
	return FVector(Axis.X, Axis.Y, Axis.Z).Cross(Forward);
}
FVector UCamera::CalculatePlaneNormal(const FVector& Axis)
{
	return FVector(Axis.X, Axis.Y, Axis.Z).Cross(Forward);
}
