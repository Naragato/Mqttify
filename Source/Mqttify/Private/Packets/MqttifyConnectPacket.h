#pragma once

#include "Mqtt/MqttifyConnectReturnCode.h"
#include "Mqtt/MqttifyProtocolVersion.h"
#include "Mqtt/MqttifyQualityOfService.h"
#include "Mqtt/MqttifyReasonCode.h"
#include "Packets/MqttifyFixedHeader.h"
#include "Packets/Interface/MqttifyControlPacket.h"
#include "Properties/MqttifyProperties.h"
#include "Serialization/ArrayReader.h"

namespace Mqttify
{
	struct FMqttifyConnectPacketBase
		: TMqttifyControlPacket<EMqttifyPacketType::Connect>
	{
	public:
		explicit FMqttifyConnectPacketBase(const FMqttifyFixedHeader& InFixedHeader)
			: TMqttifyControlPacket{ InFixedHeader }
			, bCleanSession{ false }
			, bRetainWill{ false }
			, WillQoS{} {}

		explicit FMqttifyConnectPacketBase(const FString& InClientId,
											const uint16 InKeepAliveSeconds,
											const FString& InUserName,
											const FString& InPassword,
											const bool bInCleanSession,
											const bool bInRetainWill,
											const FString& InWillTopic,
											const FString& InWillMessage,
											const EMqttifyQualityOfService& WillQoS)
			: ClientId{ InClientId }
			, KeepAliveSeconds{ InKeepAliveSeconds }
			, UserName{ InUserName }
			, Password{ InPassword }
			, bCleanSession{ bInCleanSession }
			, bRetainWill{ bInRetainWill }
			, WillTopic{ InWillTopic }
			, WillMessage{ InWillMessage }
			, WillQoS{ WillQoS } {}

	public:
		/**
		 * @brief Get the client id.
		 * @return The client id.
		 */
		const FString& GetClientId() const { return ClientId; }

		/**
		 * @brief Get the keep alive seconds.
		 * @return The keep alive seconds.
		 */
		uint16 GetKeepAliveSeconds() const { return KeepAliveSeconds; }

		/**
		 * @brief Get the user name.
		 * @return The user name.
		 */
		const FString& GetUserName() const { return UserName; }

		/**
		 * @brief Get the password.
		 * @return The password.
		 */
		const FString& GetPassword() const { return Password; }

		/**
		 * @brief Get the clean session flag.
		 * @return The clean session flag.
		 */
		bool GetCleanSession() const { return bCleanSession; }

		/**
		 * @brief Get the retain will flag.
		 * @return The retain will flag.
		 */
		bool GetRetainWill() const { return bRetainWill; }

		/**
		 * @brief Get the will topic.
		 * @return The will topic.
		 */
		const FString& GetWillTopic() const { return WillTopic; }

		/**
		 * @brief Get the will message.
		 * @return The will message.
		 */
		const FString& GetWillMessage() const { return WillMessage; }

		/**
		 * @brief Get the will QoS.
		 * @return The will QoS.
		 */
		EMqttifyQualityOfService GetWillQoS() const { return WillQoS; }

		static constexpr TCHAR InvalidProtocolVersion[]     = TEXT("Invalid protocol version.");
		static constexpr TCHAR InvalidProtocolName[]        = TEXT("Invalid protocol name.");
		static constexpr TCHAR TopicEmpty[]                 = TEXT("Topic is empty.");
		static constexpr TCHAR WillTopicEmpty[]             = TEXT("Will topic is empty.");
		static constexpr TCHAR WillMessageEmpty[]           = TEXT("Will message is empty.");
		static constexpr TCHAR WillRetainMustBe0WithFlag0[] = TEXT("Will retain must be 0 when will flag is 0.");
		static constexpr TCHAR WillQosMustBe0WithFlag0[]    = TEXT("Will QoS must be 0 when will flag is 0.");

	protected:
		FString ClientId;
		uint16 KeepAliveSeconds = 60;
		FString UserName;
		FString Password;
		bool bCleanSession;
		bool bRetainWill;
		FString WillTopic;
		FString WillMessage;
		EMqttifyQualityOfService WillQoS;
	};

	template <EMqttifyProtocolVersion InProtocolVersion>
	struct TMqttifyConnectPacket;

