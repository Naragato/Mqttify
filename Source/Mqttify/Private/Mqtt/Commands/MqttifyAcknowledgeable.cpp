#include "Mqtt/Commands/MqttifyAcknowledgeable.h"

#include "LogMqttify.h"
#include "Packets/Interface/IMqttifyControlPacket.h"
#include "Serialization/MemoryWriter.h"

namespace Mqttify
{
	bool FMqttifyAcknowledgeable::Next()
	{
		FScopeLock Lock{ &CriticalSection };

		if (IsDone())
		{
			return true;
		}

		if (RetryWaitTime > FDateTime::UtcNow())
		{
			return false;
		}

		if (PacketTries >= Settings->GetMaxPacketRetries())
		{
			LOG_MQTTIFY(Warning, TEXT("Packet retry limit reached, abandoning packet."));
			Abandon();
			return true;
		}

		return NextImpl();
	}

	void FMqttifyAcknowledgeable::SendPacketInternal(const TSharedPtr<IMqttifyControlPacket>& InPacket)
	{
		if (const TSharedPtr<IMqttifySocket> PinnedSocket = Socket.Pin())
		{
			++PacketTries;
			TArray<uint8> ActualBytes;
			FMemoryWriter Writer(ActualBytes);
			InPacket->Encode(Writer);
			PinnedSocket->Send(ActualBytes.GetData(), ActualBytes.Num());
			RetryWaitTime = FDateTime::UtcNow() + FTimespan::FromSeconds(Settings->GetPacketRetryIntervalSeconds());
		}
	}
}
