#include "MqttifyConnectPacket.h"

#include "MqttifyConnAckPacket.h"
#include "Serialization/MqttifyFArchiveEncodeDecode.h"

namespace Mqttify
{
#pragma region MQTT 3.1.1

	uint32 TMqttifyConnectPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>::GetLength() const
	{
		constexpr uint32 VariableHeaderLength = 10;
		uint32 PayloadLength                  = 0;

		PayloadLength += StringLengthFieldSize + ClientId.Len();
		PayloadLength += WillTopic.IsEmpty()
			? 0
			: StringLengthFieldSize + WillTopic.Len() + StringLengthFieldSize + WillMessage.Len();
		PayloadLength += Username.IsEmpty() ? 0 : StringLengthFieldSize + Username.Len();
		PayloadLength += Password.IsEmpty() ? 0 : StringLengthFieldSize + Password.Len();

		return VariableHeaderLength + PayloadLength;
	}

	void TMqttifyConnectPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>::Encode(FMemoryWriter& InWriter)
	{
		LOG_MQTTIFY(VeryVerbose, TEXT("[Encode][Connect]"));
		InWriter.SetByteSwapping(true);
		FixedHeader.Encode(InWriter);
		Data::EncodeString("MQTT", InWriter);

		uint8 ProtocolLevel = static_cast<uint8>(EMqttifyProtocolVersion::Mqtt_3_1_1); // MQTT 3.11
		InWriter << ProtocolLevel;

		uint8 Flags = (bCleanSession & 0x1) << 1;

		const bool bHasWill = !WillTopic.IsEmpty();
		if (bHasWill)
		{
			Flags = Flags | ((static_cast<int32>(WillQoS) & 0x3) << 3 | (bHasWill & 0x1) << 2);
			if (bRetainWill)
			{
				Flags |= (bRetainWill & 0x1) << 5;
			}
		}

		const bool bHasUserName = !Username.IsEmpty();
		if (bHasUserName)
		{
			Flags |= 0x1 << 7;
		}

		const bool bHasPassword = !Password.IsEmpty();
		if (bHasPassword)
		{
			Flags |= 0x1 << 6;
		}

		InWriter << Flags;
		InWriter << KeepAliveSeconds;

		const bool bHasClientId = !ClientId.IsEmpty();
		if (bHasClientId)
		{
			Data::EncodeString(ClientId, InWriter);
		}
		else
		{
			static uint16 Zero = 0;
			InWriter << Zero;
		}

		if (bHasWill)
		{
			Data::EncodeString(WillTopic, InWriter);
			Data::EncodeString(WillMessage, InWriter);
		}

		if (bHasUserName)
		{
			Data::EncodeString(Username, InWriter);
		}

		if (bHasPassword)
		{
			Data::EncodeString(Password, InWriter);
		}
	}

