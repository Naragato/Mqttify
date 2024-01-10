# Mqttify: An Unreal Engine 5 MQTT Client Plugin

![Mqttify Logo](logo.png)

## Introduction

Welcome to **Mqttify**, an MQTT Client Plugin designed specifically for Unreal Engine 5 (UE5). Built to address the limitations of the official UE5 MQTT plugin, Mqttify aims to provide a robust, feature-complete implementation of the MQTT protocol, supporting both MQTT 3.1.1 and MQTT 5.0.

Mqttify supports multiple connection types including `ws://`, `wss://`, `mqtt://`, and `mqtts://`.

## Why Mqttify?

The existing "official" MQTT plugin for UE5 has a number of shortcomings:

- **Incomplete QoS Handling**: The official plugin has a rudimentary Quality of Service (QoS) implementation.
- **Concurrency Issues**: There are several problems when it comes to handling multiple threads or asynchronous operations.
- **Futures**: Issues with handling asynchronous futures exist.
- **No Unit Tests**: The official plugin does not include any form of testing.
- **Partial MQTT 5 Support**: While types for MQTT 5 exist, there's no complete implementation.

In contrast, Mqttify aims to address these issues.

## Features

- Full support for MQTT 3.1.1 and MQTT 5.0.
- Support for `ws://`, `wss://`, `mqtt://`, and `mqtts://` protocols.
- Comprehensive QoS handling for reliability.
- Thread-safe and optimized for concurrent operations.
- Unit tested to ensure reliability and robustness.

## Simplified Overview of the MQTT Protocol

MQTT (Message Queuing Telemetry Transport) is a lightweight messaging protocol that runs over TCP/IP. It's designed for low-bandwidth, high-latency, or unreliable networks. MQTT is particularly useful for "Internet of Things" applications.

### MQTT Packet Types and Acknowledgments

The core of MQTT is the exchange of messages, known as "packets," between a client and a broker. There are different types of packets, each serving a specific purpose. Here is a table that shows the packets a publisher can send, along with the expected acknowledgments based on the Quality of Service (QoS) level:

| MQTT Packet          | MQTT Version | QoS 0 Expected Acknowledgment | QoS 1 Expected Acknowledgment | QoS 2 Expected Acknowledgment |
|----------------------|--------------|-------------------------------|-------------------------------|-------------------------------|
| CONNECT              | 3.1.1 & 5    | CONNACK                       | CONNACK                       | CONNACK                       |
| CONNACK              | 3.1.1 & 5    | N/A                           | N/A                           | N/A                           |
| PUBLISH              | 3.1.1 & 5    | None                          | PUBACK                        | PUBREC                        |
| PUBACK               | 3.1.1 & 5    | N/A                           | None                          | N/A                           |
| PUBREC               | 3.1.1 & 5    | N/A                           | N/A                           | PUBREL                        |
| PUBREL               | 3.1.1 & 5    | N/A                           | N/A                           | PUBCOMP                       |
| PUBCOMP              | 3.1.1 & 5    | N/A                           | N/A                           | None                          |
| SUBSCRIBE            | 3.1.1 & 5    | SUBACK                        | SUBACK                        | SUBACK                        |
| SUBACK               | 3.1.1 & 5    | N/A                           | N/A                           | N/A                           |
| UNSUBSCRIBE          | 3.1.1 & 5    | N/A                           | N/A                           | N/A                           |
| UNSUBACK             | 3.1.1 & 5    | UNSUBACK                      | UNSUBACK                      | UNSUBACK                      |
| PINGREQ              | 3.1.1 & 5    | PINGRESP                      | PINGRESP                      | PINGRESP                      |
| PINGRESP             | 3.1.1 & 5    | N/A                           | N/A                           | N/A                           |
| DISCONNECT           | 3.1.1 & 5    | None                          | None                          | None                          |
| AUTH                 | 5            | None                          | None                          | None                          |

### Quality of Service (QoS) Levels

MQTT supports 3 QoS levels:

