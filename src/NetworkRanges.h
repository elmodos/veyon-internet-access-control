/*
 * NetworkRanges.h - address range arithmetic for building firewall rules
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <QHostAddress>
#include <QList>
#include <QPair>
#include <QStringList>

namespace NetworkRanges {

using Subnet = QPair<QHostAddress, int>;

// Parses a list of IP addresses and CIDR subnets separated by commas, semicolons or whitespace.
// Entries which cannot be parsed are appended to invalidEntries (if given).
QList<Subnet> parseSubnets( const QString& list, QStringList* invalidEntries = nullptr );

// Returns the public address space of the given protocol (IPv4: 1.0.0.0-223.255.255.255,
// IPv6: 2000::/3) without the excluded subnets, loopback and link-local addresses.
// Each entry is formatted for Windows Firewall, i.e. either a single address or "first-last".
// An empty list means that nothing is left to block.
QStringList publicRangesExcluding( const QList<Subnet>& excluded,
								   QAbstractSocket::NetworkLayerProtocol protocol );

} // namespace NetworkRanges
