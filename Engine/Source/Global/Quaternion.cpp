#include "pch.h"
#include "Global/Quaternion.h"

FQuaternion FQuaternion::FromAxisAngle(const FVector& Axis, float AngleRad)
{
	FVector N = Axis;
	N.Normalize();
	float s = sinf(AngleRad * 0.5f);
	return FQuaternion(
		N.X * s,
		N.Y * s,
		N.Z * s,
		cosf(AngleRad * 0.5f)
	);
}

FQuaternion FQuaternion::FromEuler(const FVector& EulerDeg)
{
	FVector Radians = FVector::GetDegreeToRadian(EulerDeg);

	float cx = cosf(Radians.X * 0.5f);
	float sx = sinf(Radians.X * 0.5f);
	float cy = cosf(Radians.Y * 0.5f);
	float sy = sinf(Radians.Y * 0.5f);
	float cz = cosf(Radians.Z * 0.5f);
	float sz = sinf(Radians.Z * 0.5f);

	// Yaw-Pitch-Roll (Z, Y, X)
	return FQuaternion(
		sx * cy * cz - cx * sy * sz, // X
		cx * sy * cz + sx * cy * sz, // Y
		cx * cy * sz - sx * sy * cz, // Z
		cx * cy * cz + sx * sy * sz  // W
	);
}

FQuaternion FQuaternion::FromRotationMatrix(const FMatrix& M)
{
    FQuaternion Q;
    float trace = M.Data[0][0] + M.Data[1][1] + M.Data[2][2];
    if (trace > 0)
    {
        float s = 0.5f / sqrtf(trace + 1.0f);
        Q.W = 0.25f / s;
        Q.X = (M.Data[2][1] - M.Data[1][2]) * s;
        Q.Y = (M.Data[0][2] - M.Data[2][0]) * s;
        Q.Z = (M.Data[1][0] - M.Data[0][1]) * s;
    }
    else
    {
        if (M.Data[0][0] > M.Data[1][1] && M.Data[0][0] > M.Data[2][2])
        {
            float s = 2.0f * sqrtf(1.0f + M.Data[0][0] - M.Data[1][1] - M.Data[2][2]);
            Q.W = (M.Data[2][1] - M.Data[1][2]) / s;
            Q.X = 0.25f * s;
            Q.Y = (M.Data[0][1] + M.Data[1][0]) / s;
            Q.Z = (M.Data[0][2] + M.Data[2][0]) / s;
        }
        else if (M.Data[1][1] > M.Data[2][2])
        {
            float s = 2.0f * sqrtf(1.0f + M.Data[1][1] - M.Data[0][0] - M.Data[2][2]);
            Q.W = (M.Data[0][2] - M.Data[2][0]) / s;
            Q.X = (M.Data[0][1] + M.Data[1][0]) / s;
            Q.Y = 0.25f * s;
            Q.Z = (M.Data[1][2] + M.Data[2][1]) / s;
        }
        else
        {
            float s = 2.0f * sqrtf(1.0f + M.Data[2][2] - M.Data[0][0] - M.Data[1][1]);
            Q.W = (M.Data[1][0] - M.Data[0][1]) / s;
            Q.X = (M.Data[0][2] + M.Data[2][0]) / s;
            Q.Y = (M.Data[1][2] + M.Data[2][1]) / s;
            Q.Z = 0.25f * s;
        }
    }
    return Q;
}

FVector FQuaternion::ToEuler() const
{
	FVector Euler;

	// Roll (X)
	float sinr_cosp = 2.0f * (W * X + Y * Z);
	float cosr_cosp = 1.0f - 2.0f * (X * X + Y * Y);
	Euler.X = atan2f(sinr_cosp, cosr_cosp);

	// Pitch (Y)
	float sinp = 2.0f * (W * Y - Z * X);
	if (fabs(sinp) >= 1)
		Euler.Y = copysignf(PI / 2, sinp); // 90도 고정
	else
		Euler.Y = asinf(sinp);

	// Yaw (Z)
	float siny_cosp = 2.0f * (W * Z + X * Y);
	float cosy_cosp = 1.0f - 2.0f * (Y * Y + Z * Z);
	Euler.Z = atan2f(siny_cosp, cosy_cosp);

	return FVector::GetRadianToDegree(Euler);
}

