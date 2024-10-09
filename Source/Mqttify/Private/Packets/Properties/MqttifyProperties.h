#pragma once

#include "CoreMinimal.h"
#include "Packets/Properties/MqttifyProperty.h"
#include "Serialization/Interface/ISerializable.h"

namespace Mqttify
{
	/**
	 * @brief The properties of a packet.
	 */
	struct FMqttifyProperties final : ISerializable
	{
	public:
		explicit FMqttifyProperties(const TArray<FMqttifyProperty>& InProperties)
			: Properties{ InProperties } {}

		explicit FMqttifyProperties(FArrayReader& InArchive)
		{
			Decode(InArchive);
		}

		explicit FMqttifyProperties() = default;

		bool operator==(const FMqttifyProperties& InOther) const
		{
			if (Properties.Num() != InOther.Properties.Num())
			{
				return false;
			}

			for (int i = 0; i < Properties.Num(); ++i)
			{
				if (Properties[i] != InOther.Properties[i])
				{
					return false;
				}
			}

			return true;
		}

		bool operator!=(const FMqttifyProperties& InOther) const
		{
			return !(*this == InOther);
		}

		virtual void Decode(FArrayReader& InReader) override;

		virtual void Encode(FMemoryWriter& InWriter) override;

		/**
		 * @brief Get the length of the properties.
		 * @param bWithLengthField Whether to include the field to store the variable length integer in the calculation.
		 * @return The length of the properties.
		 */
		uint32 GetLength(bool bWithLengthField = true) const;

		TArray<FMqttifyProperty> GetProperties() const { return Properties; }

	private:
		TArray<FMqttifyProperty> Properties;
	};
}
