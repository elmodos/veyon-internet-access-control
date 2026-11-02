/*
 * InternetAccessControlFeaturePlugin.cpp - implementation of InternetAccessControlFeaturePlugin
 *
 * SPDX-License-Identifier: MIT
 */

#include <QAbstractSocket>
#include <QCoreApplication>
#include <QNetworkInterface>
#include <QRegularExpression>
#include <QTranslator>

#include <algorithm>

#include "InternetAccessControlFeaturePlugin.h"
#include "InternetAccessControlConfigurationPage.h"
#include "ComputerControlInterface.h"
#include "FirewallController.h"
#include "Logger.h"
#include "VeyonConfiguration.h"
#include "VeyonServerInterface.h"

InternetAccessControlFeaturePlugin::InternetAccessControlFeaturePlugin( QObject* parent ) :
	QObject( parent ),
	m_translator( installTranslator() ),
	m_configuration( &VeyonCore::config() ),
	m_internetAccessControlFeature(
		QStringLiteral( "InternetAccessControl" ),
		Feature::Flag::Mode | Feature::Flag::AllComponents,
		Feature::Uid( "b7218ac0-4592-4e4b-a256-d71fa9e31002" ),
		Feature::Uid(),
		tr( "Block Internet" ),
		tr( "Unblock Internet" ),
		tr( "Block or allow internet access on selected computers while keeping local network access active." ),
		QStringLiteral( ":/internetaccesscontrol/internet-access-control.png" ) ),
	m_features( { m_internetAccessControlFeature } )
{
	if( VeyonCore::component() == VeyonCore::Component::Server )
	{
		connect( VeyonCore::instance(), &VeyonCore::initialized, this, [this]() {
			auto& firewall = FirewallController::instance();
			firewall.refreshState();
			if( m_configuration.autoUnblockOnExit() && firewall.isInternetBlocked() )
			{
				vDebug() << "Server initialized: clearing existing internet block";
				firewall.unblockInternet();
			}
		} );
	}
}



InternetAccessControlFeaturePlugin::~InternetAccessControlFeaturePlugin()
{
	if( VeyonCore::component() == VeyonCore::Component::Server &&
		m_configuration.autoUnblockOnExit() &&
		FirewallController::instance().isInternetBlocked() )
	{
		FirewallController::instance().unblockInternet();
	}
}



bool InternetAccessControlFeaturePlugin::controlFeature( Feature::Uid featureUid, Operation operation,
														 const QVariantMap& arguments,
														 const ComputerControlInterfaceList& computerControlInterfaces )
{
	Q_UNUSED( arguments )

	if( hasFeature( featureUid ) == false )
	{
		return false;
	}

	FeatureMessage message;

	if( operation == Operation::Start )
	{
		vDebug() << "Blocking internet access on" << computerControlInterfaces.size() << "computer(s)";

		// the settings of the master are authoritative so that they only need to be configured once
		message = FeatureMessage{ featureUid, FeatureCommand::BlockInternet };
		message.addArgument( Argument::BlockMode, m_configuration.blockMode() );
		message.addArgument( Argument::BlockedPorts, m_configuration.blockedPorts() );
		message.addArgument( Argument::AllowedSubnets, m_configuration.allowedSubnets() );
		message.addArgument( Argument::AllowedHosts, m_configuration.allowedHosts() );
		message.addArgument( Argument::BlockDns, m_configuration.blockDnsTraffic() );
		message.addArgument( Argument::ExcludeSupervisorComputer, m_configuration.excludeSupervisorComputer() );
	}
	else if( operation == Operation::Stop )
	{
		vDebug() << "Unblocking internet access on" << computerControlInterfaces.size() << "computer(s)";

		message = FeatureMessage{ featureUid, FeatureCommand::UnblockInternet };
	}
	else
	{
		return false;
	}

	for( const auto& controlInterface : computerControlInterfaces )
	{
		if( controlInterface )
		{
			controlInterface->sendFeatureMessage( message );
		}
	}

	return true;
}



