#pragma once

#include "Mqtt/MqttifyConnectReturnCode.h"
#include "Mqtt/MqttifyProtocolVersion.h"
#include "Mqtt/MqttifyReasonCode.h"
#include "Packets/MqttifyFixedHeader.h"
#include "Packets/Interface/MqttifyControlPacket.h"
#include "Properties/MqttifyProperties.h"
#include "Serialization/ArrayReader.h"

namespace Mqttify
{
	struct FMqttifyConnAckPacketBase : TMqttifyControlPacket<EMqttifyPacketType::ConnAck>
	{
	public:
		explicit FMqttifyConnAckPacketBase(const FMqttifyFixedHeader& InFixedHeader)
			: TMqttifyControlPacket{ InFixedHeader }
			, bSessionPresent{ false } {}

		explicit FMqttifyConnAckPacketBase(const bool bInSessionPresent)
			: bSessionPresent{ bInSessionPresent } {}

	public:
		/**
		 * @brief Get the session present flag.
		 * @return The session present flag.
		 */
		bool GetSessionPresent() const { return bSessionPresent; }

	protected:
		bool bSessionPresent;
	};

	template <EMqttifyProtocolVersion InProtocolVersion>
	struct TMqttifyConnAckPacket;

	template <>
	struct TMqttifyConnAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1> final : FMqttifyConnAckPacketBase
	{
		explicit TMqttifyConnAckPacket(const bool bInSessionPresent, const EMqttifyConnectReturnCode InReturnCode)
			: FMqttifyConnAckPacketBase{ bInSessionPresent }
			, ReturnCode{ InReturnCode }
		{
			FixedHeader = FMqttifyFixedHeader::Create(this);
		}

		explicit TMqttifyConnAckPacket(FArrayReader& InReader, const FMqttifyFixedHeader& InFixedHeader)
			: FMqttifyConnAckPacketBase{ InFixedHeader }
		{
			Decode(InReader);
		}

		uint32 GetLength() const override { return sizeof(ReturnCode) + sizeof(uint8); } // flags
		void Encode(FMemoryWriter& InWriter) override;
		void Decode(FArrayReader& InReader) override;

		/**
		 * @brief Get the reason code.
		 * @return The reason code.
		 */
		EMqttifyConnectReturnCode GetReasonCode() const { return ReturnCode; }

	protected:
		EMqttifyConnectReturnCode ReturnCode;
	};

	template <>
	struct TMqttifyConnAckPacket<
			EMqttifyProtocolVersion::Mqtt_5> final : FMqttifyConnAckPacketBase
	{
	public:
		explicit TMqttifyConnAckPacket(const bool bInSessionPresent,
										const EMqttifyReasonCode InReasonCode,
										const FMqttifyProperties& InProperties = FMqttifyProperties{})
			: FMqttifyConnAckPacketBase{ bInSessionPresent }
			, ReasonCode{ InReasonCode }
			, Properties{ InProperties }
		{
			FixedHeader = FMqttifyFixedHeader::Create(this);
			if (!IsReasonCodeValid(InReasonCode))
			{
				bIsValid = false;
				LOG_MQTTIFY(
					Error,
					TEXT("[ConnAck] %s %s."),
					MqttifyReasonCode::InvalidReasonCode,
					EnumToTCharString(InReasonCode));
				ReasonCode = EMqttifyReasonCode::UnspecifiedError;
			}
		}

		explicit TMqttifyConnAckPacket(FArrayReader& InReader, const FMqttifyFixedHeader& InFixedHeader)
			: FMqttifyConnAckPacketBase{ InFixedHeader }
			, ReasonCode{ EMqttifyReasonCode::Success }
		{
			Decode(InReader);
		}

		uint32 GetLength() const override;

		void Encode(FMemoryWriter& InWriter) override;
		void Decode(FArrayReader& InReader) override;

		/**
		 * @brief Get the reason code.
		 * @return The reason code.
		 */
		EMqttifyReasonCode GetReasonCode() const { return ReasonCode; }

		/**
		 * @brief Get the properties.
		 * @return The properties.
		 */
		const FMqttifyProperties& GetProperties() const { return Properties; }

		/**
		 * @brief Check if the reason code is valid.
		 * @return true if the reason code is valid.
		 */
		static bool IsReasonCodeValid(EMqttifyReasonCode InReasonCode);

	protected:
		EMqttifyReasonCode ReasonCode;
		FMqttifyProperties Properties;
	};

	using FMqttifyConnAckPacket3 = TMqttifyConnAckPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>;
	using FMqttifyConnAckPacket5 = TMqttifyConnAckPacket<EMqttifyProtocolVersion::Mqtt_5>;
} // namespace Mqttify
