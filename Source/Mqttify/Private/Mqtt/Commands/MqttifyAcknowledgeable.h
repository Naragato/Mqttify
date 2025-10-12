#pragma once

#include "CoreMinimal.h"
#include "MqttifyQueueable.h"
#include "Mqtt/MqttifyResult.h"
#include "Socket/Interface/MqttifySocketBase.h"
#include "Templates/SharedPointer.h"

#include <atomic>

enum class EMqttifyProtocolVersion : uint8;

namespace Mqttify
{
	/**
	 * @brief A wrapper for packets which require acknowledgement. With built-in retry logic.
	 */
	template <typename TReturnValue>
	class TMqttifyAcknowledgeable : public TMqttifyQueueable<TReturnValue>, public IMqttifyAcknowledgeable
	{
	protected:
		uint16 PacketId;

	public:
		explicit TMqttifyAcknowledgeable(
			const uint16 InPacketId,
			const TWeakPtr<FMqttifySocketBase>& InSocket,
			const FMqttifyConnectionSettingsRef& InConnectionSettings
			)
			: TMqttifyQueueable<TReturnValue>{InSocket, InConnectionSettings}
			, PacketId{InPacketId} {}

		virtual IMqttifyAcknowledgeable* AsAcknowledgeable() override
		{
			return this;
		}

		virtual uint32 GetId() const override
		{
			return PacketId;
		}
	};
} // namespace Mqttify
