#pragma once

namespace Mqttify
{
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnDisconnect, const bool /* IsDisconnected */);
} // namespace Mqttify