1. **QoS 0 (At most once)**: The message is delivered at most once, and delivery is not confirmed.
2. **QoS 1 (At least once)**: The message is delivered at least once, and delivery is confirmed.
3. **QoS 2 (Exactly once)**: The message is delivered exactly once by using a 4-step handshake.

#### QoS 2 Publish Handshake

The 4-step handshake in QoS 2 ensures that messages are delivered exactly once. Below is a breakdown of this handshake:

| Step | Sender       | Packet  | Receiver     | Description                                                              |
|------|--------------|---------|--------------|--------------------------------------------------------------------------|
| 1    | Publisher    | PUBLISH | Broker       | Publisher sends the message with QoS 2 to the broker.                    |
| 2    | Broker       | PUBREC  | Publisher    | Broker acknowledges the receipt of the PUBLISH message with PUBREC.      |
| 3    | Publisher    | PUBREL  | Broker       | Publisher sends PUBREL as a response to PUBREC.                          |
| 4    | Broker       | PUBCOMP | Publisher    | Broker confirms the PUBREL with PUBCOMP, completing the handshake.       |
| 5    | Broker       | PUBLISH | Subscriber(s)| Broker forwards the message with QoS 2 to the subscriber(s).             |
| 6    | Subscriber(s)| PUBREC  | Broker       | Subscriber(s) acknowledge the receipt of the PUBLISH message with PUBREC.|
| 7    | Broker       | PUBREL  | Subscriber(s)| Broker sends PUBREL as a response to PUBREC.                             |
| 8    | Subscriber(s)| PUBCOMP | Broker       | Subscriber(s) confirm the PUBREL with PUBCOMP, completing the handshake. |

That's it! This covers the core essentials of MQTT. For deeper details, you should consult the official MQTT documentation.