	template <>
	struct TMqttifyConnectPacket<EMqttifyProtocolVersion::Mqtt_3_1_1> final : FMqttifyConnectPacketBase
	{
		explicit TMqttifyConnectPacket(const FString& InClientId                = {},
										const uint16 InKeepAliveSeconds         = 60,
										const FString& InUserName               = {},
										const FString& InPassword               = {},
										const bool bInCleanSession              = false,
										const bool bInRetainWill                = false,
										const FString& InWillTopic              = {},
										const FString& InWillMessage            = {},
										const EMqttifyQualityOfService& WillQoS = EMqttifyQualityOfService::AtMostOnce)
			: FMqttifyConnectPacketBase{
				InClientId,
				InKeepAliveSeconds,
				InUserName,
				InPassword,
				bInCleanSession,
				bInRetainWill,
				InWillTopic,
				InWillMessage,
				WillQoS }
		{
			FixedHeader = FMqttifyFixedHeader::Create(this);
		}

		explicit TMqttifyConnectPacket(FArrayReader& InReader, const FMqttifyFixedHeader& InFixedHeader)
			: FMqttifyConnectPacketBase{ InFixedHeader }
		{
			Decode(InReader);
		}

		virtual uint32 GetLength() const override;
		virtual void Encode(FMemoryWriter& InWriter) override;
		virtual void Decode(FArrayReader& InReader) override;

		/**
		 * @brief Get the return code.
		 * @return The return code.
		 */
		EMqttifyConnectReturnCode GetReturnCode() const { return ReturnCode; }

	protected:
		EMqttifyConnectReturnCode ReturnCode = EMqttifyConnectReturnCode::Accepted;
	};

	template <>
	struct TMqttifyConnectPacket<EMqttifyProtocolVersion::Mqtt_5> final : FMqttifyConnectPacketBase
	{
	public:
		explicit TMqttifyConnectPacket(const FString& InClientId = {},
										const uint16 InKeepAliveSeconds = 60,
										const FString& InUserName = {},
										const FString& InPassword = {},
										const bool bInCleanSession = false,
										const bool bInRetainWill = false,
										const FString& InWillTopic = {},
										const FString& InWillMessage = {},
										const EMqttifyQualityOfService& WillQoS = EMqttifyQualityOfService::AtMostOnce,
										const FMqttifyProperties& InWillProperties = FMqttifyProperties{},
										const FMqttifyProperties& InProperties = FMqttifyProperties{})
			: FMqttifyConnectPacketBase{
				InClientId,
				InKeepAliveSeconds,
				InUserName,
				InPassword,
				bInCleanSession,
				bInRetainWill,
				InWillTopic,
				InWillMessage,
				WillQoS }
			, WillProperties{ InWillProperties }
			, Properties{ InProperties }
		{
			FixedHeader = FMqttifyFixedHeader::Create(this);
		}

		explicit TMqttifyConnectPacket(FArrayReader& InReader, const FMqttifyFixedHeader& InFixedHeader)
			: FMqttifyConnectPacketBase{ InFixedHeader }
		{
			Decode(InReader);
		}

		virtual uint32 GetLength() const override;

		/**
		 * @brief Get the will properties.
		 * @return The will properties.
		 */
		const FMqttifyProperties& GetWillProperties() const { return WillProperties; }

		/**
		 * @brief Get the connect properties.
		 * @return The connect properties.
		 */
		const FMqttifyProperties& GetProperties() const { return Properties; }
		virtual void Encode(FMemoryWriter& InWriter) override;
		virtual void Decode(FArrayReader& InReader) override;

		/**
		 * @brief Get the reason code.
		 * @return The reason code.
		 */
		EMqttifyReasonCode GetReasonCode() const { return ReasonCode; }

	protected:
		FMqttifyProperties WillProperties;
		FMqttifyProperties Properties;
		EMqttifyReasonCode ReasonCode = EMqttifyReasonCode::Success;
	};

	using FMqttifyConnectPacket3 = TMqttifyConnectPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>;
	using FMqttifyConnectPacket5 = TMqttifyConnectPacket<EMqttifyProtocolVersion::Mqtt_5>;
} // namespace Mqttify