	void TMqttifyConnectPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>::Decode(FArrayReader& InReader)
	{
		LOG_MQTTIFY(VeryVerbose, TEXT("[Decode][Connect]"));
		InReader.SetByteSwapping(true);
		if (FixedHeader.GetPacketType() != EMqttifyPacketType::Connect)
		{
			LOG_MQTTIFY(
				Error,
				TEXT("[Connect] %s %s"),
				MqttifyPacketType::InvalidPacketType,
				EnumToTCharString(FixedHeader.GetPacketType()));
			ReturnCode = EMqttifyConnectReturnCode::RefusedProtocolVersion;
			bIsValid   = false;
			return;
		}

		const FString ProtocolName = Data::DecodeString(InReader);
		if (ProtocolName != "MQTT")
		{
			LOG_MQTTIFY(Error, TEXT("[Connect] %s %s"), InvalidProtocolName, *ProtocolName);
			ReturnCode = EMqttifyConnectReturnCode::RefusedProtocolVersion;
			bIsValid   = false;
			return;
		}

		uint8 ProtocolLevel = 0;
		InReader << ProtocolLevel;
		if (ProtocolLevel != static_cast<uint8>(EMqttifyProtocolVersion::Mqtt_3_1_1))
		{
			LOG_MQTTIFY(Error, TEXT("[Connect] %s %d"), InvalidProtocolVersion, ProtocolLevel);
			ReturnCode = EMqttifyConnectReturnCode::RefusedProtocolVersion;
			bIsValid   = false;
			return;
		}

		uint8 TempFlags = 0;
		InReader << TempFlags;

		bCleanSession = TempFlags >> 1 & 1;

		// Keep alive
		InReader << KeepAliveSeconds;

		// Client ID
		ClientId = Data::DecodeString(InReader);

		// Check Will Flag
		if (TempFlags & 0b00001000)
		{
			// Check will QoS
			WillQoS = static_cast<EMqttifyQualityOfService>(TempFlags >> 3 & 0b00000011);
			// Check will retain
			bRetainWill = TempFlags >> 5 & 1;

			// Will topic
			WillTopic = Data::DecodeString(InReader);

			// Will message
			WillMessage = Data::DecodeString(InReader);

			if (WillTopic.IsEmpty())
			{
				LOG_MQTTIFY(Error, TEXT("[Connect] %s."), WillTopicEmpty);
				ReturnCode = EMqttifyConnectReturnCode::RefusedProtocolVersion;
				bIsValid   = false;
				return;
			}

			if (WillMessage.IsEmpty())
			{
				LOG_MQTTIFY(Error, TEXT("[Connect] %s."), WillMessageEmpty);
				ReturnCode = EMqttifyConnectReturnCode::RefusedProtocolVersion;
				bIsValid   = false;
				return;
			}
		}
		else
		{
			// Check will QoS is 0
			if (TempFlags >> 3 & 0b00000011)
			{
				LOG_MQTTIFY(Error, TEXT("[Connect] %s."), WillQosMustBe0WithFlag0);
				ReturnCode = EMqttifyConnectReturnCode::RefusedProtocolVersion;
				bIsValid   = false;
				return;
			}
			// Check will retain is 0
			if (TempFlags >> 5 & 1)
			{
				LOG_MQTTIFY(Error, TEXT("[Connect] %s."), WillRetainMustBe0WithFlag0);
				ReturnCode = EMqttifyConnectReturnCode::RefusedProtocolVersion;
				bIsValid   = false;
				return;
			}
		}

		// Username
		if (TempFlags & 0b10000000)
		{
			Username = Data::DecodeString(InReader);
		}

		if (TempFlags & 0b01000000)
		{
			Password = Data::DecodeString(InReader);
		}
	}
#pragma endregion MQTT 3.1.1

#pragma region MQTT 5

	uint32 TMqttifyConnectPacket<EMqttifyProtocolVersion::Mqtt_5>::GetLength() const
	{
		constexpr uint32 VariableHeaderLength = 10;
		uint32 PayloadLength                  = 0;

		PayloadLength += StringLengthFieldSize + ClientId.Len();
		PayloadLength += WillTopic.IsEmpty()
			? 0
			: StringLengthFieldSize + WillTopic.Len() + StringLengthFieldSize + WillMessage.Len();
		PayloadLength += Username.IsEmpty() ? 0 : StringLengthFieldSize + Username.Len();
		PayloadLength += Password.IsEmpty() ? 0 : StringLengthFieldSize + Password.Len();
		PayloadLength += Properties.GetLength();

		if (!WillTopic.IsEmpty())
		{
			PayloadLength += WillProperties.GetLength();
		}

		return VariableHeaderLength + PayloadLength;
	}

	void TMqttifyConnectPacket<EMqttifyProtocolVersion::Mqtt_5>::Encode(FMemoryWriter& InWriter)
	{
		LOG_MQTTIFY(VeryVerbose, TEXT("[Encode][Connect]"));
		InWriter.SetByteSwapping(true);

		FixedHeader.Encode(InWriter);
		Data::EncodeString("MQTT", InWriter);
		uint8 ProtocolLevel = static_cast<uint8>(EMqttifyProtocolVersion::Mqtt_5); // MQTT 5
		InWriter << ProtocolLevel;

		uint8 Flags = (bCleanSession & 0x1) << 1;

		const bool bHasWill = !WillTopic.IsEmpty();
		if (bHasWill)
		{
			Flags = Flags | ((static_cast<int32>(WillQoS) & 0x3) << 3 | (bHasWill & 0x1) << 2);
			if (bRetainWill)
			{
				Flags |= (bRetainWill & 0x1) << 5;
			}
		}

		const bool bHasUserName = !Username.IsEmpty();
		if (bHasUserName)
		{
			Flags |= 0x1 << 7;
		}

		const bool bHasPassword = !Password.IsEmpty();
		if (bHasPassword)
		{
			Flags |= 0x1 << 6;
		}

		InWriter << Flags;
		InWriter << KeepAliveSeconds;
		Properties.Encode(InWriter);

		const bool bHasClientId = !ClientId.IsEmpty();
		if (bHasClientId)
		{
			Data::EncodeString(ClientId, InWriter);
		}
		else
		{
			static uint16 Zero = 0;
			InWriter << Zero;
		}

		if (bHasWill)
		{
			WillProperties.Encode(InWriter);
			Data::EncodeString(WillTopic, InWriter);
			Data::EncodeString(WillMessage, InWriter);
		}

		if (bHasUserName)
		{
			Data::EncodeString(Username, InWriter);
		}

		if (bHasPassword)
		{
			Data::EncodeString(Password, InWriter);
		}
	}

