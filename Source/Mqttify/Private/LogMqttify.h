#pragma once

#include "Packets/Interface/IMqttifyControlPacket.h"
#include <bitset>
#include <type_traits>

DECLARE_LOG_CATEGORY_EXTERN(LogMqttify, VeryVerbose, All);

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
 * @brief Print a log message with the file, line, function and message.
 * @param Verbosity The verbosity of the message.
 * @param Format The format of the message.
 * @param ... The arguments for the format.
 */
#define LOG_MQTTIFY(Verbosity, Format, ...)\
do {\
UE_LOG(LogMqttify, Verbosity, TEXT("%s:%d:[%s]: %s"),\
StringCast<TCHAR>(__FILE__).Get(),\
__LINE__,\
StringCast<TCHAR>(MQTTIFY_PRETTY_FUNCTION).Get(),\
*FString::Printf(Format, ##__VA_ARGS__));\
} while (0)

/**
 * @brief Print the current byte in the archive.
 * @param InArchive The archive to print the current byte of.
 */
#define LOG_MQTTIFY_CURRENT_BYTE(InArchive)\
do {\
uint8 CurrentByte;\
InArchive << CurrentByte;\
LOG_MQTTIFY(Log, TEXT("Current byte %x, index %d"), CurrentByte, InArchive.Tell());\
InArchive.Seek(InArchive.Tell() - 1);\
} while (0)

/**
 * @brief Print the packet type and packet id and the serialized data as hex.
 * @param Level The log level to print at.
 * @param Packet The packet to print.
 */
#define LOG_MQTTIFY_PACKET(Level, LogPacket)\
do {\
if (UE_LOG_ACTIVE(LogMqttify, Level))\
{\
IMqttifyControlPacket& Packet = const_cast<IMqttifyControlPacket&>(LogPacket);\
TArray<uint8> SerializedData;\
FMemoryWriter Writer(SerializedData);\
Packet.Encode(Writer);\
FString HexString;\
for (uint8 Byte : SerializedData)\
{\
HexString += FString::Printf(TEXT("%02x "), Byte);\
}\
LOG_MQTTIFY(Level, TEXT("Packet Type: %s, PacketId: %d, Hex Value: %s"),\
EnumToTCharString(Packet.GetPacketType()),\
Packet.GetPacketId(),\
*HexString);\
}\
} while(0)

inline FString SerializePacketToHexString(Mqttify::IMqttifyControlPacket& LogPacket)
{
	TArray<uint8> SerializedData;
	FMemoryWriter Writer(SerializedData);
	LogPacket.Encode(Writer);
	FString HexString;
	for (const uint8 Byte : SerializedData)
	{
		HexString += FString::Printf(TEXT("%02x "), Byte);
	}
	return HexString;
}

/**
 * @brief Print the packet type and packet id and the serialized data as hex.
 * with a custom message.
 * @param Level The log level to print at.
 * @param Msg The message to print.
 * @param LogPacket The packet to print.
 */
#define LOG_MQTTIFY_PACKET_MSG(Level, Msg, LogPacket)\
do {\
if (UE_LOG_ACTIVE(LogMqttify, Level))\
{\
LOG_MQTTIFY(Level, TEXT("%s. Packet Type: %s, PacketId: %d, Hex Value: %s"), \
Msg,\
EnumToTCharString(LogPacket.GetPacketType()),\
LogPacket.GetPacketId(),\
*SerializePacketToHexString(LogPacket));\
}\
} while(0)

/**
 * @brief Print the packet type and packet id and the serialized data as hex.
 * @param Level The log level to print at.
 * @param Data The data to print.
 * @param Length The length of the data.
 */
#define LOG_MQTTIFY_PACKET_DATA(Level, Data, Length)\
do {\
if (UE_LOG_ACTIVE(LogMqttify, Level))\
{\
using LengthType = decltype(Length);\
using UnsignedLengthType = typename std::make_unsigned<LengthType>::type;\
UnsignedLengthType LengthUnsigned = static_cast<UnsignedLengthType>(Length);\
FString BitString;\
for (uint64 i = 0; i < LengthUnsigned; ++i)\
{\
std::bitset<8> Bits(Data[i]);\
BitString += FString::Printf(TEXT("%hs "), Bits.to_string().c_str());\
}\
LOG_MQTTIFY(VeryVerbose, TEXT("Data as bits: %s, Length %d"),\
*BitString, \
Length);\
}\
} while(0)
