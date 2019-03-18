// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <System/IpAddress.h>
#include <gtest/gtest.h>

using namespace System;

TEST(IpAddressTest, value) {
  IpAddress address1(0x00000000);
  ASSERT_EQ(0x00000000, address1.getValue());
  IpAddress address2(0xfefdfcfb);
  ASSERT_EQ(0xfefdfcfb, address2.getValue());
  IpAddress address3 = address1;
  ASSERT_EQ(0x00000000, address3.getValue());
  IpAddress address4 = address2;
  ASSERT_EQ(0xfefdfcfb, address4.getValue());
  address3 = address2;
  ASSERT_EQ(0xfefdfcfb, address3.getValue());
  address4 = address1;
  ASSERT_EQ(0x00000000, address4.getValue());
}

TEST(IpAddressTest, dottedDecimal) {
  ASSERT_EQ(0x00000000, IpAddress("0.0.0.0").getValue());
  ASSERT_EQ(0x01020304, IpAddress("1.2.3.4").getValue());
  ASSERT_EQ(0x7f000001, IpAddress("127.0.0.1").getValue());
  ASSERT_EQ(0xfefdfcfb, IpAddress("254.253.252.251").getValue());
  ASSERT_EQ(0xffffffff, IpAddress("255.255.255.255").getValue());
  ASSERT_EQ("0.0.0.0", IpAddress(0x00000000).toDottedDecimal());
  ASSERT_EQ("1.2.3.4", IpAddress(0x01020304).toDottedDecimal());
  ASSERT_EQ("127.0.0.1", IpAddress(0x7f000001).toDottedDecimal());
  ASSERT_EQ("254.253.252.251", IpAddress(0xfefdfcfb).toDottedDecimal());
  ASSERT_EQ("255.255.255.255", IpAddress(0xffffffff).toDottedDecimal());
  ASSERT_THROW(IpAddress(".0.0.0.0"), std::runtime_error);
  ASSERT_THROW(IpAddress("0..0.0.0"), std::runtime_error);
  ASSERT_THROW(IpAddress("0.0.0"), std::runtime_error);
  ASSERT_THROW(IpAddress("0.0.0."), std::runtime_error);
  ASSERT_THROW(IpAddress("0.0.0.0."), std::runtime_error);
  ASSERT_THROW(IpAddress("0.0.0.0.0"), std::runtime_error);
  ASSERT_THROW(IpAddress("0.0.0.00"), std::runtime_error);
  ASSERT_THROW(IpAddress("0.0.0.01"), std::runtime_error);
  ASSERT_THROW(IpAddress("0.0.0.256"), std::runtime_error);
  ASSERT_THROW(IpAddress("00.0.0.0"), std::runtime_error);
  ASSERT_THROW(IpAddress("01.0.0.0"), std::runtime_error);
  ASSERT_THROW(IpAddress("256.0.0.0"), std::runtime_error);
}

TEST(IpAddressTest, isLoopback) {
  // 127.0.0.0/8
  ASSERT_TRUE(IpAddress("127.0.0.1").isLoopback());
  ASSERT_TRUE(IpAddress("127.1.1.1").isLoopback());
  ASSERT_TRUE(IpAddress("127.1.0.0").isLoopback());
  ASSERT_TRUE(IpAddress("127.255.255.255").isLoopback());

  ASSERT_FALSE(IpAddress("255.0.0.0").isLoopback());
  ASSERT_FALSE(IpAddress("255.255.255.255").isLoopback());
  ASSERT_FALSE(IpAddress("128.1.0.0").isLoopback());
  ASSERT_FALSE(IpAddress("192.168.1.1").isLoopback());
  ASSERT_FALSE(IpAddress("10.0.0.1").isLoopback());
}

TEST(IpAddressTest, isPrivate) {
  // 10.0.0.0/8
  ASSERT_TRUE(IpAddress("10.0.0.0").isPrivate());
  ASSERT_TRUE(IpAddress("10.0.0.1").isPrivate());
  ASSERT_TRUE(IpAddress("10.0.0.255").isPrivate());
  ASSERT_TRUE(IpAddress("10.255.255.255").isPrivate());

  ASSERT_FALSE(IpAddress("11.0.0.255").isPrivate());
  ASSERT_FALSE(IpAddress("9.0.0.0").isPrivate());
  ASSERT_FALSE(IpAddress("138.0.0.1").isPrivate());
  ASSERT_FALSE(IpAddress("255.255.255.255").isPrivate());

  // 172.16.0.0/12 
  ASSERT_TRUE(IpAddress("172.16.0.255").isPrivate());
  ASSERT_TRUE(IpAddress("172.17.0.0").isPrivate());
  ASSERT_TRUE(IpAddress("172.19.1.1").isPrivate());
  ASSERT_TRUE(IpAddress("172.31.255.255").isPrivate());

  ASSERT_FALSE(IpAddress("172.32.0.0").isPrivate());
  ASSERT_FALSE(IpAddress("172.32.0.1").isPrivate());
  ASSERT_FALSE(IpAddress("172.15.0.0").isPrivate());
  ASSERT_FALSE(IpAddress("172.15.255.255").isPrivate());

  // 192.168.0.0/16
  ASSERT_TRUE(IpAddress("192.168.0.0").isPrivate());
  ASSERT_TRUE(IpAddress("192.168.1.1").isPrivate());
  ASSERT_TRUE(IpAddress("192.168.100.100").isPrivate());
  ASSERT_TRUE(IpAddress("192.168.255.255").isPrivate());

  ASSERT_FALSE(IpAddress("192.167.255.255").isPrivate());
  ASSERT_FALSE(IpAddress("191.168.255.255").isPrivate());
  ASSERT_FALSE(IpAddress("192.169.255.255").isPrivate());
  ASSERT_FALSE(IpAddress("192.169.0.0").isPrivate());

  ASSERT_FALSE(IpAddress("255.255.255.255").isPrivate());

}
