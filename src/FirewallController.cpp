/*
 * FirewallController.cpp - platform independent internet blocking logic
 *
 * SPDX-License-Identifier: MIT
 */

#include <QHostInfo>
#include <QRegularExpression>

#include "FirewallController.h"
#include "Logger.h"
#include "backends/NoopFirewallBackend.h"

// To add a backend for another platform, implement FirewallBackend in src/backends/,
// add its sources to CMakeLists.txt and create it here
#if defined(Q_OS_WIN)
#include "backends/WindowsFirewallBackend.h"
#endif

static const auto DefaultBlockedPorts = QStringLiteral( "80,443,8080,8443" );


std::unique_ptr<FirewallBackend> FirewallController::createBackend()
{
#if defined(Q_OS_WIN)
	return std::make_unique<WindowsFirewallBackend>();
#else
	return std::make_unique<NoopFirewallBackend>();
#endif
}



FirewallController::FirewallController() :
	m_backend( createBackend() )
{
}



FirewallController::~FirewallController() = default;



FirewallController& FirewallController::instance()
{
	static FirewallController s_instance;
	return s_instance;
}



bool FirewallController::isSupported() const
{
	return m_backend->isSupported();
}



bool FirewallController::blockInternet( const Settings& settings )
{
	if( isSupported() == false )
	{
		vWarning() << "Blocking internet access is not supported on this platform yet";
		return false;
	}

	vDebug() << "Blocking internet access via" << m_backend->name() << "with mode" << static_cast<int>( settings.mode );

	const auto success = m_backend->block( createPolicy( settings ) );
	if( success == false )
	{
		vCritical() << "Failed to block internet access via" << m_backend->name();
	}

	m_isBlocked = success;
	refreshState();

	return success;
}



bool FirewallController::unblockInternet()
{
	vDebug() << "Unblocking internet access via" << m_backend->name();

	const auto success = m_backend->unblock();
	if( success == false )
	{
		vCritical() << "Failed to unblock internet access via" << m_backend->name();
	}

	m_isBlocked = false;
	refreshState();

	return success;
}



void FirewallController::refreshState()
{
	if( const auto blocked = m_backend->queryBlocked(); blocked.has_value() )
	{
		m_isBlocked = *blocked;
	}
}



FirewallPolicy FirewallController::createPolicy( const Settings& settings )
{
	static const QRegularExpression portsRX{ QStringLiteral( "^\\d{1,5}(-\\d{1,5})?(,\\d{1,5}(-\\d{1,5})?)*$" ) };

	FirewallPolicy policy;
	policy.mode = settings.mode;
	policy.blockDns = settings.blockDns;

	policy.blockedPorts = settings.blockedPorts.simplified().remove( QLatin1Char( ' ' ) );
	if( portsRX.match( policy.blockedPorts ).hasMatch() == false )
	{
		if( policy.blockedPorts.isEmpty() == false )
		{
			vWarning() << "Ignoring invalid blocked ports" << settings.blockedPorts;
		}
		policy.blockedPorts = DefaultBlockedPorts;
	}

	QStringList invalidEntries;
	policy.allowedNetworks = NetworkRanges::parseSubnets( settings.allowedSubnets, &invalidEntries );
	if( invalidEntries.isEmpty() == false )
	{
		vWarning() << "Ignoring invalid local network entries" << invalidEntries;
	}

	QStringList hostNames;
	policy.allowedNetworks += NetworkRanges::parseSubnets( settings.allowedHosts, &hostNames );
	for( const auto& hostName : std::as_const( hostNames ) )
	{
		const auto hostInfo = QHostInfo::fromName( hostName );
		if( hostInfo.error() != QHostInfo::NoError )
		{
			vWarning() << "Could not resolve allowed host" << hostName << hostInfo.errorString();
			continue;
		}
		for( const auto& address : hostInfo.addresses() )
		{
			policy.allowedNetworks += NetworkRanges::parseSubnets( address.toString() );
		}
	}

	return policy;
}