bool InternetAccessControlFeaturePlugin::handleFeatureMessage( VeyonServerInterface& server,
															   const MessageContext& messageContext,
															   const FeatureMessage& message )
{
	Q_UNUSED( server )

	if( message.featureUid() != m_internetAccessControlFeature.uid() )
	{
		return false;
	}

	auto& firewall = FirewallController::instance();

	switch( message.command<FeatureCommand>() )
	{
	case FeatureCommand::UnblockInternet:
		vDebug() << "Server received UnblockInternet command";
		firewall.unblockInternet();
		return true;

	case FeatureCommand::BlockInternet:
	{
		vDebug() << "Server received BlockInternet command";

		// fall back to the local configuration for arguments not sent by the master
		const auto argument = [&message]( Argument key, const QVariant& fallback ) {
			return message.hasArgument( key ) ? message.argument( key ) : fallback;
		};

		// e.g. when the supervisor computer is part of its own computer list in Veyon Master
		if( argument( Argument::ExcludeSupervisorComputer, m_configuration.excludeSupervisorComputer() ).toBool() &&
			isFromLocalComputer( messageContext ) )
		{
			vDebug() << "Ignoring BlockInternet command sent by Veyon Master on this computer";
			return true;
		}

		FirewallController::Settings settings;
		settings.mode = argument( Argument::BlockMode, m_configuration.blockMode() ).toInt() == 1
							? FirewallPolicy::Mode::AllTraffic
							: FirewallPolicy::Mode::WebPortsOnly;
		settings.blockedPorts = argument( Argument::BlockedPorts, m_configuration.blockedPorts() ).toString();
		settings.allowedSubnets = argument( Argument::AllowedSubnets, m_configuration.allowedSubnets() ).toString();
		settings.allowedHosts = argument( Argument::AllowedHosts, m_configuration.allowedHosts() ).toString();
		settings.blockDns = argument( Argument::BlockDns, m_configuration.blockDnsTraffic() ).toBool();

		firewall.blockInternet( settings );
		return true;
	}
	}

	return false;
}



bool InternetAccessControlFeaturePlugin::isFeatureActive( VeyonServerInterface& server, Feature::Uid featureUid ) const
{
	Q_UNUSED( server )

	return featureUid == m_internetAccessControlFeature.uid() &&
		   FirewallController::instance().isInternetBlocked();
}



ConfigurationPage* InternetAccessControlFeaturePlugin::createConfigurationPage()
{
	return new InternetAccessControlConfigurationPage( m_configuration );
}



QTranslator* InternetAccessControlFeaturePlugin::installTranslator()
{
	// Veyon has already installed its own translations and made the configured UI locale the
	// default one, except for an explicitly configured English UI which has to stay untranslated
	static const QRegularExpression configuredLocaleRX{ QStringLiteral( "[^(]*\\(([^)]*)\\)" ) };
	const auto configuredLocaleMatch = configuredLocaleRX.match( VeyonCore::config().uiLanguage() );
	if( configuredLocaleMatch.hasMatch() &&
		QLocale( configuredLocaleMatch.captured( 1 ) ).language() == QLocale::English )
	{
		return nullptr;
	}

	auto translator = new QTranslator( this );
	if( translator->load( QLocale(), QStringLiteral( "internetaccesscontrol" ), QStringLiteral( "_" ),
						  QStringLiteral( ":/internetaccesscontrol/translations" ) ) == false )
	{
		delete translator;
		return nullptr;
	}

	QCoreApplication::installTranslator( translator );

	return translator;
}



bool InternetAccessControlFeaturePlugin::isFromLocalComputer( const MessageContext& messageContext )
{
	const auto socket = qobject_cast<QAbstractSocket*>( messageContext.ioDevice() );
	if( socket == nullptr )
	{
		return false;
	}

	const auto peerAddress = socket->peerAddress();
	if( peerAddress.isLoopback() )
	{
		return true;
	}

	const auto localAddresses = QNetworkInterface::allAddresses();
	return std::any_of( localAddresses.begin(), localAddresses.end(), [&peerAddress]( const QHostAddress& address ) {
		// tolerates IPv4-mapped IPv6 peer addresses of dual-stack sockets
		return address.isEqual( peerAddress, QHostAddress::TolerantConversion );
	} );
}



IMPLEMENT_CONFIG_PROXY(InternetAccessControlConfiguration)
