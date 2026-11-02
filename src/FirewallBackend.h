/*
 * FirewallBackend.h - interface for platform specific firewall implementations
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <QString>

#include <optional>

#include "NetworkRanges.h"

// Everything a backend needs to block internet access. FirewallController fills it from the
// plugin settings, so backends don't have to parse or validate anything themselves.
struct FirewallPolicy
{
	enum class Mode
	{
		// block TCP and UDP traffic to blockedPorts
		WebPortsOnly = 0,
		// block all traffic regardless of protocol and port
		AllTraffic = 1
	};

	Mode mode = Mode::WebPortsOnly;

	// validated, non-empty list of ports and port ranges, e.g. "80,443,8000-8999"
	QString blockedPorts;

	// IPv4 and IPv6 destinations which must stay reachable: the configured local networks plus
	// the resolved addresses of the allowed hosts. Loopback and link-local addresses have to be
	// kept reachable by every backend even if not listed here.
	QList<NetworkRanges::Subnet> allowedNetworks;

	// additionally block DNS (UDP port 53) to destinations outside allowedNetworks
	bool blockDns = false;
};


// Implemented once per platform and created by FirewallController. All methods are called from
// the Veyon Server, which runs with administrative privileges on Windows (SYSTEM), but only with
// the privileges of the session user on Linux Wayland sessions.
class FirewallBackend
{
public:
	virtual ~FirewallBackend() = default;

	// short name for log messages, e.g. "Windows Firewall"
	virtual QString name() const = 0;

	// false for backends which can't block anything on the current platform
	virtual bool isSupported() const
	{
		return true;
	}

	// Blocks outbound traffic to all destinations outside policy.allowedNetworks. Must replace an
	// existing block and must never block traffic to allowedNetworks, loopback or link-local
	// addresses, as Veyon itself would stop working otherwise.
	virtual bool block( const FirewallPolicy& policy ) = 0;

	// Removes everything block() created. Must succeed if nothing is blocked.
	virtual bool unblock() = 0;

	// Reads the actual state from the system, as rules may be changed outside of Veyon.
	// Returns std::nullopt if the state can't be determined.
	virtual std::optional<bool> queryBlocked() const = 0;
};
