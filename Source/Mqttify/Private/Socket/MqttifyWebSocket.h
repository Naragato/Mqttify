#pragma once

#include "CoreMinimal.h"
#include "IWebSocket.h"
#include "Mqtt/MqttifyConnectionSettings.h"
#include "Socket/Interface/MqttifySocketBase.h"


namespace Mqttify
{
	enum class EMqttifySocketState;

	class FMqttifyWebSocket final : public FMqttifySocketBase, public TSharedFromThis<FMqttifyWebSocket> 
	{
	private:
		const FMqttifyConnectionSettingsRef ConnectionSettings;
		TSharedPtr<IWebSocket> Socket;
		EMqttifySocketState CurrentState;
		FDateTime DisconnectTime;
		TSharedPtr<FArrayReader> Packet;

	public:
		explicit FMqttifyWebSocket(const FMqttifyConnectionSettingsRef& InConnectionSettings);

		virtual ~FMqttifyWebSocket() override { Disconnect_Internal(); }
		virtual void Connect() override;
		virtual void Disconnect() override;
		virtual void Send(const uint8* Data, uint32 Size) override;
		virtual void Tick() override;
		virtual bool IsConnected() const override;

	private:
		void Disconnect_Internal();
		void HandleWebSocketConnected();
		void HandleWebSocketConnectionError(const FString& Error);
		void HandleWebSocketConnectionClosed(int32 Status, const FString& Reason, bool bWasClean);
		void HandleWebSocketData(const void* Data, SIZE_T Length, SIZE_T BytesRemaining);
		void FinalizeDisconnect();
	};
} // namespace Mqttify
