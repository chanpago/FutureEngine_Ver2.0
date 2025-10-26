#include "pch.h"


FMatrix FMatrix::UEToDx = FMatrix(
	{
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
	});

FMatrix FMatrix::DxToUE = FMatrix(
	{
		0.0f, 0.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
	});

/**
* @brief float 타입의 배열을 사용한 FMatrix의 기본 생성자
*/
FMatrix::FMatrix()
	: Data{ {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0} }
{
}


/**
* @brief float 타입의 param을 사용한 FMatrix의 기본 생성자
*/
FMatrix::FMatrix(
	float M00, float M01, float M02, float M03,
	float M10, float M11, float M12, float M13,
	float M20, float M21, float M22, float M23,
	float M30, float M31, float M32, float M33)
	: Data{ {M00,M01,M02,M03},
			{M10,M11,M12,M13},
			{M20,M21,M22,M23},
			{M30,M31,M32,M33} }
{
}

FMatrix::FMatrix(const FVector& x, const FVector& y, const FVector& z)
	:Data{ {x.X, x.Y, x.Z, 0.0f},
			{y.X, y.Y, y.Z, 0.0f},
			{z.X, z.Y, z.Z, 0.0f},
			{0.0f, 0.0f, 0.0f, 1.0f} }
{
}

FMatrix::FMatrix(const FVector4& x, const FVector4& y, const FVector4& z)
	:Data{ {x.X,x.Y,x.Z, x.W},
			{y.X,y.Y,y.Z,y.W},
			{z.X,z.Y,z.Z,z.W},
			{0.0f, 0.0f, 0.0f, 1.0f} }
{
}

