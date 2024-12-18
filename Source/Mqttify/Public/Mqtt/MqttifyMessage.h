﻿#pragma once

#include "CoreMinimal.h"
#include "Mqtt/MqttifyProtocolVersion.h"
#include "Mqtt/MqttifyQualityOfService.h"
#include "MqttifyMessage.generated.h"

namespace Mqttify
{
	template <EMqttifyProtocolVersion InProtocolVersion>
	struct TMqttifyPublishPacket;
}

/**
 * @brief Represents an MQTT message.
 *
 * This class encapsulates the data and related operations for an MQTT message.
 * It provides methods to manipulate and access the message content,
 * topic, and other related properties.
 */
USTRUCT(BlueprintType)
struct MQTTIFY_API FMqttifyMessage
{
	GENERATED_BODY()

public:
	FMqttifyMessage()
		: TimeStamp{ FDateTime::UtcNow() }
		, Topic{ TEXT("") }
		, bRetain{ false }
		, QualityOfService(EMqttifyQualityOfService::AtMostOnce) {}

	FMqttifyMessage(const FString& InTopic,
				const TArray<uint8>& InPayload,
				const bool bInRetain,
				const EMqttifyQualityOfService InQualityOfService)
	: TimeStamp{ FDateTime::UtcNow() }
	, Topic{ InTopic }
	, Payload{ InPayload }
	, bRetain{ bInRetain }
	, QualityOfService(InQualityOfService) {}

	FMqttifyMessage(FString&& InTopic,
					TArray<uint8>&& InPayload,
					const bool bInRetain,
					const EMqttifyQualityOfService InQualityOfService)
		: TimeStamp{ FDateTime::UtcNow() }
		, Topic{ MoveTemp(InTopic) }
		, Payload{ MoveTemp(InPayload) }
		, bRetain{ bInRetain }
		, QualityOfService(InQualityOfService) {}

	/**
	 * @brief Get the TimeStamp as UTC.
	 * @return TimeStamp as UTC.
	 */
	const FORCEINLINE FDateTime& GetTimeStamp() const { return TimeStamp; }
	/**
	 * @brief Get the Topic.
	 * @return Topic the message was sent to.
	 */
	const FORCEINLINE FString& GetTopic() const { return Topic; }
	/**
	 * @brief Get the Payload.
	 * @return Payload the message contains.
	 */
	const FORCEINLINE TArray<uint8>& GetPayload() const { return Payload; }
	/**
	 * @brief Get the Retain flag.
	 * @return Retain flag.
	 */
	FORCEINLINE bool GetIsRetained() const { return bRetain; }
	/**
	 * @brief Get the Quality of Service.
	 * @return Quality of Service.
	 */
	FORCEINLINE EMqttifyQualityOfService GetQualityOfService() const { return QualityOfService; }

private:
	/** TimeStamp as UTC. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MQTT", meta = (AllowPrivateAccess = true))
	FDateTime TimeStamp;

	/** Packet topic. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MQTT", meta = (AllowPrivateAccess = true))
	FString Topic;

	/** Packet content. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MQTT", meta = (AllowPrivateAccess = true))
	TArray<uint8> Payload;

	/**
	 * @brief Indicates whether the broker should retain the message for new subscribers.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MQTT", meta = (AllowPrivateAccess = true))
	bool bRetain;

	/** Quality of Service. */
	UPROPERTY(EditAnywhere,
		BlueprintReadOnly,
		Category = "MQTT",
		meta = (DisplayName = "Quality of Service", AllowPrivateAccess = true))
	EMqttifyQualityOfService QualityOfService;

	friend struct Mqttify::TMqttifyPublishPacket<EMqttifyProtocolVersion::Mqtt_3_1_1>;
	friend struct Mqttify::TMqttifyPublishPacket<EMqttifyProtocolVersion::Mqtt_5>;
};
