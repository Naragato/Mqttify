#pragma once
#if WITH_DEV_AUTOMATION_TESTS

#include "Socket/Interface/MqttifySocketBase.h"
#include "Misc/AutomationTest.h"

namespace Mqttify
{
	class FFakeTestSocket final : public FMqttifySocketBase
	{
	public:
		explicit FFakeTestSocket(const FMqttifyConnectionSettingsRef& InConnectionSettings)
			: FMqttifySocketBase{InConnectionSettings}
			, bConnected(false)
		{
		}

		virtual ~FFakeTestSocket() override = default;

		virtual void Connect() override
		{
			bConnected = true;
			OnConnectDelegate.Broadcast(true);
		}

		virtual void Disconnect() override
		{
			if (bConnected)
			{
				bConnected = false;
				OnDisconnectDelegate.Broadcast();
			}
		}

		virtual void Close(int32 /*Code*/ = 1000, const FString& /*Reason*/ = FString()) override
		{
			Disconnect();
		}

		virtual bool IsConnected() const override
		{
			return bConnected;
		}

		virtual void Tick() override {}

		virtual void Send(const TSharedRef<IMqttifyControlPacket>& InPacket) override
		{
			// Encode and keep the bytes for assertions in tests
			LastSentBytes.Reset();
			FMemoryWriter Writer(LastSentBytes);
			Writer.SetByteSwapping(true);
			InPacket->Encode(Writer);
		}

		const TArray<uint8>& GetLastSentBytes() const { return LastSentBytes; }

	protected:
		virtual void Send(const uint8* /*Data*/, uint32 /*Size*/) override {}

	private:
		bool bConnected;
		TArray<uint8> LastSentBytes;
	};
}

#endif // WITH_DEV_AUTOMATION_TESTS
