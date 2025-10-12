#pragma once

#include "Containers/Union.h"
#include "Packets/Properties//MqttifyPropertyIdentifier.h"
#include "Serialization/ArrayReader.h"
#include "Serialization/Interface/ISerializable.h"
#include "Templates/Tuple.h"

namespace Mqttify
{
	/**
	 * @brief Wrapper for MQTT properties.
	 */
	class FMqttifyProperty final : public ISerializable
	{
	private:
		template <typename TData>
		explicit FMqttifyProperty(const EMqttifyPropertyIdentifier InIdentifier, TData InValue)
			: Identifier{InIdentifier}
		{
			Data.SetSubtype<TData>(InValue);
		}

	public:
		static constexpr TCHAR InvalidPropertyIdentifier[] = TEXT("Invalid property identifier");
		static constexpr TCHAR FailedToReadPropertyError[] = TEXT("Failed to read property.");

		FMqttifyProperty() = delete;

		template <EMqttifyPropertyIdentifier TIdentifier, typename TData>
		static FORCEINLINE FMqttifyProperty Create(TData InValue);

		// add more specializations...
		explicit FMqttifyProperty(FArrayReader& InReader) { Decode(InReader); }

		bool operator==(const FMqttifyProperty& InOther) const
		{
			return Identifier == InOther.Identifier && Data == InOther.Data;
		}

		bool operator!=(const FMqttifyProperty& Element) const
		{
			return !(*this == Element);
		}

		virtual void Encode(FMemoryWriter& InWriter) override;
		virtual void Decode(FArrayReader& InReader) override;
		EMqttifyPropertyIdentifier GetIdentifier() const { return Identifier; }

		uint32 GetLength() const;
		/**
		 * @brief Get the property data length.
		 * @param OutValue The value of the property.
		 * @return True when has value, false otherwise.
		 */
		bool TryGetValue(uint8& OutValue) const;
		/**
		 * @brief Get the property data length.
		 * @param OutValue The value of the property.
		 * @return True when has value, false otherwise.
		 */
		bool TryGetValue(uint16& OutValue) const;
		/**
		 * @brief Get the property data length.
		 * @param OutValue The value of the property.
		 * @return True when has value, false otherwise.
		 */
		bool TryGetValue(uint32& OutValue) const;
		/**
		 * @brief Get the property data length.
		 * @param OutValue The value of the property.
		 * @return True when has value, false otherwise.
		 */
		bool TryGetValue(TArray<uint8>& OutValue) const;
		/**
		 * @brief Get the property data length.
		 * @param OutValue The value of the property.
		 * @return True when has value, false otherwise.
		 */
		bool TryGetValue(FString& OutValue) const;
		/**
		 * @brief Get the property data length.
		 * @param OutValue The value of the property.
		 * @return True when has value, false otherwise.
		 */
		bool TryGetValue(TTuple<FString, FString>& OutValue) const;

		FString ToString() const
		{

			if (Data.HasSubtype<uint8>())
			{
				return FString::Printf(TEXT("%hhu"), Data.GetSubtype<uint8>());
			}

			if (Data.HasSubtype<uint16>())
			{
				return FString::Printf(TEXT("%hu"), Data.GetSubtype<uint16>());
			}

			if (Data.HasSubtype<uint32>())
			{
				return FString::Printf(TEXT("%u"), Data.GetSubtype<uint32>());
			}

			if (Data.HasSubtype<TArray<uint8>>())
			{
				FString Result;
				for (const auto& Byte : Data.GetSubtype<TArray<uint8>>())
				{
					Result += FString::Printf(TEXT("%hhu"), Byte);
				}
				return Result;
			}

			if (Data.HasSubtype<FString>())
			{
				return Data.GetSubtype<FString>();
			}

			if (Data.HasSubtype<TTuple<FString, FString>>())
			{
				const auto& Tuple = Data.GetSubtype<TTuple<FString, FString>>();
				return FString::Printf(TEXT("%s, %s"), *Tuple.Key, *Tuple.Value);
			}

			return TEXT("Unknown");
		}

	private:
		TUnion<uint8, uint16, uint32, TArray<uint8>, FString, TTuple<FString, FString>> Data;
		EMqttifyPropertyIdentifier Identifier;
		static FORCEINLINE EMqttifyPropertyIdentifier DecodePropertyIdentifier(FArrayReader& InReader);
	};

	template <EMqttifyPropertyIdentifier TIdentifier, typename TData>
	FORCEINLINE FMqttifyProperty FMqttifyProperty::Create(TData /*InValue*/)

