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
- Support for `ws://`, `wss://`, `mqtt://`, and `mqtts://`(coming soon) protocols.
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

## Quick Start

Here's a quick example to get you started:

```cpp
#include "MqttifyModule.h"
#include "Mqtt/MqttifyClient.h"

//...

// Example ...
void MyFunction() 
{
    IMqttifyModule& MqttifyModule = IMqttifyModule::Get();

    TSharedPtr<IMqttifyClient> MqttClient = MqttifyModule.GetOrCreateClient(
        FString(TEXT("mqtt://clientA:password@localhost:1883")));


    // Set up connection delegate
    MqttClient->OnConnect().AddRaw(this, &MyClass::OnClientConnect);

    // Connect clients
    MqttClient->ConnectAsync();

    // Subscribe to a topic with Client B
    MqttClient->SubscribeAsync(FString(TEXT("/Test")));

    // Define handling for received messages
    FOnMessage::FDelegate ClientOnMessageDelegate = FOnMessage::FDelegate::CreateLambda(
        [](const FMqttifyMessage& InMessage)
        {
            // Handle incoming message
            UE_LOG(LogTemp, Log, TEXT("Received message on topic %s"), *InMessage.GetTopic());
        });

    MqttClient->OnMessage().Add(ClientOnMessageDelegate);

    // Publish a message with Client A
    TArray<uint8> Payload = { /* Your payload data here */ };
    MqttClient->PublishAsync(FMqttifyMessage{
        FString(TEXT("/Test")), Payload, false, EMqttifyQualityOfService::AtMostOnce
    });
}

// Callback for client connections
void MyClass::OnClientConnect(const bool bConnected) const
{
    // Handle connection event
    if (bConnected)
    {
        UE_LOG(LogTemp, Log, TEXT("Client connected."));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Client failed to connect."));
    }
}
```

# Contribution Guidelines for Mqttify

We warmly welcome contributions to the Mqttify project! Whether you're fixing bugs, adding new features, or improving documentation, your help is invaluable. Below are the guidelines to follow when contributing to this project.

## Code of Conduct

Our project is dedicated to providing a welcoming and respectful environment for everyone. We expect contributors to adhere to our code of conduct, which promotes a harassment-free experience for all members of our community.

## Getting Started

1. **Fork the Repository**: Start by forking the repository on GitHub. This creates your own copy of the project to work on.

2. **Clone Your Fork**: Clone your fork to your local machine to start making changes.

3. **Set Up Your Environment**: Ensure you have Unreal Engine and the necessary dependencies installed to work with the project.

## Making Contributions

1. **Create a New Branch**: For each new feature or fix, create a new branch in your fork. Use a descriptive name that reflects the purpose of your changes.

2. **Write Quality Code**: Follow the existing coding style and best practices. Ensure your code is clean, readable, and well-commented.

3. **Adhere to Commit Message Conventions**: Write clear, concise commit messages that describe the changes you've made.

4. **Test Your Changes**: Before submitting your changes, thoroughly test them in the Unreal Engine environment. Ensure that your changes do not introduce any new bugs.

5. **Update Documentation**: If your changes involve user-facing features or require modifications to the documentation, be sure to update the README or other relevant documentation files.

6. **Submit a Pull Request**: Once your changes are ready, push your branch to your fork and submit a pull request to the main repository. Clearly describe the purpose of your changes and any relevant information.

## Review Process

1. **Code Reviews**: Our maintainers will review your pull request. Be open to feedback and ready to make adjustments as necessary.

2. **Discussion and Revisions**: There may be a discussion regarding your contribution. Be open to constructive criticism and be prepared to make further revisions.

3. **Acceptance**: Once your pull request meets all criteria and receives approval from the maintainers, it will be merged into the project.

## Additional Guidelines

- **Stay Up-to-Date**: Regularly sync your fork with the main repository to stay up-to-date with the latest changes.

- **Issue Tracking**: If you're working on a specific issue, mention it in your pull request. If you're adding a new feature or fixing an unreported bug, consider opening an issue first to discuss it with the community.

- **Respect Copyrights and Licenses**: Make sure your contributions do not violate any copyrights or licenses. If you're adding third-party code or libraries, they must be compatible with the project's license.

- **Be Patient**: The review and acceptance process can take some time. Please be patient as maintainers and other contributors review your changes.

---

Thank you for considering contributing to Mqttify. Your efforts help make this project better for everyone! 🚀🌟

## License

This project is licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.

---

Developed by (https://github.com/naragato | ETH: 0xf7f210e645859B13edde6b99BECACf89fbEF80bA | BTC:  33FWb8ddqJMmWdrbQmDcLND3aNa8G3hiH7). For any issues, please report them [here](https://github.com/Naragato/Mqttify/issues).
