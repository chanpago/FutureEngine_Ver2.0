#pragma once

#include <type_traits>

#include "Global/CoreTypes.h"
#include "Global/Vector.h"

struct FArchive
{
	virtual ~FArchive() = default;

	/** Returns true if this archive is for loading data. */
	virtual bool IsLoading() const = 0;
	virtual void Serialize(void* V, size_t Length) = 0;

	template<typename T, typename = std::enable_if_t<std::is_trivially_copyable_v<T>>>
	FArchive& operator<<(T& Value)
	{
		Serialize(&Value, sizeof(T));
		return *this;
	}

	template<typename T>
	FArchive& operator<<(TArray<T>& Value)
	{
		size_t Length = Value.size();
		*this << Length;

		if (IsLoading())
		{
			Value.resize(Length);
		}

		for (T& Element : Value)
		{
			*this << Element;
		}

		return *this;
	}

	FArchive& operator<<(FString& Value)
	{
		size_t Length = Value.size();
		*this << Length;

		if (!IsLoading())
		{
			Serialize(Value.data(), Length * sizeof(FString::value_type));
		}
		else
		{
			Value.resize(Length);
			Serialize(Value.data(), Length * sizeof(FString::value_type));
		}

		return *this;
	}
};
