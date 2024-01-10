#pragma once

#include "CoreMinimal.h"
#include "IWebSocket.h"
#include "Mqtt/MqttifyConnectionSettings.h"
#include "Socket/Interface/IMqttifySocket.h"


namespace Mqttify
{
	enum class EMqttifySocketState;

	class FMqttifyWebSocket final : public IMqttifySocket
	{
	private:
		const FMqttifyConnectionSettingsRef ConnectionSettings;
		TSharedPtr<IWebSocket> Socket;
		EMqttifySocketState CurrentState;
		FDateTime DisconnectTime;
		TSharedPtr<FArrayReader> Packet;

	public:
		explicit FMqttifyWebSocket(const FMqttifyConnectionSettingsRef& InConnectionSettings);

		~FMqttifyWebSocket() override { Disconnect(); }
		void Connect() override;
		void Disconnect() override;
		void Send(const uint8* Data, uint32 Size) override;
		void Tick() override;
		bool IsConnected() const override;

	private:
		void HandleWebSocketConnected();
		void HandleWebSocketConnectionError(const FString& Error);
		void HandleWebSocketConnectionClosed(int32 Status, const FString& Reason, bool bWasClean);
		void HandleWebSocketData(const void* Data, SIZE_T Length, SIZE_T BytesRemaining);
		void FinalizeDisconnect();
	};
} // namespace Mqttify
