#include "Socket/SocketState/MqttifySocketConnectedState.h"

#include "LogMqttify.h"
#include "MqttifySocketDisconnectingState.h"
#include "Sockets.h"
#include "Serialization/ArrayReader.h"
#include "Socket/MqttifySocket.h"

namespace Mqttify
{
	void FMqttifySocketConnectedState::Tick()
	{
		uint32 Size = 0;
		if (IsConnected() && Socket->HasPendingData(Size))
		{
			switch (InitializePacket())
			{
				case EInitializeResult::Success:
					ReadPacket();
					break;
				case EInitializeResult::NotEnoughData:
					break;
				case EInitializeResult::Failed:
				default:
					TransitionTo(MakeUnique<FMqttifySocketDisconnectingState>(MoveTemp(Socket), MqttifySocket));
			}
		}
	}

	void FMqttifySocketConnectedState::ReadPacket()
	{
		int32 Read = 0;
		TArray<uint8> Data;
		const int32 ViewCount = PacketSize - Packet->Tell();
		const TArrayView<uint8> PacketView(&Packet->GetData()[Packet->Tell()], ViewCount);

		if (Socket->Recv(PacketView.GetData(), ViewCount, Read))
		{
			LOG_MQTTIFY_PACKET_DATA(VeryVerbose, PacketView.GetData(), Read);
			Packet->Seek(Packet->Tell() + Read);
		}

		if (Packet->Tell() == PacketSize)
		{
			LOG_MQTTIFY(Verbose, TEXT("Packet received."));
			Packet->Seek(0);
			MqttifySocket->GetOnDataReceivedDelegate().Broadcast(Packet);
			Packet.Reset();
			PacketSize      = 0;
			RemainingLength = 0;
		}
	}

	FMqttifySocketConnectedState::EInitializeResult FMqttifySocketConnectedState::InitializePacket()
	{
		if (nullptr != Packet)
		{
			return EInitializeResult::Success;
		}

		int32 Read = 0;
		TArray<uint8> Data;

		Data.SetNumUninitialized(5);
		const bool bStillAlive = Socket->Recv(Data.GetData(), Data.Num(), Read, ESocketReceiveFlags::Peek);
		if (!bStillAlive)
		{
			LOG_MQTTIFY(Error, TEXT("Socket is not alive."));
			return EInitializeResult::Failed;
		}

		// Get remaining length from the last 4 bytes
		RemainingLength           = 0;
		uint32 Multiplier         = 1;
		int32 i                   = 1;
		bool bHaveRemainingLength = false;
		for (; i < 5; ++i)
		{
			RemainingLength += (Data[i] & 127) * Multiplier;
			Multiplier *= 128;
			// check if the last byte has the MSB set to 0
			if ((Data[i] & 128) == 0 || i == 4)
			{
				bHaveRemainingLength = true;
				break;
			}
		}

		if (!bHaveRemainingLength)
		{
			LOG_MQTTIFY(Verbose, TEXT("Header not complete yet."));
			return EInitializeResult::NotEnoughData;
		}

		PacketSize = RemainingLength + i + 1;
		Packet     = MakeShared<FArrayReader>(false);
		Packet->SetNumUninitialized(PacketSize);

		return EInitializeResult::Success;
	}

	void FMqttifySocketConnectedState::Send(const uint8* Data, const uint32 Size)
	{
		LOG_MQTTIFY_PACKET_DATA(VeryVerbose, Data, Size);
		if (!IsConnected())
		{
			LOG_MQTTIFY(Error, TEXT("Socket is not connected."));
			TransitionTo(MakeUnique<FMqttifySocketDisconnectingState>(MoveTemp(Socket), MqttifySocket));
			return;
		}

		int32 BytesSent = 0;
		Socket->Send(Data, Size, BytesSent);

		if (BytesSent != Size)
		{
			LOG_MQTTIFY(
				Error,
				TEXT("Failed to send data. Bytes sent: %d, Size: %d."),
				BytesSent,
				Size);
			TransitionTo(MakeUnique<FMqttifySocketDisconnectingState>(MoveTemp(Socket), MqttifySocket));
		}
	}

	void FMqttifySocketConnectedState::Disconnect()
	{
		TransitionTo(MakeUnique<FMqttifySocketDisconnectingState>(MoveTemp(Socket), MqttifySocket));
	}
} // namespace Mqttify
