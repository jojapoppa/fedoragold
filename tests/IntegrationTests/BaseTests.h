// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <gtest/gtest.h>
#include <future>

#include <Logging/ConsoleLogger.h>
#include <System/Dispatcher.h>
#include "CryptoNoteCore/Currency.h"

#include "../IntegrationTestLib/TestNetwork.h"

#include "Logging/LoggerManager.h"
using namespace Logging;
static LoggerManager olManager;
static LoggerRef ollogger(olManager, "BaseTest");

namespace Tests {

class BaseTest : public testing::Test {
public:

  BaseTest() :
    currency(CryptoNote::CurrencyBuilder(ollogger.getLogger()).testnet(true).currency()),
    network(dispatcher, currency) {
  }

protected:

  virtual void TearDown() override {
    network.shutdown();
  }

  System::Dispatcher& getDispatcher() {
    return dispatcher;
  }

  System::Dispatcher dispatcher;
  CryptoNote::Currency currency;
  TestNetwork network;
};

}
