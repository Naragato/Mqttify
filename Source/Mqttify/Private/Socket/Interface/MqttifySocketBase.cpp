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
		TArray<TSharedPtr<FArrayReader>> PacketsToDispatch;
		bool bShouldDisconnect = false;

		{
			FScopeLock Lock{&SocketAccessLock};

			int32 Available = DataBuffer.Num() - DataBufferReadOffset;
			while (Available > 1)
			{
				const uint8* Base = DataBuffer.GetData() + DataBufferReadOffset;
				uint32 RemainingLength = 0;
				uint32 Multiplier = 1;
				int32 Index = 1;
				bool bHaveRemainingLength = false;

				// Parse Remaining Length (MQTT varint, up to 4 bytes)
				for (; Index < 5; ++Index)
				{
					if (Index >= Available)
					{
						// Header not complete yet
						break;
					}

					const uint8 EncodedByte = Base[Index];
					RemainingLength += (EncodedByte & 127u) * Multiplier;
					Multiplier *= 128u;

					if ((EncodedByte & 128u) == 0)
					{
						bHaveRemainingLength = true;
						++Index;
						break;
					}
				}

				if (!bHaveRemainingLength)
				{
					LOG_MQTTIFY(VeryVerbose, TEXT("Header not complete yet"));
					break; // wait for more data
				}

				if (RemainingLength > ConnectionSettings->GetMaxPacketSize())
				{
					LOG_MQTTIFY(
						Error,
						TEXT("Packet too large: %d, Max: %d"),
						RemainingLength,
						ConnectionSettings->GetMaxPacketSize());
					bShouldDisconnect = true;
					break;
				}

				const int32 FixedHeaderSize = Index;
				const int32 TotalPacketSize = FixedHeaderSize + static_cast<int32>(RemainingLength);

				if (Available < TotalPacketSize)
				{
					// We don't have the full packet yet
					break;
				}

				TSharedPtr<FArrayReader> Packet = MakeShared<FArrayReader>(false);
				Packet->SetNumUninitialized(TotalPacketSize);
				FMemory::Memcpy(Packet->GetData(), Base, TotalPacketSize);
				LOG_MQTTIFY(VeryVerbose, TEXT("Packet prepared of size %d"), TotalPacketSize);

				PacketsToDispatch.Emplace(MoveTemp(Packet));

				// Advance read offset instead of removing from front
				DataBufferReadOffset += TotalPacketSize;
				Available -= TotalPacketSize;
			}

			// Periodic compaction to reclaim memory, it might be better to switch to a ring buffer in future
			if (DataBufferReadOffset > 0)
			{
				if (DataBufferReadOffset == DataBuffer.Num())
				{
					// All consumed; clear buffer cheaply
					DataBuffer.Reset();
					DataBufferReadOffset = 0;
				}
				else
				{
					constexpr int32 CompactMinBytes = 256 * 1024; // 256 KiB
					constexpr float CompactFraction = 0.75f;          // 75% of buffer
					const int32 ThresholdByFraction = static_cast<int32>(DataBuffer.Num() * CompactFraction);
					if (DataBufferReadOffset >= CompactMinBytes && DataBufferReadOffset >= ThresholdByFraction)
					{
						DataBuffer.RemoveAt(0, DataBufferReadOffset, EAllowShrinking::No);
						DataBufferReadOffset = 0;
					}
				}
			}
		}

		if (bShouldDisconnect)
		{
			Disconnect();
			return;
		}

		if (GetOnDataReceivedDelegate().IsBound())
		{
			for (const TSharedPtr<FArrayReader>& Packet : PacketsToDispatch)
			{
				GetOnDataReceivedDelegate().Broadcast(Packet);
			}
		}
	}
} // namespace Mqttify