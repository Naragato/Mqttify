#pragma once

#include "IsAllowedPointerType.h"
#include "Packets/Interface/IMqttifyControlPacket.h"
#include "Templates/SharedPointer.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMqttify, Display, All);

// __PRETTY_FUNCTION__ isn't a macro on gcc, so use a check for _MSC_VER
// so we only define it as one for MSVC
#if defined(_MSC_VER)
#define MQTTIFY_PRETTY_FUNCTION __FUNCSIG__
#elif defined(__GNUC__) || defined(__clang__)
#define MQTTIFY_PRETTY_FUNCTION __PRETTY_FUNCTION__
#else
#define MQTTIFY_PRETTY_FUNCTION __FUNCTION__
#endif // defined(_MSC_VER)

/**
 * @brief Log a message with the MQTTIFY log category.
 * @param Verbosity The verbosity level of the log.
 * @param Format The format string for the log message.
 * @param ... Additional arguments for the format string.
 */
#define LOG_MQTTIFY(Verbosity, Format, ...)\
do {\
UE_LOG(LogMqttify, Verbosity, TEXT("[%s]: %s"),\
StringCast<TCHAR>(MQTTIFY_PRETTY_FUNCTION).Get(),\
*FString::Printf(Format, ##__VA_ARGS__));\
} while (0)

namespace Mqttify
{


	/**
	 * @brief Convert data to a hex string.
	 * @param Data The data to convert.
	 * @param Length The length of the data.
	 * @return The hex string representation of the data.
	 */
	inline FString DataToHexString(const uint8* Data, size_t Length)
	{
		FString HexString;
		for (size_t i = 0; i < Length; ++i)
		{
			HexString += FString::Printf(TEXT("%02x "), Data[i]);
		}
		return HexString;
	}

	/**
	* @brief Serialize a packet to a hex string.
	* @param InLogPacket The packet to serialize.
	* @return The hex string representation of the serialized packet.
	*/
	template <typename TPointerType>
	FString SerializePacketToHexString(const TPointerType& InLogPacket)
	{
		static_assert(
			IsAllowedPointerType<TPointerType>::value,
			"TPointerType must be TSharedRef, TSharedPtr, or TUniquePtr");

		if constexpr (std::is_same_v<TPointerType, TUniquePtr<IMqttifyControlPacket>> || std::is_same_v<
			TPointerType, TSharedPtr<IMqttifyControlPacket>>)
		{
			if (!InLogPacket.IsValid())
			{
				return TEXT("nullptr");
			}
		}

		TArray<uint8> SerializedData;
		FMemoryWriter Writer(SerializedData);
		InLogPacket->Encode(Writer);
		return DataToHexString(SerializedData.GetData(), SerializedData.Num());
	}
}

/**
 * @brief Print the packet type, packet id, and the serialized data as hex with a custom message.
 * @param Level The log level to print at.
 * @param Msg The custom message to print.
 * @param LogPacket The packet to print.
 * @param ... Additional arguments for the custom message.
 */
#define LOG_MQTTIFY_PACKET(Level, Msg, LogPacket, ...)\
do {\
if (UE_LOG_ACTIVE(LogMqttify, Level))\
{\
LOG_MQTTIFY(Level, TEXT("%s. Packet Type: %s, PacketId: %d, Hex Value: %s"), \
*FString::Printf(Msg, ##__VA_ARGS__),\
Mqttify::EnumToTCharString(LogPacket->GetPacketType()),\
LogPacket->GetPacketId(),\
*Mqttify::SerializePacketToHexString(LogPacket));\
}\
} while(0)

/**
 * @brief Print the data as hex with a custom message.
 * @param Level The log level to print at.
 * @param Data The data to print.
 * @param Length The length of the data.
 * @param Msg The custom message to print.
 * @param ... Additional arguments for the custom message.
 */
#define LOG_MQTTIFY_PACKET_DATA(Level, Data, Length, Msg, ...)\
do {\
if (UE_LOG_ACTIVE(LogMqttify, Level))\
{\
FString HexString = Mqttify::DataToHexString(reinterpret_cast<const uint8*>(Data), static_cast<size_t>(Length));\
LOG_MQTTIFY(Level, TEXT("%s. Data as hex: %s, Length: %d"),\
*FString::Printf(Msg, ##__VA_ARGS__),\
*HexString,\
Length);\
}\
} while(0)