	{
		static_assert("Invalid type for given enum value");
		return FMqttifyProperty{EMqttifyPropertyIdentifier::Unknown, 0};
	}

	// uint32
	template <>
	FORCEINLINE FMqttifyProperty FMqttifyProperty::Create<EMqttifyPropertyIdentifier::MessageExpiryInterval, uint32>(
		const uint32 InValue)
	{
		return FMqttifyProperty(EMqttifyPropertyIdentifier::MessageExpiryInterval, InValue);
	}

	template <>
	FORCEINLINE FMqttifyProperty FMqttifyProperty::Create<EMqttifyPropertyIdentifier::SubscriptionIdentifier, uint32>(
		const uint32 InValue)
	{
		return FMqttifyProperty(EMqttifyPropertyIdentifier::SubscriptionIdentifier, InValue);
	}

	template <>
	FORCEINLINE FMqttifyProperty FMqttifyProperty::Create<EMqttifyPropertyIdentifier::SessionExpiryInterval, uint32>(
		const uint32 InValue)
	{
		return FMqttifyProperty(EMqttifyPropertyIdentifier::SessionExpiryInterval, InValue);
	}

	template <>
	FORCEINLINE FMqttifyProperty FMqttifyProperty::Create<EMqttifyPropertyIdentifier::WillDelayInterval, uint32>(
		const uint32 InValue)
	{
		return FMqttifyProperty(EMqttifyPropertyIdentifier::WillDelayInterval, InValue);
	}

	template <>
	FORCEINLINE FMqttifyProperty FMqttifyProperty::Create<EMqttifyPropertyIdentifier::MaximumPacketSize, uint32>(
		const uint32 InValue)
	{
		return FMqttifyProperty(EMqttifyPropertyIdentifier::MaximumPacketSize, InValue);
	}

	// FString
	template <>
	FORCEINLINE FMqttifyProperty FMqttifyProperty::Create<EMqttifyPropertyIdentifier::ContentType, FString>(
		const FString InValue)
	{
		return FMqttifyProperty(EMqttifyPropertyIdentifier::ContentType, InValue);
	}

	template <>
	FORCEINLINE FMqttifyProperty FMqttifyProperty::Create<EMqttifyPropertyIdentifier::ResponseTopic, FString>(
		const FString InValue)
	{
		return FMqttifyProperty(EMqttifyPropertyIdentifier::ResponseTopic, InValue);
	}

	template <>
	FORCEINLINE FMqttifyProperty FMqttifyProperty::Create<
		EMqttifyPropertyIdentifier::AssignedClientIdentifier, FString>(
		const FString InValue)
	{
		return FMqttifyProperty(EMqttifyPropertyIdentifier::AssignedClientIdentifier, InValue);
	}

	template <>
	FORCEINLINE FMqttifyProperty FMqttifyProperty::Create<EMqttifyPropertyIdentifier::AuthenticationMethod, FString>(
		const FString InValue)
	{
		return FMqttifyProperty(EMqttifyPropertyIdentifier::AuthenticationMethod, InValue);
	}

	template <>
	FORCEINLINE FMqttifyProperty FMqttifyProperty::Create<EMqttifyPropertyIdentifier::ResponseInformation, FString>(
		const FString InValue)
	{
		return FMqttifyProperty(EMqttifyPropertyIdentifier::ResponseInformation, InValue);
	}

	template <>
	FORCEINLINE FMqttifyProperty FMqttifyProperty::Create<EMqttifyPropertyIdentifier::ServerReference, FString>(
		const FString InValue)
	{
		return FMqttifyProperty(EMqttifyPropertyIdentifier::ServerReference, InValue);
	}

	template <>
	FORCEINLINE FMqttifyProperty FMqttifyProperty::Create<EMqttifyPropertyIdentifier::ReasonString, FString>(
		const FString InValue)
	{
		return FMqttifyProperty(EMqttifyPropertyIdentifier::ReasonString, InValue);
	}

	// uint8
	template <>
	FORCEINLINE FMqttifyProperty FMqttifyProperty::Create<EMqttifyPropertyIdentifier::PayloadFormatIndicator, uint8>(
		const uint8 InValue)
	{
		return FMqttifyProperty(EMqttifyPropertyIdentifier::PayloadFormatIndicator, InValue);
	}

	template <>
	FORCEINLINE FMqttifyProperty FMqttifyProperty::Create<EMqttifyPropertyIdentifier::RequestProblemInformation, uint8>(
		const uint8 InValue)
	{
		return FMqttifyProperty(EMqttifyPropertyIdentifier::RequestProblemInformation, InValue);
	}

