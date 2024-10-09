#pragma once

#include "CoreMinimal.h"
#include "LogMqttify.h"

namespace Mqttify::Data
{
	constexpr TCHAR CharLimitExceeded[]          = TEXT("String length exceeds uint16 limit");
	constexpr TCHAR ArchiveNotSaving[]           = TEXT("Expected a saving archive");
	constexpr TCHAR ArchiveNotLoading[]          = TEXT("Expected a loading archive");
	constexpr TCHAR UnexpectedNullTerminator[]   = TEXT("Unexpected null terminator");
	constexpr TCHAR InvalidVariableByteInteger[] = TEXT("Invalid variable byte integer");
	constexpr TCHAR PayloadLimitExceeded[]       = TEXT("Payload length exceeds uint32 limit");


	/**
	 * @brief Get the size of a variable byte integer.
	 * @param InValue Integer to be encoded.
	 * @return The size of the encoded integer. 0 if the integer is too large.
	 */
	inline uint8 VariableByteIntegerSize(const uint32 InValue)
	{
		if (InValue < 1 << 7)
		{
			return 1;
		}
		if (InValue < 1 << 14)
		{
			return 2;
		}
		if (InValue < 1 << 21)
		{
			return 3;
		}
		if (InValue < 1 << 28)
		{
			return 4;
		}

		return 0;
	}

	/**
	 * @brief Decode a variable byte integer from an FArchive.
	 * @param InArchive FArchive object to deserialize from.
	 * @return The decoded integer.
	 */
	inline uint32 DecodeVariableByteInteger(FArchive& InArchive)
	{
		SIZE_T Value = 0;
		uint8 Shift  = 0;
		uint8 Byte;

		do
		{
			if (InArchive.AtEnd())
			{
				LOG_MQTTIFY(Error, InvalidVariableByteInteger);
				return 0;
			}

			InArchive << Byte;
			Value |= (Byte & 0x7F) << Shift;
			Shift += 7;
		}
		while (Byte & 0x80);

		return Value;
	}

	/**
	 * @brief Encode a variable byte integer to an FArchive.
	 * @param OutArchive FArchive object to serialize to.
	 * @param InValue Integer to be encoded.
	 */
	inline void EncodeVariableByteInteger(uint32 InValue, FArchive& OutArchive)
	{
		do
		{
			uint8 Byte = InValue % 128;
			InValue /= 128;

			// If there are more data to encode, set the top bit of this byte
			if (InValue > 0)
			{
				Byte |= 0x80;
			}

			OutArchive << Byte;
		}
		while (InValue > 0);
	}

	/**
	 * @brief Encode a string to an FArchive.
	 * @param OutArchive FArchive object to serialize to.
	 * @param InString String to be encoded.
	*/
	inline void EncodeString(const FString& InString, FArchive& OutArchive)
	{
		UE_CLOG(!OutArchive.IsSaving(), LogMqttify, Error, ArchiveNotSaving);
		uint16 StringLength = InString.Len();
		UE_CLOG(StringLength > 65535, LogMqttify, Error, CharLimitExceeded);
		const auto String = StringCast<UTF8CHAR>(*InString);
		OutArchive << StringLength;
		OutArchive.Serialize(const_cast<void*>(static_cast<const void*>(String.Get())), StringLength);
	}

	/**
	 * @brief Decode a string from an FArchive.
	 * @param InArchive FArchive object to deserialize from.
	 * @return The decoded string.
	 */
	inline FString DecodeString(FArchive& InArchive)
	{
		UE_CLOG(!InArchive.IsLoading(), LogMqttify, Error, ArchiveNotLoading);
		FString Result;
		if (InArchive.IsLoading())
		{
			uint16 StringLength = 0;
			InArchive << StringLength;
			Result.Reserve(StringLength);

			UTF8CHAR TempChar;
			for (uint16 i = 0; i < StringLength; ++i)
			{
				const uint32 CurrentPosition = InArchive.Tell();
				InArchive.Serialize(&TempChar, 1);
				const TCHAR SafeChar = CharCast<TCHAR>(TempChar);
				if (CurrentPosition == InArchive.Tell())
				{
					break;
				}
				Result.AppendChars(&SafeChar, 1);
			}

			// Check the last char isn't a null terminator
			UE_CLOG(Result.Right(1) == TEXT("\0"), LogMqttify, Error, UnexpectedNullTerminator);
		}

		return Result;
	}

	inline void EncodePayload(const TArray<uint8>& InPayload, FArchive& OutArchive)
	{
		UE_CLOG(!OutArchive.IsSaving(), LogMqttify, Error, ArchiveNotSaving);
		const uint32 PayloadSize = InPayload.Num();
		UE_CLOG(PayloadSize > 268435455, LogMqttify, Error, PayloadLimitExceeded);
		OutArchive.Serialize(const_cast<void*>(static_cast<const void*>(InPayload.GetData())), PayloadSize);
	}

	inline TArray<uint8> DecodePayload(FArchive& InArchive, const uint32 PayloadSize)
	{
		UE_CLOG(!InArchive.IsLoading(), LogMqttify, Error, ArchiveNotLoading);
		TArray<uint8> Result;
		if (InArchive.IsLoading())
		{
			Result.Init(0, PayloadSize);
			for(uint32 Idx = 0; Idx < PayloadSize; ++Idx)
			{
				InArchive << Result[Idx];
			}
		}

		return Result;
	}

	inline void EmplaceVariableByteInteger(uint32 InValue, TArray<uint8>& OutArray)
	{
		// we have a variable byte integer, so we need to encode it
		do
		{
			uint8 Byte = InValue % 128;
			InValue /= 128;

			// If there are more data to encode, set the top bit of this byte
			if (InValue > 0)
			{
				Byte |= 0x80;
			}

			OutArray.Emplace(Byte);
		}
		while (InValue > 0);
	}

} // namespace Mqttify::Data
