#pragma once
#include "Component/Public/ProjectileMovementComponent.h"

struct FVector;
struct FQuaternion
{
	float X;
	float Y;
	float Z;
	float W;

	FQuaternion() : X(0), Y(0), Z(0), W(1) {}
	FQuaternion(float InX, float InY, float InZ, float InW) : X(InX), Y(InY), Z(InZ), W(InW) {}

	static FQuaternion Identity() { return FQuaternion(0, 0, 0, 1); }
	static FQuaternion FromAxisAngle(const FVector& Axis, float AngleRad);

	static FQuaternion FromEuler(const FVector& EulerDeg);
	static FQuaternion FromRotationMatrix(const FMatrix& M);
	FVector ToEuler() const;

	FMatrix ToRotationMatrix() const;

	FQuaternion operator*(const FQuaternion& Q) const;

	void Normalize();

	FQuaternion Conjugate() const { return FQuaternion(-X, -Y, -Z, W); }
	FQuaternion Inverse() const { FQuaternion c = Conjugate(); float n = X * X + Y * Y + Z * Z + W * W; return (n > 0) ? FQuaternion(c.X / n, c.Y / n, c.Z / n, c.W / n) : FQuaternion(); }
	static FQuaternion MakeFromDirection(const FVector& Direction);
	static FVector RotateVector(const FQuaternion& q, const FVector& v);
	FVector RotateVector(const FVector& V) const;
};
