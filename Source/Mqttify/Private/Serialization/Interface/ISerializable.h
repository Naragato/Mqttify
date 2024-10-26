#pragma once
#include "Serialization/ArrayReader.h"
#include "Serialization/MemoryWriter.h"

class FArchive;

namespace Mqttify
{
	/**
	 * @brief Interface for serializable objects.
	 *
	 * Classes implementing this interface can be serialized
	 * to and deserialized from FArchive objects, which can
	 * be useful for encoding and decoding MQTT packets.
	 */
	class ISerializable
	{
	public:
		virtual ~ISerializable() = default;

		/**
		 * @brief serialize the object to an FMemoryWriter.
		 * @param InWriter FMemoryWriter object to serialize to.
		 */
		virtual void Encode(FMemoryWriter& InWriter) = 0;
		/**
		 * @bried deserialize the object from an FArrayReader.
		 * @param InReader FArrayReader object to deserialize from.
		 */
		virtual void Decode(FArrayReader& InReader) = 0;
	};
} // namespace Mqttify
