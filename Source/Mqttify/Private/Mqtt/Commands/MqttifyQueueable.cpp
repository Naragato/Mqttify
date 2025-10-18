#include "Mqtt/Commands/MqttifyQueueable.h"

#include "LogMqttify.h"
#include "Packets/Interface/IMqttifyControlPacket.h"

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

	void FMqttifyQueueable::SendPacketInternal(const TSharedRef<IMqttifyControlPacket>& InPacket)
	{

		if (const TSharedPtr<FMqttifySocketBase> PinnedSocket = Socket.Pin())
		{
			if (!PinnedSocket->IsConnected())
			{
				return;
			}
			++PacketTries;
			LOG_MQTTIFY_PACKET(
				VeryVerbose,
				TEXT( "(Connection %s, ClientId %s) Sending %s, Attempt %d"),
				InPacket,
				*Settings->GetHost(),
				*Settings->GetClientIdRef(),
				EnumToTCharString(InPacket->GetPacketType()),
				PacketTries);

			PinnedSocket->Send(InPacket);

			if (!PinnedSocket->IsConnected())
			{
				LOG_MQTTIFY_PACKET(
					Error,
					TEXT( "(Connection %s, ClientId %s) Socket disconnected while sending packet"),
					InPacket,
					*Settings->GetHost(),
					*Settings->GetClientIdRef());
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