/**
* @brief 항등행렬
*/
FMatrix FMatrix::Identity()
{
	return FMatrix(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
}


/**
* @brief 두 행렬곱을 진행한 행렬을 반환하는 연산자 함수
*/
FMatrix FMatrix::operator*(const FMatrix& InOtherMatrix) const
{
	FMatrix Result;

	for (int32 i = 0; i < 4; ++i)
	{
		// Result의 i번째 행을 0으로 초기화
		__m128 Res = _mm_setzero_ps();

		// A의 i번째 행과 B의 각 열의 내적 계산
		for (int32 k = 0; k < 4; ++k)
		{
			// A[i][k]를 4개 레인에 브로드캐스트
			__m128 Scalar = _mm_set1_ps(Data[i][k]);

			// B의 k번째 행 (또는 전치된 B의 k번째 열)
			__m128 OtherRow = InOtherMatrix.V[k];

			// 병렬 곱셈 후 누적
			Res = _mm_add_ps(Res, _mm_mul_ps(Scalar, OtherRow));
		}

		Result.V[i] = Res;
	}

	return Result;
}



void FMatrix::operator*=(const FMatrix& InOtherMatrix)
{
	*this = (*this) * InOtherMatrix;
}

FVector4 FMatrix::operator[](uint32 i) const
{
	// 잘못된 index를 전달받으면 빈 벡터를 반환
	if (i >= 4)
		return FVector4();
	return FVector4(Data[0][i], Data[1][i], Data[2][i], Data[3][i]);
}

/**
* @brief Position의 정보를 행렬로 변환하여 제공하는 함수
*/
FMatrix FMatrix::TranslationMatrix(const FVector& InOtherVector)
{
	FMatrix Result = FMatrix::Identity();
	Result.Data[3][0] = InOtherVector.X;
	Result.Data[3][1] = InOtherVector.Y;
	Result.Data[3][2] = InOtherVector.Z;
	Result.Data[3][3] = 1;

	return Result;
}

FMatrix FMatrix::TranslationMatrixInverse(const FVector& InOtherVector)
{
	FMatrix Result = FMatrix::Identity();
	Result.Data[3][0] = -InOtherVector.X;
	Result.Data[3][1] = -InOtherVector.Y;
	Result.Data[3][2] = -InOtherVector.Z;
	Result.Data[3][3] = 1;

	return Result;
}

/**
* @brief Scale의 정보를 행렬로 변환하여 제공하는 함수
*/
FMatrix FMatrix::ScaleMatrix(const FVector& InOtherVector)
{
	FMatrix Result = FMatrix::Identity();
	Result.Data[0][0] = InOtherVector.X;
	Result.Data[1][1] = InOtherVector.Y;
	Result.Data[2][2] = InOtherVector.Z;
	Result.Data[3][3] = 1;

	return Result;
}
FMatrix FMatrix::ScaleMatrixInverse(const FVector& InOtherVector)
{
	FMatrix Result = FMatrix::Identity();
	Result.Data[0][0] = 1 / InOtherVector.X;
	Result.Data[1][1] = 1 / InOtherVector.Y;
	Result.Data[2][2] = 1 / InOtherVector.Z;
	Result.Data[3][3] = 1;

	return Result;
}

/**
* @brief Rotation의 정보를 행렬로 변환하여 제공하는 함수
*/
FMatrix FMatrix::RotationMatrix(const FVector& InOtherVector)
{
	// Dx11 yaw(y), pitch(x), roll(z)
	// UE yaw(z), pitch(y), roll(x)
	// 회전 축이 바뀌어서 각 회전행렬 함수에 바뀐 값을 적용

	const float yaw = InOtherVector.Y; // pitch
	const float pitch = InOtherVector.X; // roll
	const float roll = InOtherVector.Z; // yaw
	//return RotationZ(yaw) * RotationY(pitch) * RotationX(roll);
	//return RotationX(yaw) * RotationY(roll) * RotationZ(pitch);
	return RotationX(pitch) * RotationY(yaw) * RotationZ(roll);
}

FMatrix FMatrix::CreateFromYawPitchRoll(const float yaw, const float pitch, const float roll)
{
	//return RotationZ(yaw) * RotationY(pitch)* RotationX(roll);
	return RotationX(pitch) * RotationY(yaw) * RotationZ(roll);
}

FMatrix FMatrix::RotationMatrixInverse(const FVector& InOtherVector)
{
	const float yaw = InOtherVector.Y;
	const float pitch = InOtherVector.X;
	const float roll = InOtherVector.Z;

	return RotationZ(-roll) * RotationY(-yaw) * RotationX(-pitch);
}

/**
* @brief X의 회전 정보를 행렬로 변환
*/
FMatrix FMatrix::RotationX(float Radian)
{
	FMatrix Result = FMatrix::Identity();
	const float C = std::cosf(Radian);
	const float S = std::sinf(Radian);

	Result.Data[1][1] = C;
	Result.Data[1][2] = S;
	Result.Data[2][1] = -S;
	Result.Data[2][2] = C;

	return Result;
}

/**
* @brief Y의 회전 정보를 행렬로 변환
*/
FMatrix FMatrix::RotationY(float Radian)
{
	FMatrix Result = FMatrix::Identity();
	const float C = std::cosf(Radian);
	const float S = std::sinf(Radian);

	Result.Data[0][0] = C;
	Result.Data[0][2] = -S;
	Result.Data[2][0] = S;
	Result.Data[2][2] = C;

	return Result;
}

/**
* @brief Y의 회전 정보를 행렬로 변환
*/
FMatrix FMatrix::RotationZ(float Radian)
{
	FMatrix Result = FMatrix::Identity();
	const float C = std::cosf(Radian);
	const float S = std::sinf(Radian);

	Result.Data[0][0] = C;
	Result.Data[0][1] = S;
	Result.Data[1][0] = -S;
	Result.Data[1][1] = C;

	return Result;
}

//
FMatrix FMatrix::GetModelMatrix(const FVector& Location, const FVector& Rotation, const FVector& Scale)
{
	FMatrix T = TranslationMatrix(Location);
	FMatrix R = RotationMatrix(Rotation);
	FMatrix S = ScaleMatrix(Scale);
	FMatrix modelMatrix = S * R * T;

	return  modelMatrix;
}

FMatrix FMatrix::GetModelMatrix(const FVector& Location, const FQuaternion& Rotation, const FVector& Scale)
{
    FMatrix T = TranslationMatrix(Location);
    FMatrix R = Rotation.ToRotationMatrix();
    FMatrix S = ScaleMatrix(Scale);
    FMatrix modelMatrix = S * R * T;

    return  modelMatrix;
}

FMatrix FMatrix::GetModelMatrixInverse(const FVector& Location, const FVector& Rotation, const FVector& Scale)
{
	FMatrix T = TranslationMatrixInverse(Location);
	FMatrix R = RotationMatrixInverse(Rotation);
	FMatrix S = ScaleMatrixInverse(Scale);
	FMatrix modelMatrixInverse = T * R * S;

	return modelMatrixInverse;
}

FMatrix FMatrix::GetModelMatrixInverse(const FVector& Location, const FQuaternion& Rotation, const FVector& Scale)
{
    FMatrix T = TranslationMatrixInverse(Location);
    // The inverse of a rotation matrix is its transpose.
    FMatrix R = Rotation.ToRotationMatrix().Transpose();
    FMatrix S = ScaleMatrixInverse(Scale);
    FMatrix modelMatrixInverse = T * R * S;

    return modelMatrixInverse;
}

FVector4 FMatrix::VectorMultiply(const FVector4& V, const FMatrix& M)
{
	FVector4 result = {};
	result.X = (V.X * M.Data[0][0]) + (V.Y * M.Data[1][0]) + (V.Z * M.Data[2][0]) + (V.W * M.Data[3][0]);
	result.Y = (V.X * M.Data[0][1]) + (V.Y * M.Data[1][1]) + (V.Z * M.Data[2][1]) + (V.W * M.Data[3][1]);
	result.Z = (V.X * M.Data[0][2]) + (V.Y * M.Data[1][2]) + (V.Z * M.Data[2][2]) + (V.W * M.Data[3][2]);
	result.W = (V.X * M.Data[0][3]) + (V.Y * M.Data[1][3]) + (V.Z * M.Data[2][3]) + (V.W * M.Data[3][3]);


	return result;
}

FVector FMatrix::VectorMultiply(const FVector& V, const FMatrix& M)
{
	FVector result = {};
	result.X = (V.X * M.Data[0][0]) + (V.Y * M.Data[1][0]) + (V.Z * M.Data[2][0]);
	result.Y = (V.X * M.Data[0][1]) + (V.Y * M.Data[1][1]) + (V.Z * M.Data[2][1]);
	result.Z = (V.X * M.Data[0][2]) + (V.Y * M.Data[1][2]) + (V.Z * M.Data[2][2]);
	//result.W = (V.X * M.Data[0][3]) + (V.Y * M.Data[1][3]) + (V.Z * M.Data[2][3]) + (V.W * M.Data[3][3]);


	return result;
}

FMatrix FMatrix::Transpose() const
{
	// 1. 4개 행을 SIMD 레지스터에 로드
	__m128 Row0 = V[0];
	__m128 Row1 = V[1];
	__m128 Row2 = V[2];
	__m128 Row3 = V[3];

	// 2. 1단계 셔플: 0/1행, 2/3행을 묶어 하위/상위 요소를 교차
	__m128 T0 = _mm_unpacklo_ps(Row0, Row1); // (M00, M10, M01, M11)
	__m128 T1 = _mm_unpackhi_ps(Row0, Row1); // (M02, M12, M03, M13)
	__m128 T2 = _mm_unpacklo_ps(Row2, Row3); // (M20, M30, M21, M31)
	__m128 T3 = _mm_unpackhi_ps(Row2, Row3); // (M22, M32, M23, M33)

	// 3. 2단계 셔플: 중간 결과를 조합하여 최종 전치된 행(원래 행렬의 열)을 만듦
	FMatrix Result;

	// Result.V[0] = (M00, M10, M20, M30) - 기존 0열
	Result.V[0] = _mm_shuffle_ps(T0, T2, _MM_SHUFFLE(1, 0, 1, 0));

	// Result.V[1] = (M01, M11, M21, M31) - 기존 1열
	Result.V[1] = _mm_shuffle_ps(T0, T2, _MM_SHUFFLE(3, 2, 3, 2));

	// Result.V[2] = (M02, M12, M22, M32) - 기존 2열
	Result.V[2] = _mm_shuffle_ps(T1, T3, _MM_SHUFFLE(1, 0, 1, 0));

	// Result.V[3] = (M03, M13, M23, M33) - 기존 3열
	Result.V[3] = _mm_shuffle_ps(T1, T3, _MM_SHUFFLE(3, 2, 3, 2));

	return Result;
}

FVector FMatrix::GetLocation() const
{
    return FVector(Data[3][0], Data[3][1], Data[3][2]);
}

FVector FMatrix::GetRotation() const
{
    // Assuming the angles are in radians.
    float pitch = -asinf(Data[2][0]);
    float yaw = atan2f(Data[1][0], Data[0][0]);
    float roll = atan2f(Data[2][1], Data[2][2]);
    return FVector(pitch, yaw, roll);
}

FVector FMatrix::GetScale() const
{
    return FVector(sqrtf(Data[0][0] * Data[0][0] + Data[0][1] * Data[0][1] + Data[0][2] * Data[0][2]),
                   sqrtf(Data[1][0] * Data[1][0] + Data[1][1] * Data[1][1] + Data[1][2] * Data[1][2]),
                   sqrtf(Data[2][0] * Data[2][0] + Data[2][1] * Data[2][1] + Data[2][2] * Data[2][2]));
}

FVector FMatrix::TransformPosition(const FVector& V) const
{
    return FVector(
        V.X * Data[0][0] + V.Y * Data[1][0] + V.Z * Data[2][0] + Data[3][0],
        V.X * Data[0][1] + V.Y * Data[1][1] + V.Z * Data[2][1] + Data[3][1],
        V.X * Data[0][2] + V.Y * Data[1][2] + V.Z * Data[2][2] + Data[3][2]
    );
}

/**
* @brief 4x4 행렬의 역행렬을 계산 (Row-major)
* @return 역행렬 (determinant가 0이면 항등행렬 반환)
*/
FMatrix FMatrix::Inverse() const
{
    const float* m = &Data[0][0];

    // 2x2 서브행렬의 determinant들을 계산
    float s0 = m[0] * m[5] - m[4] * m[1];
    float s1 = m[0] * m[6] - m[4] * m[2];
    float s2 = m[0] * m[7] - m[4] * m[3];
    float s3 = m[1] * m[6] - m[5] * m[2];
    float s4 = m[1] * m[7] - m[5] * m[3];
    float s5 = m[2] * m[7] - m[6] * m[3];

    float c5 = m[10] * m[15] - m[14] * m[11];
    float c4 = m[9] * m[15] - m[13] * m[11];
    float c3 = m[9] * m[14] - m[13] * m[10];
    float c2 = m[8] * m[15] - m[12] * m[11];
    float c1 = m[8] * m[14] - m[12] * m[10];
    float c0 = m[8] * m[13] - m[12] * m[9];

    // Determinant 계산
    float det = s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0;

    // Determinant가 0이면 역행렬이 존재하지 않음
    const float epsilon = 1e-8f;
    if (fabsf(det) < epsilon)
    {
        return FMatrix::Identity();
    }

    float invDet = 1.0f / det;

    // Cofactor matrix 계산 (row-major)
    FMatrix result;

    result.Data[0][0] = (m[5] * c5 - m[6] * c4 + m[7] * c3) * invDet;
    result.Data[0][1] = (-m[1] * c5 + m[2] * c4 - m[3] * c3) * invDet;
    result.Data[0][2] = (m[13] * s5 - m[14] * s4 + m[15] * s3) * invDet;
    result.Data[0][3] = (-m[9] * s5 + m[10] * s4 - m[11] * s3) * invDet;

    result.Data[1][0] = (-m[4] * c5 + m[6] * c2 - m[7] * c1) * invDet;
    result.Data[1][1] = (m[0] * c5 - m[2] * c2 + m[3] * c1) * invDet;
    result.Data[1][2] = (-m[12] * s5 + m[14] * s2 - m[15] * s1) * invDet;
    result.Data[1][3] = (m[8] * s5 - m[10] * s2 + m[11] * s1) * invDet;

    result.Data[2][0] = (m[4] * c4 - m[5] * c2 + m[7] * c0) * invDet;
    result.Data[2][1] = (-m[0] * c4 + m[1] * c2 - m[3] * c0) * invDet;
    result.Data[2][2] = (m[12] * s4 - m[13] * s2 + m[15] * s0) * invDet;
    result.Data[2][3] = (-m[8] * s4 + m[9] * s2 - m[11] * s0) * invDet;

    result.Data[3][0] = (-m[4] * c3 + m[5] * c1 - m[6] * c0) * invDet;
    result.Data[3][1] = (m[0] * c3 - m[1] * c1 + m[2] * c0) * invDet;
    result.Data[3][2] = (-m[12] * s3 + m[13] * s1 - m[14] * s0) * invDet;
    result.Data[3][3] = (m[8] * s3 - m[9] * s1 + m[10] * s0) * invDet;

    return result;
}



