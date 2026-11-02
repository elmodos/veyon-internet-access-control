/*
 * NoopFirewallBackend.h - fallback for platforms without a firewall implementation
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "FirewallBackend.h"

class NoopFirewallBackend : public FirewallBackend
{
public:
	QString name() const override
	{
		return QStringLiteral( "no-op" );
	}

	bool isSupported() const override
	{
		return false;
	}

	bool block( const FirewallPolicy& policy ) override
	{
		Q_UNUSED(policy)
		return false;
	}

	bool unblock() override
	{
		return true;
	}

	std::optional<bool> queryBlocked() const override
	{
		return false;
	}
};
