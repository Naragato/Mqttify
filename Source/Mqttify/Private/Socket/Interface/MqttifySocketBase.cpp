#include "MqttifySocketBase.h"

#include "LogMqttify.h"
#include "MqttifyConstants.h"
#include "Async/Async.h"
#include "Socket/MqttifySecureSocket.h"
#include "Socket/MqttifyWebSocket.h"

namespace Mqttify
{
	FMqttifySocketBase::~FMqttifySocketBase()
	{
		FScopeLock Lock{&SocketAccessLock};
		OnConnectDelegate.Clear();
		OnDisconnectDelegate.Clear();
		OnDataReceiveDelegate.Clear();
	}

	FMqttifySocketRef FMqttifySocketBase::Create(const FMqttifyConnectionSettingsRef& InConnectionSettings)
	{

		switch (InConnectionSettings->GetTransportProtocol())
		{
			case EMqttifyConnectionProtocol::Mqtt:
			case EMqttifyConnectionProtocol::Mqtts:
				return MakeShared<FMqttifySecureSocket>(InConnectionSettings);
			case EMqttifyConnectionProtocol::Ws:
			case EMqttifyConnectionProtocol::Wss:
			default:
				return MakeShared<FMqttifyWebSocket>(InConnectionSettings);
		}
	}

	void FMqttifySocketBase::ReadPacketsFromBuffer()
	{
		while (DataBuffer.Num() >= 1)
		{
			constexpr int32 PacketStartIndex = 0;
			uint32 RemainingLength           = 0;
			uint32 Multiplier                = 1;
			int32 Index                      = 1;
			bool bHaveRemainingLength        = false;

			for (; Index < 5; ++Index)
			{
				if (Index >= DataBuffer.Num())
				{
					return;
				}

				const uint8 EncodedByte = DataBuffer[Index];
				RemainingLength += (EncodedByte & 127) * Multiplier;
				Multiplier *= 128;

				if ((EncodedByte & 128) == 0)
				{
					bHaveRemainingLength = true;
					++Index;
					break;
				}
			}

			if (!bHaveRemainingLength)
			{
				LOG_MQTTIFY(VeryVerbose, TEXT("Header not complete yet"));
				return;
			}

			if (RemainingLength > ConnectionSettings->GetMaxPacketSize())
			{
				LOG_MQTTIFY(
					Error,
					TEXT("Packet too large: %d, Max: %d"),
					RemainingLength,
					ConnectionSettings->GetMaxPacketSize());
				Disconnect();
				return;
			}

			const int32 FixedHeaderSize = Index;
			const int32 TotalPacketSize = FixedHeaderSize + RemainingLength;

			if (DataBuffer.Num() < TotalPacketSize)
			{
				// We don't have the full packet yet
				return;
			}

			TSharedPtr<FArrayReader> Packet = MakeShared<FArrayReader>(false);
			Packet->SetNumUninitialized(TotalPacketSize);
			FMemory::Memcpy(Packet->GetData(), DataBuffer.GetData(), TotalPacketSize);

			DataBuffer.RemoveAt(0, TotalPacketSize, EAllowShrinking::No);

			if (GetOnDataReceivedDelegate().IsBound())
			{
				GetOnDataReceivedDelegate().Broadcast(Packet);
			}

			LOG_MQTTIFY(VeryVerbose, TEXT("Packet received of size %d"), TotalPacketSize);
		}
	}
} // namespace Mqttify
