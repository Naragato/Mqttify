#pragma once

namespace Mqttify
{
	DECLARE_TS_MULTICAST_DELEGATE_OneParam(FOnPublish, const bool /* IsPublished */);
} // namespace Mqttify
