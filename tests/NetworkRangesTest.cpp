/*
 * NetworkRangesTest.cpp - unit tests for NetworkRanges
 *
 * SPDX-License-Identifier: MIT
 */

#include <QTest>

#include "NetworkRanges.h"

class NetworkRangesTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void parseSubnets()
	{
		QStringList invalid;
		const auto subnets = NetworkRanges::parseSubnets(
			QStringLiteral( " 10.0.0.0/8, 192.168.1.5;fd00::/8\nexample.org 10.0.0.0/33 " ), &invalid );

		QCOMPARE( subnets.size(), 3 );
		QCOMPARE( subnets[0].first, QHostAddress( QStringLiteral( "10.0.0.0" ) ) );
		QCOMPARE( subnets[0].second, 8 );
		QCOMPARE( subnets[1].second, 32 );
		QCOMPARE( subnets[2].first.protocol(), QAbstractSocket::IPv6Protocol );
		QCOMPARE( invalid, QStringList( { QStringLiteral( "example.org" ), QStringLiteral( "10.0.0.0/33" ) } ) );
	}

	void defaultLocalNetworksV4()
	{
		const auto excluded = NetworkRanges::parseSubnets(
			QStringLiteral( "10.0.0.0/8,172.16.0.0/12,192.168.0.0/16,127.0.0.0/8,169.254.0.0/16" ) );

		QCOMPARE( NetworkRanges::publicRangesExcluding( excluded, QAbstractSocket::IPv4Protocol ),
				  QStringList( {
					  QStringLiteral( "1.0.0.0-9.255.255.255" ),
					  QStringLiteral( "11.0.0.0-126.255.255.255" ),
					  QStringLiteral( "128.0.0.0-169.253.255.255" ),
					  QStringLiteral( "169.255.0.0-172.15.255.255" ),
					  QStringLiteral( "172.32.0.0-192.167.255.255" ),
					  QStringLiteral( "192.169.0.0-223.255.255.255" ) } ) );
	}

	void loopbackAlwaysExcluded()
	{
		QCOMPARE( NetworkRanges::publicRangesExcluding( {}, QAbstractSocket::IPv4Protocol ),
				  QStringList( {
					  QStringLiteral( "1.0.0.0-126.255.255.255" ),
					  QStringLiteral( "128.0.0.0-169.253.255.255" ),
					  QStringLiteral( "169.255.0.0-223.255.255.255" ) } ) );
	}

	void singleHostsAndOverlaps()
	{
		const auto excluded = NetworkRanges::parseSubnets(
			QStringLiteral( "0.0.0.0/1,128.0.0.0/2,128.1.0.0/16,203.0.113.10,203.0.113.11,203.0.113.13" ) );

		QCOMPARE( NetworkRanges::publicRangesExcluding( excluded, QAbstractSocket::IPv4Protocol ),
				  QStringList( {
					  QStringLiteral( "192.0.0.0-203.0.113.9" ),
					  QStringLiteral( "203.0.113.12" ),
					  QStringLiteral( "203.0.113.14-223.255.255.255" ) } ) );
	}

	void everythingExcluded()
	{
		const auto excluded = NetworkRanges::parseSubnets( QStringLiteral( "0.0.0.0/0,::/0" ) );

		QVERIFY( NetworkRanges::publicRangesExcluding( excluded, QAbstractSocket::IPv4Protocol ).isEmpty() );
		QVERIFY( NetworkRanges::publicRangesExcluding( excluded, QAbstractSocket::IPv6Protocol ).isEmpty() );
	}

	void ipv6()
	{
		QCOMPARE( NetworkRanges::publicRangesExcluding( {}, QAbstractSocket::IPv6Protocol ),
				  QStringList( QStringLiteral( "2000::-3fff:ffff:ffff:ffff:ffff:ffff:ffff:ffff" ) ) );

		const auto excluded = NetworkRanges::parseSubnets(
			QStringLiteral( "2001:db8::/32,2001:db8:1::1,fd00::/8,10.0.0.0/8" ) );
		QCOMPARE( NetworkRanges::publicRangesExcluding( excluded, QAbstractSocket::IPv6Protocol ),
				  QStringList( {
					  QStringLiteral( "2000::-2001:db7:ffff:ffff:ffff:ffff:ffff:ffff" ),
					  QStringLiteral( "2001:db9::-3fff:ffff:ffff:ffff:ffff:ffff:ffff:ffff" ) } ) );
	}
};

QTEST_GUILESS_MAIN( NetworkRangesTest )

#include "NetworkRangesTest.moc"
