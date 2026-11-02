/*
 * FirewallController.h - platform independent internet blocking logic
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <QString>

#include <memory>

#include "FirewallBackend.h"

class FirewallController
{
public:
	// settings as configured in Veyon Configurator
	struct Settings
	{
		FirewallPolicy::Mode mode = FirewallPolicy::Mode::WebPortsOnly;
		QString blockedPorts;
		QString allowedSubnets;
		QString allowedHosts;
		bool blockDns = false;
	};

	static FirewallController& instance();

	bool isSupported() const;

	bool blockInternet( const Settings& settings );
	bool unblockInternet();

	bool isInternetBlocked() const
	{
		return m_isBlocked;
	}

	// re-reads the blocking state from the system
	void refreshState();

private:
	FirewallController();
	~FirewallController();

	FirewallController( const FirewallController& ) = delete;
	FirewallController& operator=( const FirewallController& ) = delete;

	static std::unique_ptr<FirewallBackend> createBackend();
	static FirewallPolicy createPolicy( const Settings& settings );

	const std::unique_ptr<FirewallBackend> m_backend;
	bool m_isBlocked = false;
};
