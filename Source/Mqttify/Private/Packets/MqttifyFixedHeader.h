#pragma once

#include "Mqtt/MqttifyQualityOfService.h"
#include "Packets/MqttifyPacketType.h"
#include "Packets/Interface/IMqttifyControlPacket.h"
#include "Serialization/ArrayReader.h"
#include "Serialization/MqttifyFArchiveEncodeDecode.h"
#include "Serialization/Interface/ISerializable.h"

namespace Mqttify
{
	/**
	 * @brief Represents the fixed header for an MQTT control packet.
	 * Handles the serialization and deserialization of the fixed header,
	 * as defined in the MQTT 5.0 and 3.1.1 specifications.
	 * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/mqtt-v5.0.html
	 * MQTT 3.1.1: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Figure_2.2_-
	 */
	class FMqttifyFixedHeader final : ISerializable
	{
	private:
		using FFixedHeaderPtr = TUniquePtr<FMqttifyFixedHeader>;

		/// @brief Maximum packet size for MQTT 3.
		static constexpr int Mqtt3_1_1MaxPacketSize = 268435455;

		/// @brief Stores packet-specific flags.
		uint8 Flags;
		/// @brief Stores the length of the remaining payload.
		uint32 RemainingLength;
		/**
		 * @brief Private constructor.
		 * @param InFlags Packet-specific flags.
		 * @param InRemainingLength Length of the remaining payload.
		 */
		FMqttifyFixedHeader(const uint8 InFlags, const uint32 InRemainingLength)
			: Flags{ InFlags }
			, RemainingLength{ InRemainingLength } {}

		/**
		 * @brief Private constructor for deserialization.
		 * @param InReader Archive for deserialization.
		 */
		explicit FMqttifyFixedHeader(FArrayReader& InReader)
			: Flags{ 0 }
			, RemainingLength{ 0 }
		{
			Decode(InReader);
		}

	public:
		explicit FMqttifyFixedHeader()
			: Flags{ 0 }
			, RemainingLength{ 0 } {}

		/**
		 * @brief Get the remaining payload length.
		 * @return The remaining payload length.
		 */
		uint32 GetRemainingLength() const { return RemainingLength; }

		/**
		 * @brief Get the packet type from flags.
		 * @return The type of the MQTT packet.
		 */
		EMqttifyPacketType GetPacketType() const
		{
			return static_cast<EMqttifyPacketType>((Flags & 0xF0) >> 4);
		}

		/**
		 * @brief Get the packet type from flags.
		 * @return The flags of the MQTT packet's fixed header.
		 */
		uint8 GetFlags() const { return Flags; }

		/**
		 * @brief Create a unique pointer to a fixed header by deserializing it.
		 * @param InArchive Archive for deserialization.
		 * @return A unique pointer to the deserialized fixed header.
		 */
		static FMqttifyFixedHeader Create(FArrayReader& InArchive)
		{
			return FMqttifyFixedHeader(InArchive);
		}

		/**
		 * @brief Create a fixed header from an MQTT control packet.
		 * @tparam Args Types of additional arguments.
		 * @param InControlPacket Pointer to the MQTT control packet.
		 * @return A unique pointer to the fixed header.
		 */
		static FMqttifyFixedHeader Create(const IMqttifyControlPacket* InControlPacket);

		/**
		 * @brief Create a fixed header from an MQTT control packet.
		 * @param InControlPacket Pointer to the MQTT control packet.
		 * @param InRemainingLength Length of the remaining payload. (Basically the packet size excluding the fixed header)
		 * @param bInShouldRetain Whether the packet should be retained.
		 * @param InQualityOfService Quality of service for the packet.
		 * @param bInIsDuplicated Whether the packet is a duplicate.
		 * @return A unique pointer to the fixed header.
		 */
		static FMqttifyFixedHeader Create(const IMqttifyControlPacket* InControlPacket,
										const uint32 InRemainingLength,
										bool bInShouldRetain,
										EMqttifyQualityOfService InQualityOfService,
										bool bInIsDuplicated);

		void Encode(FMemoryWriter& InWriter) override;
		void Decode(FArrayReader& InReader) override;

		static constexpr TCHAR InvalidPacketSize[]      = TEXT("Invalid packet size.");
		static constexpr TCHAR InvalidRemainingLength[] = TEXT("Invalid Remaining Length.");
	};


