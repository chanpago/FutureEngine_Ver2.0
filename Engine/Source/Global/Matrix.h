#pragma once
struct FVector;
struct FVector4;
struct FQuaternion;

struct FMatrix
{
	/**
	* @brief 4x4 float 타입의 행렬
	*/
	#if defined(_MSC_VER) // MSVC 컴파일러
		__declspec(align(16))
	#else // GCC, Clang 등
		alignas(16)
	#endif
	union
	{
		/**
		 * @brief 스칼라 접근용
		 */
		float Data[4][4];
		/**
		 * @brief SIMD 접근용: 4개의 __m128 벡터로 접근
		 */
		__m128 V[4];
	};

	static FMatrix DxToUE;
	static FMatrix UEToDx;

	/**
	* @brief float 타입의 배열을 사용한 FMatrix의 기본 생성자
	*/
	FMatrix();

	/**
	* @brief float 타입의 param을 사용한 FMatrix의 기본 생성자
	*/
	FMatrix(
		float M00, float M01, float M02, float M03,
		float M10, float M11, float M12, float M13,
		float M20, float M21, float M22, float M23,
		float M30, float M31, float M32, float M33);

	FMatrix(const FVector&, const FVector&, const FVector&);

	FMatrix(const FVector4&, const FVector4&, const FVector4&);

	/**
	* @brief 항등행렬
	*/
	static FMatrix Identity();


	/**
	* @brief 두 행렬곱을 진행한 행렬을 반환하는 연산자 함수
	*/
	FMatrix operator*(const FMatrix& InOtherMatrix) const;
	
	void operator*=(const FMatrix& InOtherMatrix);

	/**
	* @brief Data의 행을 FVector4의 형태로 반환
	*/
	FVector4 operator[](uint32 i) const;

	/**
	* @brief Position의 정보를 행렬로 변환하여 제공하는 함수
	*/
	static FMatrix TranslationMatrix(const FVector& InOtherVector);
	static FMatrix TranslationMatrixInverse(const FVector& InOtherVector);

	/**
	* @brief Scale의 정보를 행렬로 변환하여 제공하는 함수
	*/
	static FMatrix ScaleMatrix(const FVector& InOtherVector);
	static FMatrix ScaleMatrixInverse(const FVector& InOtherVector);

	/**
	* @brief Rotation의 정보를 행렬로 변환하여 제공하는 함수
	*/
	static FMatrix RotationMatrix(const FVector& InOtherVector);

	static FMatrix CreateFromYawPitchRoll(const float yaw, const float pitch, const float roll);

	static FMatrix RotationMatrixInverse(const FVector& InOtherVector);

	/**
	* @brief X의 회전 정보를 행렬로 변환
	*/
	static FMatrix RotationX(float Radian);

	/**
	* @brief Y의 회전 정보를 행렬로 변환
	*/
	static FMatrix RotationY(float Radian);

	/**
	* @brief Y의 회전 정보를 행렬로 변환
	*/
	static FMatrix RotationZ(float Radian);

	    static FMatrix GetModelMatrix(const FVector& Location, const FVector& Rotation, const FVector& Scale);
	
	    static FMatrix GetModelMatrix(const FVector& Location, const FQuaternion& Rotation, const FVector& Scale);
	    static FMatrix GetModelMatrixInverse(const FVector& Location, const FVector& Rotation, const FVector& Scale);
	
		static FMatrix GetModelMatrixInverse(const FVector& Location, const FQuaternion& Rotation, const FVector& Scale);
	static FVector4 VectorMultiply(const FVector4&, const FMatrix&);

	static FVector VectorMultiply(const FVector& v, const FMatrix& m);

	FMatrix Transpose() const;

	/**
	* @brief 역행렬을 반환하는 함수
	* @return 역행렬 (역행렬이 존재하지 않으면 항등행렬 반환)
	*/
	FMatrix Inverse() const;

	FVector GetLocation() const;
	FVector GetRotation() const;
	FVector GetScale() const;
	FVector TransformPosition(const FVector& V) const;
};
