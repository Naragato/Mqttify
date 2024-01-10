#pragma once

#include "Mqtt/MqttifyProtocolVersion.h"
#include "Mqtt/MqttifyReasonCode.h"
#include "Packets/MqttifyFixedHeader.h"
#include "Packets/Interface/MqttifyControlPacket.h"
#include "Properties/MqttifyProperties.h"

namespace Mqttify
{

	struct FMqttifyDisconnectPacketBase : TMqttifyControlPacket<EMqttifyPacketType::Disconnect>
	{
	public:
		explicit FMqttifyDisconnectPacketBase(const FMqttifyFixedHeader& InFixedHeader)
			: TMqttifyControlPacket{ InFixedHeader } {}

		explicit FMqttifyDisconnectPacketBase() {}

	};

	template <EMqttifyProtocolVersion InProtocolVersion>
	struct TMqttifyDisconnectPacket;

	template <>
	struct TMqttifyDisconnectPacket<EMqttifyProtocolVersion::Mqtt_3_1_1> final : FMqttifyDisconnectPacketBase
	{
		explicit TMqttifyDisconnectPacket()
		{
			FixedHeader = FMqttifyFixedHeader::Create(this);
		}

		explicit TMqttifyDisconnectPacket(FArrayReader& InReader, const FMqttifyFixedHeader& InFixedHeader)
			: FMqttifyDisconnectPacketBase{ InFixedHeader }
		{
			Decode(InReader);
		}

		uint32 GetLength() const override { return 0; }
		void Encode(FMemoryWriter& InWriter) override;
		void Decode(FArrayReader& InReader) override;
	};

	template <>
	struct TMqttifyDisconnectPacket<
			EMqttifyProtocolVersion::Mqtt_5> final : FMqttifyDisconnectPacketBase
	{
	public:
		explicit TMqttifyDisconnectPacket(const EMqttifyReasonCode InReasonCode = EMqttifyReasonCode::Success,
										const FMqttifyProperties& InProperties  = FMqttifyProperties{})
			: ReasonCode{ InReasonCode }
			, Properties{ InProperties }
		{
			FixedHeader = FMqttifyFixedHeader::Create(this);
			if (!IsReasonCodeValid(InReasonCode))
			{
				bIsValid = false;
				LOG_MQTTIFY(
					Error,
					TEXT("[Disconnect] %s %s."),
					MqttifyReasonCode::InvalidReasonCode,
					EnumToTCharString(InReasonCode));
			}
		}

		explicit TMqttifyDisconnectPacket(FArrayReader& InReader, const FMqttifyFixedHeader& InFixedHeader)
			: FMqttifyDisconnectPacketBase{ InFixedHeader }
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

	using FMqttifyDisconnectPacket3 = TMqttifyDisconnectPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>;
	using FMqttifyDisconnectPacket5 = TMqttifyDisconnectPacket<EMqttifyProtocolVersion::Mqtt_5>;
} // namespace Mqttify
