/*
 * InternetAccessControlFeaturePlugin.h - declaration of InternetAccessControlFeaturePlugin
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "ConfigurationPagePluginInterface.h"
#include "FeatureProviderInterface.h"
#include "InternetAccessControlConfiguration.h"

class QTranslator;

class InternetAccessControlFeaturePlugin : public QObject,
										   public FeatureProviderInterface,
										   public PluginInterface,
										   public ConfigurationPagePluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA( IID "io.veyon.Veyon.Plugins.InternetAccessControl" )
	Q_INTERFACES( PluginInterface FeatureProviderInterface ConfigurationPagePluginInterface )

public:
	enum class Argument
	{
		BlockMode,
		BlockedPorts,
		AllowedSubnets,
		AllowedHosts,
		BlockDns,
		ExcludeSupervisorComputer
	};
	Q_ENUM( Argument )

	explicit InternetAccessControlFeaturePlugin( QObject* parent = nullptr );
	~InternetAccessControlFeaturePlugin() override;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{ QStringLiteral("e82fa510-39b1-419a-9e11-8845e2c56a01") };
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( PLUGIN_VERSION_MAJOR, PLUGIN_VERSION_MINOR, PLUGIN_VERSION_PATCH );
	}

	QString name() const override
	{
		return QStringLiteral("InternetAccessControl");
	}

	QString description() const override
	{
		return tr( "Block or restore internet access on managed computers" );
	}

	QString vendor() const override
	{
		return QStringLiteral("Veyon Internet Access Control contributors");
	}

	QString copyright() const override
	{
		return QStringLiteral("Veyon Internet Access Control contributors (MIT License)");
	}

	const FeatureList& featureList() const override
	{
		return m_features;
	}

	bool controlFeature( Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						 const ComputerControlInterfaceList& computerControlInterfaces ) override;

	bool handleFeatureMessage( VeyonServerInterface& server,
							   const MessageContext& messageContext,
							   const FeatureMessage& message ) override;

	bool isFeatureActive( VeyonServerInterface& server, Feature::Uid featureUid ) const override;

	ConfigurationPage* createConfigurationPage() override;

private:
	enum class FeatureCommand
	{
		BlockInternet,
		UnblockInternet
	};

	QTranslator* installTranslator();

	static bool isFromLocalComputer( const MessageContext& messageContext );

	// must be initialized first so that the feature names below get translated
	QTranslator* m_translator;
	InternetAccessControlConfiguration m_configuration;
	const Feature m_internetAccessControlFeature;
	const FeatureList m_features;
};