* [MQTT 5.0](https://docs.oasis-open.org/mqtt/mqtt/v5.0/mqtt-v5.0.html)
* [MQTT 3.1.1](http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html)

## Getting Started

### Installation

1. Download the Mqttify plugin.
2. Place the plugin in your UE5 project's `Plugins` directory.
3. Enable `Mqttify` from the UE5 editor.

### Quick Start

Here's a quick example to get you started:
@TODO
```cpp
// Initialize Mqttify client
// Include the Mqttify Client interface
#include "IMqttifyClient.h"

// Initialize Mqttify client settings
FMqttifyConnectionSettings ConnectionSettings;
ConnectionSettings.URL = TEXT("mqtt://broker.example.com");

// Create an instance of the Mqttify client
TSharedRef<IMqttifyClient> Client = IMqttifyClient::Create();

// Register delegate for connection events
Client->OnConnect().AddLambda([](EMqttifyConnectReturnCode ConnectReturnCode) {
    if (ConnectReturnCode == EMqttifyConnectReturnCode::Success) {
        UE_LOG(LogTemp, Log, TEXT("Successfully connected to broker."));
    } else {
        UE_LOG(LogTemp, Log, TEXT("Failed to connect to broker."));
    }
});

// Connect to the broker
TFuture<EMqttifyConnectReturnCode> ConnectionFuture = Client->Connect(ConnectionSettings);

// Publish a message
TArray<uint8> Payload = { /* Your payload data here */ };
Client->Publish("my/topic", Payload, EMqttifyQualityOfService::Once);

// Subscribe to a topic
TFuture<FMqttSubscribeResult> SubscribeFuture = Client->Subscribe("my/topic", NEMqttifyQualityOfService::Once);

// Register delegate for receiving messages
Client->OnMessage().AddLambda([](const FMqttifyMessage& Message) {
    // Your logic for handling received messages here
    UE_LOG(LogTemp, Log, TEXT("Received message: %s"), *Message.Payload);
});
```


## Contributing

TODO

## License

TODO

---

Developed by (https://github.com/naragato). For any issues, please report them [here](https://github.com/naragato/Mqttify).


```
#pragma once

#include "CoreMinimal.h"
#include "Mqtt/MqttifyQualityOfService.h"

struct FMqttifyConnectionSettings;
enum class EMqttifyConnectReturnCode : uint8;
struct FMqttSubscribeResult;
struct FMqttifyMessage;

class MQTTIFY_API IMqttifyClient
{
public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnConnect, EMqttifyConnectReturnCode)
	virtual FOnConnect& OnConnect() = 0;

	DECLARE_MULTICAST_DELEGATE(FOnDisconnect)
	virtual FOnDisconnect& OnDisconnect() = 0;

	DECLARE_MULTICAST_DELEGATE(FOnPublish)
	virtual FOnPublish& OnPublish() = 0;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnSubscribe, TArray<FMqttSubscribeResult> /* SubscriptionHandles */)
	virtual FOnSubscribe& OnSubscribe() = 0;

	DECLARE_MULTICAST_DELEGATE(FOnUnsubscribe)
	virtual FOnUnsubscribe& OnUnsubscribe() = 0;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnMessage, const FMqttifyMessage& /* Packet */)
	virtual FOnMessage& OnMessage() = 0;

public:
	virtual ~IMqttifyClient() = default;

	/**
	 * @brief Connect to the MQTT broker
	 * @param InConnectionSettings The URL of the MQTT server
	 * @param bCleanSession False Silently updates auth credentials and triggers reconnection if a connection is active
	 * @return The return code of the connection attempt
	 */
	virtual TFuture<EMqttifyConnectReturnCode> Connect(FMqttifyConnectionSettings& InConnectionSettings, bool bCleanSession = true) = 0;

	/**
	 * @brief Disconnect from to the MQTT broker
	 * @return Whether the disconnection was successful, true if already disconnected
	 */
	virtual TFuture<bool> Disconnect() = 0;

	/**
	 * @brief Publish to a topic on the connected MQTT broker
	 * @param InTopic The topic to publish to
	 * @param InPayload The payload / message to publish to the topic
	 * @param InQoS The quality of service to use for the publish
	 * @param bInRetain	Whether the message should be retained by the server
	 * @return Whether the publish was successful, depending on the QoS
	 */
	virtual TFuture<bool> Publish(const FString& InTopic,
	                              const TArray<uint8>& InPayload,
	                              EMqttifyQualityOfService InQoS = EMqttifyQualityOfService::Once,
	                              const bool bInRetain        = false) = 0;

	/**
	 * @brief Subscribe to a topic on the connected MQTT broker
	 * @param InTopicFilterQoSPairs The topic to subscribe to and the QoS to use for each topic
	 * @return an array of subscribe results, one for each topic
	 */
	virtual TFuture<TArray<FMqttSubscribeResult>> Subscribe(
		const TArray<TPair<FString, EMqttifyQualityOfService>>& InTopicFilterQoSPairs) = 0;

	/**
	 * @param InTopicFilter The topic to subscribe to
	 * @param InQoS The quality of service to use for the subscription
	 * @return The subscribe result for the topic
	 */
	TFuture<FMqttSubscribeResult> Subscribe(const FString& InTopicFilter,
	                                        EMqttifyQualityOfService InQoS = EMqttifyQualityOfService::Once);

	/**
	 * @return Whether the Unsubscribe was successful, true if already unsubscribed
	 */
	virtual TFuture<bool> Unsubscribe(const TSet<FString>& InTopicFilters) = 0;

	/**
	 * @return Returns true if response received before timeout.
	 */
	virtual TFuture<bool> Ping(const float& InTimeout = 2.0f) = 0;

	/**
	 * @return Unique Id for this client.
	 */
	virtual FGuid GetClientId() const = 0;

	/**
	 * @return URL for this client.
	 */
	virtual const FMqttifyConnectionSettings& GetConnectionSettings() const = 0;

	/**
	 * @return Is client currently connected?
	 */
	virtual bool IsConnected() const = 0;

	/**
	 * @return Validity of this client.
	 */
	virtual bool IsValid() const = 0;
};
```