	FORCEINLINE FMqttifyFixedHeader FMqttifyFixedHeader::Create(const IMqttifyControlPacket* InControlPacket)
	{
		ensureMsgf(nullptr != InControlPacket, TEXT("Control packet is null"));
		uint8 Flags                  = static_cast<uint8>(InControlPacket->GetPacketType()) << 4;
		const uint32 RemainingLength = InControlPacket->GetLength();
		// All other packets are reserved however use and MUST be set to the value listed in that table in the spec
		// See the spec. Which is why we set the flags here.
		if (InControlPacket->GetPacketType() == EMqttifyPacketType::PubRel
			|| InControlPacket->GetPacketType() == EMqttifyPacketType::Subscribe
			|| InControlPacket->GetPacketType() == EMqttifyPacketType::Unsubscribe)
		{
			Flags |= 0x02;
		}

		return FMqttifyFixedHeader(Flags, RemainingLength);
	}

	FORCEINLINE FMqttifyFixedHeader FMqttifyFixedHeader::Create(const IMqttifyControlPacket* InControlPacket,
																const uint32 InRemainingLength,
																const bool bInShouldRetain,
																const EMqttifyQualityOfService InQualityOfService,
																const bool bInIsDuplicated)
	{
		ensureMsgf(nullptr != InControlPacket, TEXT("Control packet is null"));
		uint8 Flags = static_cast<uint8>(InControlPacket->GetPacketType()) << 4;

		if (bInShouldRetain)
		{
			Flags |= 1 << 0;
		}
		Flags |= static_cast<uint8>(InQualityOfService) << 1;
		if (bInIsDuplicated && InQualityOfService != EMqttifyQualityOfService::AtMostOnce)
		{
			Flags |= 1 << 3;
		}

		return FMqttifyFixedHeader(Flags, InRemainingLength);
	}

	FORCEINLINE void FMqttifyFixedHeader::Encode(FMemoryWriter& InWriter)
	{
		InWriter.SetByteSwapping(true);
		InWriter << Flags;
		Data::EncodeVariableByteInteger(RemainingLength, InWriter);
	}

	FORCEINLINE void FMqttifyFixedHeader::Decode(FArrayReader& InReader)
	{
		InReader.SetByteSwapping(true);
		uint8 TempFlags = 0;
		InReader << TempFlags;

		// Extract the raw packet type from TempFlags
		const uint8 RawPacketType = (TempFlags & 0xF0) >> 4;

		constexpr uint8 MaxAllowedType = static_cast<uint8>(EMqttifyPacketType::Max);
		// Determine the maximum allowable packet type based on MQTT protocol version
		// Check for an invalid packet type
		if (RawPacketType >= MaxAllowedType)
		{
			const std::bitset<8> Bits(TempFlags);
			LOG_MQTTIFY(Error,
						TEXT("[Fixed Header] %s %s."),
						MqttifyPacketType::InvalidPacketType,
						*FString(Bits.to_string().c_str()));
			return;
		}

		const auto PacketType = static_cast<EMqttifyPacketType>(RawPacketType);

		if (PacketType == EMqttifyPacketType::None)
		{
			LOG_MQTTIFY(Error, TEXT("[Fixed Header] %s"), MqttifyPacketType::InvalidPacketType);
			return;
		}

		RemainingLength = Data::DecodeVariableByteInteger(InReader);

		const uint8 RemainingLengthFieldSize = Data::VariableByteIntegerSize(RemainingLength);
		if (RemainingLengthFieldSize == 0)
		{
			LOG_MQTTIFY(
				Error,
				TEXT("[Fixed Header] %s %d."),
				InvalidRemainingLength,
				RemainingLength);
			Flags           = 0;
			RemainingLength = 0;
			return;
		}

		const int32 TotalPacketSize = RemainingLength + sizeof(uint8) + RemainingLengthFieldSize;
		if (InReader.TotalSize() != TotalPacketSize)
		{
			LOG_MQTTIFY(
				Error,
				TEXT("[Fixed Header] %s Expected: %d Actual: %d."),
				InvalidPacketSize,
				TotalPacketSize,
				InReader.TotalSize());
			TempFlags = static_cast<uint8>(EMqttifyPacketType::None) << 4;
		}

		Flags = TempFlags;
	}
} // namespace Mqttify
