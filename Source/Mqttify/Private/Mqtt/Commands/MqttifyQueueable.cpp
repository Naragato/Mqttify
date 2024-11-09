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
			Abandon();
			return true;
		}

		return NextImpl();
	}

	void FMqttifyQueueable::SendPacketInternal(const TSharedPtr<IMqttifyControlPacket>& InPacket)
	{
		if (!InPacket.IsValid())
		{
			return;
		}
		if (const TSharedPtr<FMqttifySocketBase> PinnedSocket = Socket.Pin())
		{
			if (!PinnedSocket->IsConnected())
			{
				return;
			}
			++PacketTries;
			LOG_MQTTIFY_PACKET_REF(
				VeryVerbose,
				TEXT( "(Connection %s, ClientId %s) Sending %s, Attempt %d"),
				InPacket.ToSharedRef(),
				*Settings->GetHost(),
				*Settings->GetClientId(),
				EnumToTCharString(InPacket->GetPacketType()),
				PacketTries);
			TArray<uint8> ActualBytes;
			FMemoryWriter Writer(ActualBytes);
			InPacket->Encode(Writer);
			PinnedSocket->Send(ActualBytes.GetData(), ActualBytes.Num());

			if (!PinnedSocket->IsConnected())
			{
				LOG_MQTTIFY_PACKET_REF(
					Error,
					TEXT( "(Connection %s, ClientId %s) Socket disconnected while sending packet"),
					InPacket.ToSharedRef(),
					*Settings->GetHost(),
					*Settings->GetClientId());
				Abandon();
			}

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
