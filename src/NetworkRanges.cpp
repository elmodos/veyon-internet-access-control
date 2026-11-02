/*
 * NetworkRanges.cpp - address range arithmetic for building firewall rules
 *
 * SPDX-License-Identifier: MIT
 */

#include <QRegularExpression>

#include <algorithm>

#include "NetworkRanges.h"

namespace {

// 128 bit address value; IPv4 addresses only use the lower 32 bits
struct Address
{
	quint64 hi = 0;
	quint64 lo = 0;

	bool operator<( const Address& other ) const
	{
		return hi < other.hi || ( hi == other.hi && lo < other.lo );
	}

	bool operator==( const Address& other ) const
	{
		return hi == other.hi && lo == other.lo;
	}

	bool operator<=( const Address& other ) const
	{
		return *this < other || *this == other;
	}

	Address next() const
	{
		return lo == ~quint64( 0 ) ? Address{ hi + 1, 0 } : Address{ hi, lo + 1 };
	}

	Address previous() const
	{
		return lo == 0 ? Address{ hi - 1, ~quint64( 0 ) } : Address{ hi, lo - 1 };
	}
};

struct Range
{
	Address first;
	Address last;
};

quint64 lowBitsMask( int bits )
{
	return bits >= 64 ? ~quint64( 0 ) : ( quint64( 1 ) << bits ) - 1;
}

int addressBits( QAbstractSocket::NetworkLayerProtocol protocol )
{
	return protocol == QAbstractSocket::IPv4Protocol ? 32 : 128;
}

Address toAddress( const QHostAddress& hostAddress )
{
	if( hostAddress.protocol() == QAbstractSocket::IPv4Protocol )
	{
		return { 0, hostAddress.toIPv4Address() };
	}

	const auto ipv6 = hostAddress.toIPv6Address();
	Address address;
	for( int i = 0; i < 8; ++i )
	{
		address.hi = ( address.hi << 8 ) | ipv6[i];
		address.lo = ( address.lo << 8 ) | ipv6[i + 8];
	}
	return address;
}

QHostAddress toHostAddress( const Address& address, QAbstractSocket::NetworkLayerProtocol protocol )
{
	if( protocol == QAbstractSocket::IPv4Protocol )
	{
		return QHostAddress( quint32( address.lo ) );
	}

	Q_IPV6ADDR ipv6;
	for( int i = 0; i < 8; ++i )
	{
		ipv6[7 - i] = quint8( address.hi >> ( 8 * i ) );
		ipv6[15 - i] = quint8( address.lo >> ( 8 * i ) );
	}
	return QHostAddress( ipv6 );
}

Range subnetRange( const NetworkRanges::Subnet& subnet )
{
	const auto hostBits = addressBits( subnet.first.protocol() ) - subnet.second;
	const Address hostMask{ hostBits > 64 ? lowBitsMask( hostBits - 64 ) : 0, lowBitsMask( hostBits ) };

	const auto address = toAddress( subnet.first );
	const Address first{ address.hi & ~hostMask.hi, address.lo & ~hostMask.lo };
	return { first, { first.hi | hostMask.hi, first.lo | hostMask.lo } };
}

QString formatRange( const Range& range, QAbstractSocket::NetworkLayerProtocol protocol )
{
	const auto first = toHostAddress( range.first, protocol ).toString();
	if( range.first == range.last )
	{
		return first;
	}
	return QStringLiteral( "%1-%2" ).arg( first, toHostAddress( range.last, protocol ).toString() );
}

} // namespace


QList<NetworkRanges::Subnet> NetworkRanges::parseSubnets( const QString& list, QStringList* invalidEntries )
{
	static const QRegularExpression separators{ QStringLiteral( "[,;\\s]+" ) };

	QList<Subnet> subnets;
	const auto entries = list.split( separators, Qt::SkipEmptyParts );
	for( const auto& entry : entries )
	{
		Subnet subnet{ QHostAddress(), -1 };
		if( entry.contains( QLatin1Char( '/' ) ) )
		{
			subnet = QHostAddress::parseSubnet( entry );
		}
		else if( QHostAddress address( entry ); address.isNull() == false )
		{
			subnet = { address, addressBits( address.protocol() ) };
		}

		if( subnet.first.isNull() || subnet.second < 0 ||
			( subnet.first.protocol() != QAbstractSocket::IPv4Protocol &&
			  subnet.first.protocol() != QAbstractSocket::IPv6Protocol ) )
		{
			if( invalidEntries )
			{
				invalidEntries->append( entry );
			}
			continue;
		}

		subnets.append( subnet );
	}

	return subnets;
}



QStringList NetworkRanges::publicRangesExcluding( const QList<Subnet>& excluded,
												  QAbstractSocket::NetworkLayerProtocol protocol )
{
	Range universe;
	QList<Subnet> exclusions;

	if( protocol == QAbstractSocket::IPv4Protocol )
	{
		// everything below 1.0.0.0 and from 224.0.0.0 (multicast, reserved, broadcast) is never blocked
		universe = { { 0, 0x01000000 }, { 0, 0xDFFFFFFF } };
		exclusions = parseSubnets( QStringLiteral( "127.0.0.0/8,169.254.0.0/16" ) );
	}
	else
	{
		// global unicast space, excludes loopback, link-local, ULA and multicast by definition
		universe = subnetRange( QHostAddress::parseSubnet( QStringLiteral( "2000::/3" ) ) );
	}

	for( const auto& subnet : excluded )
	{
		if( subnet.first.protocol() == protocol )
		{
			exclusions.append( subnet );
		}
	}

	QList<Range> exclusionRanges;
	exclusionRanges.reserve( exclusions.size() );
	for( const auto& subnet : std::as_const( exclusions ) )
	{
		exclusionRanges.append( subnetRange( subnet ) );
	}
	std::sort( exclusionRanges.begin(), exclusionRanges.end(),
			   []( const Range& a, const Range& b ) { return a.first < b.first; } );

	QStringList result;
	auto cursor = universe.first;
	for( const auto& range : std::as_const( exclusionRanges ) )
	{
		if( range.last < cursor )
		{
			continue;
		}
		if( universe.last < range.first )
		{
			break;
		}
		if( cursor < range.first )
		{
			result.append( formatRange( { cursor, range.first.previous() }, protocol ) );
		}
		if( universe.last <= range.last )
		{
			return result;
		}
		cursor = range.last.next();
	}

	result.append( formatRange( { cursor, universe.last }, protocol ) );

	return result;
}
