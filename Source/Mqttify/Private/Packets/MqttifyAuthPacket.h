#pragma once

#include "Mqtt/MqttifyReasonCode.h"
#include "Packets/MqttifyFixedHeader.h"
#include "Packets/Interface/MqttifyControlPacket.h"
#include "Properties/MqttifyProperties.h"

namespace Mqttify
{

	struct FMqttifyAuthPacket final : TMqttifyControlPacket<EMqttifyPacketType::Auth>
	{
	public:
		explicit FMqttifyAuthPacket(const EMqttifyReasonCode InReasonCode,
		                            const FMqttifyProperties& InProperties = FMqttifyProperties{})
			: ReasonCode{InReasonCode}
			, Properties{InProperties}
		{
#if !UE_BUILD_SHIPPING
			switch (InReasonCode)
			{
				case EMqttifyReasonCode::Success:
				case EMqttifyReasonCode::ContinueAuthentication:
				case EMqttifyReasonCode::ReAuthenticate:
					break;
				default:
					LOG_MQTTIFY(
						Error,
						TEXT("[Auth Packet] %s %s"),
						MqttifyReasonCode::InvalidReasonCode,
						EnumToTCharString(ReasonCode));
					bIsValid = false;
					checkNoEntry();
					break;
			}

			// check properties are valid
			if (!ArePropertiesValid(InProperties))
			{
				LOG_MQTTIFY(Error, TEXT("[Auth Packet] %s"), FMqttifyProperty::FailedToReadPropertyError);
				bIsValid = false;
				checkNoEntry();
			}
#endif // !UE_BUILD_SHIPPING

			FixedHeader = FMqttifyFixedHeader::Create(this);
		}

		explicit FMqttifyAuthPacket(FArrayReader& InReader, const FMqttifyFixedHeader& InFixedHeader)
			: TMqttifyControlPacket{InFixedHeader}
			, ReasonCode{EMqttifyReasonCode::Success}
		{
			Decode(InReader);
		}

		virtual uint32 GetLength() const override;

		virtual void Encode(FMemoryWriter& InWriter) override;
		virtual void Decode(FArrayReader& InReader) override;

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

	private:
		/**
		 * @brief Check if the properties are valid.
		 * @param InProperties The properties to check.
		 * @return True if the properties are valid, false otherwise.
		 */
		static bool ArePropertiesValid(const FMqttifyProperties& InProperties);

		static bool IsReasonCodeValid(const EMqttifyReasonCode InReasonCode);

		EMqttifyReasonCode ReasonCode;
		FMqttifyProperties Properties;
	};
} // namespace Mqttify
