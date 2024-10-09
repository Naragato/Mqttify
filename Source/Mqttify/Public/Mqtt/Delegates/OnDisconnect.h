#pragma once

namespace Mqttify
{
	DECLARE_TS_MULTICAST_DELEGATE_OneParam(FOnDisconnect, const bool /* IsDisconnected */);
} // namespace Mqttify