FMatrix FQuaternion::ToRotationMatrix() const
{
    FMatrix M;

    const float X2 = X * X;
    const float Y2 = Y * Y;
    const float Z2 = Z * Z;
    const float XY = X * Y;
    const float XZ = X * Z;
    const float YZ = Y * Z;
    const float WX = W * X;
    const float WY = W * Y;
    const float WZ = W * Z;

    M.Data[0][0] = 1.0f - 2.0f * (Y2 + Z2);
    M.Data[0][1] = 2.0f * (XY - WZ);
    M.Data[0][2] = 2.0f * (XZ + WY);
    M.Data[0][3] = 0.0f;

    M.Data[1][0] = 2.0f * (XY + WZ);
    M.Data[1][1] = 1.0f - 2.0f * (X2 + Z2);
    M.Data[1][2] = 2.0f * (YZ - WX);
    M.Data[1][3] = 0.0f;

    M.Data[2][0] = 2.0f * (XZ - WY);
    M.Data[2][1] = 2.0f * (YZ + WX);
    M.Data[2][2] = 1.0f - 2.0f * (X2 + Y2);
    M.Data[2][3] = 0.0f;

    M.Data[3][0] = 0.0f;
    M.Data[3][1] = 0.0f;
    M.Data[3][2] = 0.0f;
    M.Data[3][3] = 1.0f;

    return M;
}

FQuaternion FQuaternion::operator*(const FQuaternion& Q) const
{
	return FQuaternion(
		W * Q.X + X * Q.W + Y * Q.Z - Z * Q.Y,
		W * Q.Y - X * Q.Z + Y * Q.W + Z * Q.X,
		W * Q.Z + X * Q.Y - Y * Q.X + Z * Q.W,
		W * Q.W - X * Q.X - Y * Q.Y - Z * Q.Z
	);
}

void FQuaternion::Normalize()
{
	float mag = sqrtf(X * X + Y * Y + Z * Z + W * W);
	if (mag > 0.0001f)
	{
		X /= mag;
		Y /= mag;
		Z /= mag;
		W /= mag;
	}
}

FQuaternion FQuaternion::MakeFromDirection(const FVector& Direction)
{
	const FVector& ForwardVector = FVector::ForwardVector();
	FVector Dir = Direction.GetNormalized();

	float Dot = ForwardVector.Dot(Dir);
	if (Dot == 1.f) { return Identity(); }

	if (Dot == -1.f)
	{
		// 180도 회전
		FVector RotAxis = FVector::UpVector().Cross(ForwardVector);
		if (RotAxis.IsZero()) // Forward가 UP 벡터와 평행하면 다른 축 사용
		{
			RotAxis = FVector::ForwardVector().Cross(ForwardVector);
		}
		return FromAxisAngle(RotAxis.GetNormalized(), PI);
	}

	float AngleRad = acos(Dot);

	// 두 벡터에 수직인 회전축 계산 후 쿼터니언 생성
	FVector Axis = Dir.Cross(ForwardVector);
	Axis.Normalize();
	return FromAxisAngle(Axis, AngleRad);
}

FVector FQuaternion::RotateVector(const FQuaternion& q, const FVector& v)
{
	FQuaternion p(v.X, v.Y, v.Z, 0.0f);
	FQuaternion r = q * p * q.Inverse();
	return FVector(r.X, r.Y, r.Z);
}

FVector FQuaternion::RotateVector(const FVector& V) const
{
	const FVector Q(X, Y, Z);
	const FVector TT = V.Cross(Q) * 2.f;
	const FVector Result = V + (TT * W) + TT.Cross(Q);
	return Result;
}
