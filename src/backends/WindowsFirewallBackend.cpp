/*
 * WindowsFirewallBackend.cpp - Windows Firewall implementation of FirewallBackend
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <initguid.h>
#include <netfw.h>
#include <objbase.h>

#ifndef NET_FW_IP_PROTOCOL_ANY
#define NET_FW_IP_PROTOCOL_ANY static_cast<NET_FW_IP_PROTOCOL>(256)
#endif

#include <QProcess>

#include "Logger.h"
#include "WindowsFirewallBackend.h"

static const auto RuleNamePrefix = QStringLiteral( "Veyon_IAC_" );
static const auto RuleGroup = QStringLiteral( "Veyon Internet Access Control" );


bool WindowsFirewallBackend::block( const FirewallPolicy& policy )
{
	// remove any existing block first to avoid duplicates
	unblock();

	const auto rules = buildRules( policy );
	if( rules.isEmpty() )
	{
		vWarning() << "No public address ranges left to block - check the configured local networks";
		return false;
	}

	if( addRulesViaCom( rules ) )
	{
		return true;
	}

	vWarning() << "Failed to block via Windows Firewall COM API, falling back to netsh";
	removeRulesViaCom();
	return addRulesViaNetsh( rules );
}



bool WindowsFirewallBackend::unblock()
{
	if( removeRulesViaCom() )
	{
		return true;
	}

	vWarning() << "Failed to unblock via Windows Firewall COM API, falling back to netsh";
	return removeRulesViaNetsh();
}



std::optional<bool> WindowsFirewallBackend::queryBlocked() const
{
	bool hasRules = false;
	if( hasRulesViaCom( &hasRules ) )
	{
		return hasRules;
	}
	return std::nullopt;
}



QList<WindowsFirewallBackend::Rule> WindowsFirewallBackend::buildRules( const FirewallPolicy& policy )
{
	// Windows Firewall evaluates block rules before allow rules, so exceptions can't be added as
	// allow rules - instead the block rules only cover the address ranges outside allowedNetworks
	const auto v4 = NetworkRanges::publicRangesExcluding( policy.allowedNetworks, QAbstractSocket::IPv4Protocol ).join( QLatin1Char( ',' ) );
	const auto v6 = NetworkRanges::publicRangesExcluding( policy.allowedNetworks, QAbstractSocket::IPv6Protocol ).join( QLatin1Char( ',' ) );

	QList<Rule> rules;
	const auto addRules = [&]( const QString& name, const QString& description, Protocol protocol, const QString& ports ) {
		// an empty address list would mean "any address" - never create such a rule
		if( v4.isEmpty() == false )
		{
			rules.append( { RuleNamePrefix + name, description + QStringLiteral( " (IPv4)" ), protocol, ports, v4 } );
		}
		if( v6.isEmpty() == false )
		{
			rules.append( { RuleNamePrefix + name + QStringLiteral( "_V6" ), description + QStringLiteral( " (IPv6)" ),
							protocol, ports, v6 } );
		}
	};

	if( policy.mode == FirewallPolicy::Mode::WebPortsOnly )
	{
		addRules( QStringLiteral( "Block_Web_TCP" ), QStringLiteral( "Veyon Internet Access Control - block outbound web traffic (TCP)" ),
				  Protocol::Tcp, policy.blockedPorts );
		// HTTP/3 (QUIC)
		addRules( QStringLiteral( "Block_Web_UDP" ), QStringLiteral( "Veyon Internet Access Control - block outbound web traffic (UDP)" ),
				  Protocol::Udp, policy.blockedPorts );
	}
	else
	{
		addRules( QStringLiteral( "Block_All_NonLAN" ), QStringLiteral( "Veyon Internet Access Control - block all outbound non-LAN traffic" ),
				  Protocol::Any, {} );
	}

	if( policy.blockDns )
	{
		addRules( QStringLiteral( "Block_DNS" ), QStringLiteral( "Veyon Internet Access Control - block outbound DNS to external servers" ),
				  Protocol::Udp, QStringLiteral( "53" ) );
	}

	return rules;
}



QStringList WindowsFirewallBackend::ruleNames()
{
	QStringList names;
	for( const auto* name : { "Block_Web_TCP", "Block_Web_UDP", "Block_All_NonLAN", "Block_DNS" } )
	{
		names.append( RuleNamePrefix + QLatin1String( name ) );
		names.append( RuleNamePrefix + QLatin1String( name ) + QStringLiteral( "_V6" ) );
	}
	return names;
}



namespace {

class ComScope
{
public:
	ComScope() :
		m_result( CoInitializeEx( nullptr, COINIT_APARTMENTTHREADED ) )
	{
	}

	~ComScope()
	{
		if( SUCCEEDED( m_result ) )
		{
			CoUninitialize();
		}
	}

	bool isUsable() const
	{
		// COM already initialized with a different threading model is fine for INetFwPolicy2
		return SUCCEEDED( m_result ) || m_result == RPC_E_CHANGED_MODE;
	}

private:
	const HRESULT m_result;
};

class Bstr
{
public:
	explicit Bstr( const QString& string ) :
		m_bstr( SysAllocString( reinterpret_cast<const OLECHAR*>( string.utf16() ) ) )
	{
	}

	~Bstr()
	{
		SysFreeString( m_bstr );
	}

	operator BSTR() const
	{
		return m_bstr;
	}

private:
	BSTR m_bstr;
};

template<typename Function>
bool withFirewallRules( Function function )
{
	ComScope com;
	if( com.isUsable() == false )
	{
		vCritical() << "Failed to initialize COM";
		return false;
	}

	INetFwPolicy2* policy = nullptr;
	HRESULT hr = CoCreateInstance( CLSID_NetFwPolicy2, nullptr, CLSCTX_INPROC_SERVER,
								   IID_INetFwPolicy2, reinterpret_cast<void**>( &policy ) );
	if( FAILED( hr ) || policy == nullptr )
	{
		vCritical() << "Failed to create INetFwPolicy2 instance:" << hr;
		return false;
	}

	bool result = false;
	INetFwRules* rules = nullptr;
	hr = policy->get_Rules( &rules );
	if( SUCCEEDED( hr ) && rules )
	{
		result = function( rules );
		rules->Release();
	}
	else
	{
		vCritical() << "Failed to get firewall rules:" << hr;
	}

	policy->Release();

	return result;
}

} // namespace



bool WindowsFirewallBackend::addRulesViaCom( const QList<Rule>& rules )
{
	return withFirewallRules( [&rules]( INetFwRules* firewallRules ) {
		for( const auto& rule : rules )
		{
			const auto protocol = rule.protocol == Protocol::Tcp ? NET_FW_IP_PROTOCOL_TCP
								  : rule.protocol == Protocol::Udp ? NET_FW_IP_PROTOCOL_UDP
																   : NET_FW_IP_PROTOCOL_ANY;

			INetFwRule* firewallRule = nullptr;
			HRESULT hr = CoCreateInstance( CLSID_NetFwRule, nullptr, CLSCTX_INPROC_SERVER,
										   IID_INetFwRule, reinterpret_cast<void**>( &firewallRule ) );
			if( FAILED( hr ) || firewallRule == nullptr )
			{
				vCritical() << "Failed to create INetFwRule instance:" << hr;
				return false;
			}

			// the protocol has to be set before the ports; every step has to succeed as a rule
			// with missing remote addresses would block all traffic including Veyon itself
			const auto check = [&hr]( HRESULT result ) { hr = result; return SUCCEEDED( result ); };
			const bool configured =
				check( firewallRule->put_Name( Bstr( rule.name ) ) ) &&
				check( firewallRule->put_Description( Bstr( rule.description ) ) ) &&
				check( firewallRule->put_Grouping( Bstr( RuleGroup ) ) ) &&
				check( firewallRule->put_Direction( NET_FW_RULE_DIR_OUT ) ) &&
				check( firewallRule->put_Action( NET_FW_ACTION_BLOCK ) ) &&
				check( firewallRule->put_Protocol( protocol ) ) &&
				( rule.remotePorts.isEmpty() || check( firewallRule->put_RemotePorts( Bstr( rule.remotePorts ) ) ) ) &&
				check( firewallRule->put_RemoteAddresses( Bstr( rule.remoteAddresses ) ) ) &&
				check( firewallRule->put_Profiles( NET_FW_PROFILE2_ALL ) ) &&
				check( firewallRule->put_Enabled( VARIANT_TRUE ) ) &&
				check( firewallRules->Add( firewallRule ) );

			firewallRule->Release();

			if( configured == false )
			{
				vCritical() << "Failed to add firewall rule" << rule.name << ":" << hr;
				return false;
			}
		}

		return true;
	} );
}



bool WindowsFirewallBackend::removeRulesViaCom()
{
	return withFirewallRules( []( INetFwRules* firewallRules ) {
		for( const auto& name : ruleNames() )
		{
			const Bstr bstrName( name );
			INetFwRule* rule = nullptr;
			// Remove() only deletes one of several rules with the same name and succeeds
			// even if no rule exists, so loop while the rule can still be found
			for( int i = 0; i < 10 && SUCCEEDED( firewallRules->Item( bstrName, &rule ) ) && rule; ++i )
			{
				rule->Release();
				rule = nullptr;
				firewallRules->Remove( bstrName );
			}
		}
		return true;
	} );
}



bool WindowsFirewallBackend::hasRulesViaCom( bool* hasRules )
{
	return withFirewallRules( [hasRules]( INetFwRules* firewallRules ) {
		*hasRules = false;
		for( const auto& name : ruleNames() )
		{
			INetFwRule* rule = nullptr;
			if( SUCCEEDED( firewallRules->Item( Bstr( name ), &rule ) ) && rule )
			{
				rule->Release();
				*hasRules = true;
				break;
			}
		}
		return true;
	} );
}



bool WindowsFirewallBackend::addRulesViaNetsh( const QList<Rule>& rules )
{
	for( const auto& rule : rules )
	{
		QStringList arguments{
			QStringLiteral( "advfirewall" ), QStringLiteral( "firewall" ), QStringLiteral( "add" ), QStringLiteral( "rule" ),
			QStringLiteral( "name=%1" ).arg( rule.name ),
			QStringLiteral( "description=%1" ).arg( rule.description ),
			QStringLiteral( "dir=out" ), QStringLiteral( "action=block" ), QStringLiteral( "profile=any" ),
			QStringLiteral( "protocol=%1" ).arg( rule.protocol == Protocol::Tcp ? QStringLiteral( "TCP" )
												 : rule.protocol == Protocol::Udp ? QStringLiteral( "UDP" )
																				  : QStringLiteral( "any" ) ),
			QStringLiteral( "remoteip=%1" ).arg( rule.remoteAddresses ),
			QStringLiteral( "enable=yes" )
		};
		if( rule.remotePorts.isEmpty() == false )
		{
			arguments.append( QStringLiteral( "remoteport=%1" ).arg( rule.remotePorts ) );
		}

		if( QProcess::execute( QStringLiteral( "netsh" ), arguments ) != 0 )
		{
			vCritical() << "netsh failed to add firewall rule" << rule.name;
			return false;
		}
	}

	return true;
}



bool WindowsFirewallBackend::removeRulesViaNetsh()
{
	for( const auto& name : ruleNames() )
	{
		// fails if the rule does not exist, which is fine
		QProcess::execute( QStringLiteral( "netsh" ),
						   { QStringLiteral( "advfirewall" ), QStringLiteral( "firewall" ), QStringLiteral( "delete" ),
							 QStringLiteral( "rule" ), QStringLiteral( "name=%1" ).arg( name ) } );
	}

	return true;
}

