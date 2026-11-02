/*
 * WindowsFirewallBackend.h - Windows Firewall implementation of FirewallBackend
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <QList>
#include <QStringList>

#include "FirewallBackend.h"

// Adds outbound block rules named Veyon_IAC_* via the Windows Firewall COM API (INetFwPolicy2),
// with netsh as fallback
class WindowsFirewallBackend : public FirewallBackend
{
public:
	QString name() const override
	{
		return QStringLiteral( "Windows Firewall" );
	}

	bool block( const FirewallPolicy& policy ) override;
	bool unblock() override;
	std::optional<bool> queryBlocked() const override;

private:
	enum class Protocol
	{
		Tcp,
		Udp,
		Any
	};

	struct Rule
	{
		QString name;
		QString description;
		Protocol protocol;
		QString remotePorts;
		QString remoteAddresses;
	};

	static QList<Rule> buildRules( const FirewallPolicy& policy );
	static QStringList ruleNames();

	static bool addRulesViaCom( const QList<Rule>& rules );
	static bool removeRulesViaCom();
	static bool hasRulesViaCom( bool* hasRules );

	static bool addRulesViaNetsh( const QList<Rule>& rules );
	static bool removeRulesViaNetsh();
};