	template <>
	FORCEINLINE FMqttifyProperty FMqttifyProperty::Create<EMqttifyPropertyIdentifier::RetainAvailable, uint8>(
		const uint8 InValue)
	{
		return FMqttifyProperty(EMqttifyPropertyIdentifier::RetainAvailable, InValue);
	}

	template <>
	FORCEINLINE FMqttifyProperty FMqttifyProperty::Create<
		EMqttifyPropertyIdentifier::WildcardSubscriptionAvailable, FPlatformTypes::uint8>(
		const uint8 InValue)
	{
		return FMqttifyProperty(EMqttifyPropertyIdentifier::WildcardSubscriptionAvailable, InValue);
	}

	template <>
	FORCEINLINE FMqttifyProperty FMqttifyProperty::Create<
		EMqttifyPropertyIdentifier::SubscriptionIdentifierAvailable, uint8>(
		const uint8 InValue)
	{
		return FMqttifyProperty(EMqttifyPropertyIdentifier::SubscriptionIdentifierAvailable, InValue);
	}

	template <>
	FORCEINLINE FMqttifyProperty FMqttifyProperty::Create<
		EMqttifyPropertyIdentifier::SharedSubscriptionAvailable, uint8>(
		const uint8 InValue)
	{
		return FMqttifyProperty(EMqttifyPropertyIdentifier::SharedSubscriptionAvailable, InValue);
	}

	template <>
	FORCEINLINE FMqttifyProperty FMqttifyProperty::Create<EMqttifyPropertyIdentifier::MaximumQoS, uint8>(
		const uint8 InValue)
	{
		return FMqttifyProperty(EMqttifyPropertyIdentifier::MaximumQoS, InValue);
	}

	template <>
	FORCEINLINE FMqttifyProperty FMqttifyProperty::Create<
		EMqttifyPropertyIdentifier::RequestResponseInformation, uint8>(
		const uint8 InValue)
	{
		return FMqttifyProperty(EMqttifyPropertyIdentifier::RequestResponseInformation, InValue);
	}

	// uint16
	template <>
	FORCEINLINE FMqttifyProperty FMqttifyProperty::Create<EMqttifyPropertyIdentifier::ServerKeepAlive, uint16>(
		const uint16 InValue)
	{
		return FMqttifyProperty(EMqttifyPropertyIdentifier::ServerKeepAlive, InValue);
	}

	template <>
	FORCEINLINE FMqttifyProperty FMqttifyProperty::Create<EMqttifyPropertyIdentifier::ReceiveMaximum, uint16>(
		const uint16 InValue)
	{
		return FMqttifyProperty(EMqttifyPropertyIdentifier::ReceiveMaximum, InValue);
	}

	template <>
	FORCEINLINE FMqttifyProperty FMqttifyProperty::Create<EMqttifyPropertyIdentifier::TopicAliasMaximum, uint16>(
		const uint16 InValue)
	{
		return FMqttifyProperty(EMqttifyPropertyIdentifier::TopicAliasMaximum, InValue);
	}

	template <>
	FORCEINLINE FMqttifyProperty FMqttifyProperty::Create<EMqttifyPropertyIdentifier::TopicAlias, uint16>(
		const uint16 InValue)
	{
		return FMqttifyProperty(EMqttifyPropertyIdentifier::TopicAlias, InValue);
	}

	// TArray<uint8>
	template <>
	FORCEINLINE FMqttifyProperty FMqttifyProperty::Create<EMqttifyPropertyIdentifier::CorrelationData, TArray<uint8>>(
		const TArray<uint8> InValue)
	{
		return FMqttifyProperty(EMqttifyPropertyIdentifier::CorrelationData, InValue);
	}

	template <>
	FORCEINLINE FMqttifyProperty FMqttifyProperty::Create<
		EMqttifyPropertyIdentifier::AuthenticationData, TArray<uint8>>(
		const TArray<uint8> InValue)
	{
		return FMqttifyProperty(EMqttifyPropertyIdentifier::AuthenticationData, InValue);
	}

	// TTuple<FString, FString>
	template <>
	FORCEINLINE FMqttifyProperty FMqttifyProperty::Create<
		EMqttifyPropertyIdentifier::UserProperty, TTuple<FString, FString>>(
		const TTuple<FString, FString> InValue)
	{
		return FMqttifyProperty(EMqttifyPropertyIdentifier::UserProperty, InValue);
	}

} // namespace Mqttify
