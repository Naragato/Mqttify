#pragma once

#include <atomic>

#include "Mqtt/Commands/MqttifyAcknowledgeable.h"
#include "Packets/MqttifyPublishPacket.h"


enum class EMqttifyReasonCode : uint8;

namespace Mqttify
{
	class FMqttifyPubRec final : public TMqttifyAcknowledgeable<void>
	{
	private:
		EMqttifyReasonCode ReasonCode;

		enum class EPubRecState : uint8
		{
			Unacknowledged,
			Released,
			Complete
		};

		EPubRecState PubRecState;

	public:
		FMqttifyPubRec(
			const uint16 InPacketId,
			const EMqttifyReasonCode InReasonCode,
			const TWeakPtr<IMqttifySocket>& InSocket,
			const FMqttifyConnectionSettingsRef& InConnectionSettings);

		void Abandon() override;
		bool Acknowledge(const FMqttifyPacketPtr& InPacket) override;

		/**
		 * @brief Get the key for the command
		 * @return Returns the key for the command. Which is the packet identifier. Bit shifted to the left by 16 bits.
		 */
		uint32 GetKey() const override
		{
			// The reason for the bit shift is we are using the first 16 bits for our packet identifier
			// and the last 16 bits for the identifiers generated by the broker.
			return static_cast<uint32>(PacketId) << 16;
		}

	protected:
		bool NextImpl() override;
		bool IsDone() const override;
		template <typename TPacket>
		TSharedPtr<TPacket> GetPacket() const;
	};

	template <typename TPacket>
	TSharedPtr<TPacket> FMqttifyPubRec::GetPacket() const
	{
		if constexpr (GMqttifyProtocol == EMqttifyProtocolVersion::Mqtt_3_1_1)
		{
			return MakeShared<TPacket>(PacketId);
		}
		else if constexpr (GMqttifyProtocol == EMqttifyProtocolVersion::Mqtt_5)
		{
			return MakeShared<TPacket>(PacketId, ReasonCode);
		}
	}
}
