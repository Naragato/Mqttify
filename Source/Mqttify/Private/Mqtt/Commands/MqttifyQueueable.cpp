#include "Mqtt/Commands/MqttifyQueueable.h"

#include "LogMqttify.h"
#include "Packets/Interface/IMqttifyControlPacket.h"
#include "Serialization/MemoryWriter.h"

namespace Mqttify
{
	bool FMqttifyQueueable::Next()
	{
		FScopeLock Lock{&CriticalSection};

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
			LOG_MQTTIFY(
				Warning,
				TEXT( " (Connection %s, ClientId %s) Packet retry limit reached, abandoning packet" ),
				*Settings->GetHost(),
				*Settings->GetClientId());
			Abandon();
			return true;
		}

		return NextImpl();
	}

	void FMqttifyQueueable::SendPacketInternal(const TSharedPtr<IMqttifyControlPacket>& InPacket)
	{
		if (const TSharedPtr<FMqttifySocketBase> PinnedSocket = Socket.Pin())
		{
			++PacketTries;
			LOG_MQTTIFY(
				VeryVerbose,
				TEXT( "(Connection %s, ClientId %s) Sending %s, Attempt %d"),
				*Settings->GetHost(),
				*Settings->GetClientId(),
				EnumToTCharString(InPacket->GetPacketType()),
				PacketTries);
			TArray<uint8> ActualBytes;
			FMemoryWriter Writer(ActualBytes);
			InPacket->Encode(Writer);
			PinnedSocket->Send(ActualBytes.GetData(), ActualBytes.Num());

			const double Jitter = FMath::RandRange(0.0, 1.0);
			const double Backoff = FMath::Pow(Settings->GetPacketRetryBackoffMultiplier(), PacketTries);
			const double RetryInterval = FMath::Min(
				Settings->GetInitialRetryIntervalSeconds() * Backoff,
				Settings->GetMaxPacketRetryIntervalSeconds());
			RetryWaitTime = FDateTime::UtcNow() + FTimespan::FromSeconds(RetryInterval + Jitter);
			LOG_MQTTIFY(
				VeryVerbose,
				TEXT( "(Connection %s, ClientId %s) Retry interval %f, Retry wait time %s"),
				*Settings->GetHost(),
				*Settings->GetClientId(),
				RetryInterval,
				*RetryWaitTime.ToString());
		}
	}
}
