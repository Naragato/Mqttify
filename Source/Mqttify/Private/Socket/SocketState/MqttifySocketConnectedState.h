#pragma once

#include "Socket/SocketState/IMqttifySocketDisconnectable.h"
#include "Socket/SocketState/IMqttifySocketSendable.h"
#include "Socket/SocketState/IMqttifySocketTickable.h"
#include "Socket/SocketState/MqttifySocketState.h"

class FArrayReader;

namespace Mqttify
{
	class FMqttifySocketConnectedState final : public FMqttifySocketState,
												public IMqttifySocketSendable,
												public IMqttifySocketTickable,
												public IMqttifySocketDisconnectable
	{
	public:
		explicit FMqttifySocketConnectedState(FUniqueSocket&& InSocket, FMqttifySocket* InMqttifySocket)
			: FMqttifySocketState{ MoveTemp(InSocket), InMqttifySocket }
			, Packet{ nullptr }
			, PacketSize{ 0 }
			, RemainingLength{ 0 } {}


		virtual void Tick() override;
		virtual void Send(const uint8* Data, uint32 Size) override;
		virtual void Disconnect() override;

		virtual IMqttifySocketDisconnectable* AsDisconnectable() override { return this; }
		virtual IMqttifySocketSendable* AsSendable() override { return this; }
		virtual IMqttifySocketTickable* AsTickable() override { return this; }

		virtual EMqttifySocketState GetState() const override { return EMqttifySocketState::Connected; }

	private:
		enum class EInitializeResult : uint8
		{
			Success,
			Failed,
			NotEnoughData
		};

		TSharedPtr<FArrayReader> Packet;
		int32 PacketSize;
		int32 RemainingLength;

		/**
		 * @brief Prints the received data to the log.
		 * @return true if the packet was successfully initialized, false otherwise
		 */
		EInitializeResult InitializePacket();

		void ReadPacket();
	};
} // namespace Mqttify