	void TMqttifyConnectPacket<EMqttifyProtocolVersion::Mqtt_5>::Decode(FArrayReader& InReader)
	{
		LOG_MQTTIFY(VeryVerbose, TEXT("[Decode][Connect]"));
		InReader.SetByteSwapping(true);
		if (FixedHeader.GetPacketType() != EMqttifyPacketType::Connect)
		{
			LOG_MQTTIFY(
				Error,
				TEXT("[Connect] %s %s."),
				MqttifyPacketType::InvalidPacketType,
				EnumToTCharString(FixedHeader.GetPacketType()));
			ReasonCode = EMqttifyReasonCode::MalformedPacket;
			bIsValid   = false;
			return;
		}

		const FString ProtocolName = Data::DecodeString(InReader);
		if (ProtocolName != "MQTT")
		{
			LOG_MQTTIFY(Error, TEXT("[Connect] %s %s."), InvalidProtocolName, *ProtocolName);
			ReasonCode = EMqttifyReasonCode::UnsupportedProtocolVersion;
			bIsValid   = false;
			return;
		}

		uint8 ProtocolLevel = 0;
		InReader << ProtocolLevel;
		if (ProtocolLevel != static_cast<uint8>(EMqttifyProtocolVersion::Mqtt_5))
		{
			LOG_MQTTIFY(Error, TEXT("[Connect] %s %d."), InvalidProtocolVersion, ProtocolLevel);
			ReasonCode = EMqttifyReasonCode::UnsupportedProtocolVersion;
			bIsValid   = false;
			return;
		}

		uint8 TempFlags = 0;
		InReader << TempFlags;

		bCleanSession = TempFlags >> 1 & 1;

		// Keep alive
		InReader << KeepAliveSeconds;

		// Properties
		Properties.Decode(InReader);

		// Client ID
		ClientId = Data::DecodeString(InReader);

		// Check Will Flag
		if (TempFlags & 0b00001000)
		{
			// Check will QoS
			WillQoS = static_cast<EMqttifyQualityOfService>(TempFlags >> 3 & 0b00000011);
			// Check will retain
			bRetainWill = TempFlags >> 5 & 1;

			// Will properties
			WillProperties.Decode(InReader);

			// Will topic
			WillTopic = Data::DecodeString(InReader);

			// Will message
			WillMessage = Data::DecodeString(InReader);

			if (WillTopic.IsEmpty())
			{
				LOG_MQTTIFY(Error, TEXT("[Connect] %s."), WillTopicEmpty);
				ReasonCode = EMqttifyReasonCode::MalformedPacket;
				bIsValid   = false;
				return;
			}

			if (WillMessage.IsEmpty())
			{
				LOG_MQTTIFY(Error, TEXT("[Connect] %s."), WillMessageEmpty);
				ReasonCode = EMqttifyReasonCode::MalformedPacket;
				bIsValid   = false;
				return;
			}
		}
		else
		{
			// Check will QoS is 0
			if (TempFlags >> 3 & 0b00000011)
			{
				LOG_MQTTIFY(Error, TEXT("[Connect] %s."), WillQosMustBe0WithFlag0);
				ReasonCode = EMqttifyReasonCode::MalformedPacket;
				bIsValid   = false;
				return;
			}
			// Check will retain is 0
			if (TempFlags >> 5 & 1)
			{
				LOG_MQTTIFY(Error, TEXT("[Connect] %s."), WillRetainMustBe0WithFlag0);
				ReasonCode = EMqttifyReasonCode::MalformedPacket;
				bIsValid   = false;
				return;
			}
		}

		// Username
		if (TempFlags & 0b10000000)
		{
			Username = Data::DecodeString(InReader);
		}

		if (TempFlags & 0b01000000)
		{
			Password = Data::DecodeString(InReader);
		}
	}
#pragma endregion MQTT 5
} // namespace Mqttify
