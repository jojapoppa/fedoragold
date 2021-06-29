/*
Copyright (C) 2018, The TurtleCoin developers
Copyright (C) 2018, The PinkstarcoinV2 developers
Copyright (C) 2018, The Bittorium developers
Copyright (C) 2019, The B2Bcoin developers

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <cstdio>
#include <ctime>
#include <fstream>
#include <future>
#include <iomanip>
#include <thread>
#include <set>
#include <sstream>

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include "ParseArguments.h"

#include <INode.h>
#include <Logging/LoggerRef.h>
#include <Logging/LoggerManager.h>
#include <NodeRpcProxy/NodeRpcProxy.h>
#include <IWalletLegacy.h>
#include <WalletLegacy/WalletLegacy.h>
#include <WalletLegacy/WalletHelper.h>
#include <Wallet/LegacyKeysImporter.h>
#include <Wallet/WalletRpcServer.h>

#include <Common/SignalHandler.h>

namespace po = boost::program_options;

/* Thanks to https://stackoverflow.com/users/85381/iain for this small command
   line parsing snippet! https://stackoverflow.com/a/868894/8737306 */
char* getCmdOption(char ** begin, char ** end, const std::string & option)
{
    char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

bool cmdOptionExists(char** begin, char** end, const std::string& option)
{
    return std::find(begin, end, option) != end;
}

std::error_code initAndLoadWallet(CryptoNote::IWalletLegacy& wallet, std::istream& walletFile, const std::string& password) {
  CryptoNote::WalletHelper::InitWalletResultObserver initObserver;
  std::future<std::error_code> f_initError = initObserver.initResult.get_future();

  CryptoNote::WalletHelper::IWalletRemoveObserverGuard removeGuard(wallet, initObserver);
  wallet.initAndLoad(walletFile, password);
  auto initError = f_initError.get();

  return initError;
}

std::string tryToOpenWalletOrLoadKeysOrThrow(Logging::LoggerRef& logger, std::unique_ptr<CryptoNote::IWalletLegacy>& wallet, const std::string& walletFile, const std::string& password) {
  std::string keys_file, walletFileName;
  CryptoNote::WalletHelper::prepareFileNames(walletFile, keys_file, walletFileName);

  boost::system::error_code ignore;
  bool keysExists = boost::filesystem::exists(keys_file, ignore);
  bool walletExists = boost::filesystem::exists(walletFileName, ignore);
  if (!walletExists && !keysExists && boost::filesystem::exists(walletFile, ignore)) {
    boost::system::error_code renameEc;
    boost::filesystem::rename(walletFile, walletFileName, renameEc);
    if (renameEc) {
      throw std::runtime_error("failed to rename file '" + walletFile + "' to '" +
        walletFileName + "': " + renameEc.message());
    }

    walletExists = true;
  }

  if (walletExists) {
    logger(Logging::INFO) << "Loading the FED wallet...";
    std::ifstream walletFile;
    walletFile.open(walletFileName, std::ios_base::binary | std::ios_base::in);
    if (walletFile.fail()) {
      throw std::runtime_error(std::string("Error opening the ") +
        "FED wallet file '" + walletFileName + "'");
    }

    auto initError = initAndLoadWallet(*wallet, walletFile, password);

    walletFile.close();
    if (initError) { //bad password, or legacy format
      if (keysExists) {
        std::stringstream ss;
        CryptoNote::importLegacyKeys(keys_file, password, ss);
        boost::filesystem::rename(keys_file, keys_file + ".back");
        boost::filesystem::rename(walletFileName, walletFileName + ".back");

        initError = initAndLoadWallet(*wallet, ss, password);
        if (initError) {
          throw std::runtime_error(std::string("Failed to load the ") +
            "FED wallet: " + initError.message());
        }

        logger(Logging::INFO) << "Storing the FED wallet...";

       try {
          CryptoNote::WalletHelper::storeWallet(*wallet, walletFileName);
        } catch (std::exception& e) {
          logger(Logging::ERROR) << "Failed to store the FED wallet: " << e.what();
          throw std::runtime_error(std::string("Error saving the ") +
            "FED wallet file '" + walletFileName + "'");
        }

        logger(Logging::INFO) << "Stored ok";
        return walletFileName;
      } else { // no keys, wallet error loading
        throw std::runtime_error(std::string("can't load the ") +
          "FED wallet file '" + walletFileName + "', check the password");
      }
    } else { //new wallet ok 
      return walletFileName;
    }
 } else if (keysExists) { //wallet not exists but keys presented
    std::stringstream ss;
    CryptoNote::importLegacyKeys(keys_file, password, ss);
    boost::filesystem::rename(keys_file, keys_file + ".back");

    CryptoNote::WalletHelper::InitWalletResultObserver initObserver;
    std::future<std::error_code> f_initError = initObserver.initResult.get_future();

    CryptoNote::WalletHelper::IWalletRemoveObserverGuard removeGuard(*wallet, initObserver);
    wallet->initAndLoad(ss, password);
    auto initError = f_initError.get();

    removeGuard.removeObserver();
    if (initError) {
      throw std::runtime_error(std::string("Failed to load the ") +
        "FED wallet: " + initError.message());
    }

    logger(Logging::INFO) << "Storing the FED wallet...";

    try {
      CryptoNote::WalletHelper::storeWallet(*wallet, walletFileName);
    } catch(std::exception& e) {
      logger(Logging::ERROR) << "Failed to store the FED wallet: " << e.what();
      throw std::runtime_error(std::string("Error saving the ") +
        "FED wallet file '" + walletFileName + "'");
    }
    logger(Logging::INFO) << "Stored ok";
    return walletFileName;
  } else { //no wallet no keys
    throw std::runtime_error(std::string("The ") + "FED wallet file '" +
      walletFileName + "' is notfound");
  }
}

Config parseArguments(int argc, char **argv)
{
    Config config;

    config.exit = false;
    config.walletGiven = false;
    config.passGiven = false;

    config.host = "127.0.0.1";
    config.port = CryptoNote::RPC_DEFAULT_PORT;

    config.walletFile = "";
    config.walletPass = "";

    if (cmdOptionExists(argv, argv+argc, "-h")
     || cmdOptionExists(argv, argv+argc, "--help"))
    {
        helpMessage();
        config.exit = true;
        return config;
    }

    if (cmdOptionExists(argv, argv+argc, "-v")
     || cmdOptionExists(argv, argv+argc, "--version"))
    {
        versionMessage();
        config.exit = true;
        return config;
    }

    if (cmdOptionExists(argv, argv+argc, "--wallet-file"))
    {
        char *wallet = getCmdOption(argv, argv+argc, "--wallet-file");

        if (!wallet)
        {
            std::cout << "--wallet-file was specified, but no wallet file "
                      << "was given!" << std::endl;

            helpMessage();
            config.exit = true;
            return config;
        }

        config.walletFile = std::string(wallet);
        config.walletGiven = true;
    }

    if (cmdOptionExists(argv, argv+argc, "--password"))
    {
        char *password = getCmdOption(argv, argv+argc, "--password");

        if (!password)
        {
            std::cout << "--password was specified, but no password was "
                      << "given!" << std::endl;

            helpMessage();
            config.exit = true;
            return config;
        }

        config.walletPass = std::string(password);
        config.passGiven = true;
    }

    if (cmdOptionExists(argv, argv+argc, "--remote-daemon"))
    {
        char *url = getCmdOption(argv, argv + argc, "--remote-daemon");

        /* No url following --remote-daemon */
        if (!url)
        {
            std::cout << "--remote-daemon was specified, but no daemon was "
                      << "given!" << std::endl;

            helpMessage();

            config.exit = true;
        }
        else
        {
            std::string urlString(url);

            /* Get the index of the ":" */
            size_t splitter = urlString.find_first_of(":");

            /* No ":" present */
            if (splitter == std::string::npos)
            {
                config.host = urlString;
            }
            else
            {
                /* Host is everything before ":" */
                config.host = urlString.substr(0, splitter);

                /* Port is everything after ":" */
                std::string port = urlString.substr(splitter + 1,   
                                                    std::string::npos);

                try
                {
                    config.port = std::stoi(port);
                }
                catch (...)
                {
                    std::cout << "Failed to parse daemon port!" << std::endl;
                    config.exit = true;
                }
            }
        }
    }

    if (cmdOptionExists(argv, argv+argc, "--rpc-bind-port")) {

      if (!cmdOptionExists(argv, argv+argc, "--password")) {
        std::cout << "\n\nUse the parameter --password.\n";
        return config;
      }
 
      if (!cmdOptionExists(argv, argv+argc, "--wallet-file")) {
        std::cout << "FED wallet file not set.";
        return config;
      }

      std::string wallet_file = config.walletFile;
      std::string wallet_password = config.walletPass;

      Logging::LoggerManager logManager;
      Logging::LoggerRef logger(logManager, "simplewallet"); 
      CryptoNote::Currency currency = CryptoNote::CurrencyBuilder(logManager).testnet(false).currency();

      std::unique_ptr<CryptoNote::INode> node(new CryptoNote::NodeRpcProxy(config.host,
        (unsigned short)config.port, logger.getLogger()));
 
      std::promise<std::error_code> errorPromise;
      std::future<std::error_code> error = errorPromise.get_future();
      auto callback = [&errorPromise](std::error_code e) {errorPromise.set_value(e); };
      node->init(callback);
      if (error.get()) {
        std::cout << "failed to init NodeRPCProxy";
        return config;
      }
 
      std::unique_ptr<CryptoNote::IWalletLegacy>
        wallet(new CryptoNote::WalletLegacy(currency, *node.get(), logger));
 
      std::string walletFileName;
      try  {
        walletFileName = ::tryToOpenWalletOrLoadKeysOrThrow(logger, wallet, wallet_file, wallet_password);
 
        std::cout << "\n\nAvailable balance: " << currency.formatAmount(wallet->actualBalance())
          << " FED" << "\n" << "Locked amount: " << currency.formatAmount(wallet->pendingBalance())
          << " FED" << "\n";
 
        std::cout << "Loaded ok";
      } catch (const std::exception& e)  {
        std::cout << "The FED wallet initialize failed: " << e.what();
        return config;
      }

      po::variables_map vm;
      System::Dispatcher dispatcher;
      Tools::wallet_rpc_server wrpc(dispatcher, logManager, *wallet, *node, currency, walletFileName);
 
      if (!wrpc.init(vm)) {
        std::cout << "Failed to initialize the FED wallet rpc server";
        return config;
      }
 
      Tools::SignalHandler::install([&wrpc] {
        wrpc.send_stop_signal();
      });
 
      std::cout << "Starting the FED wallet rpc server";
      wrpc.run();
      std::cout << "Stopped the FED wallet rpc server";
 
      try {
        std::cout << "Storing the FED wallet...";
        CryptoNote::WalletHelper::storeWallet(*wallet, walletFileName);
        std::cout << "Stored ok";
      } catch (const std::exception& e) {
        std::cout << "Failed to store the FED wallet: " << e.what();
        return config;
      }
    }

    return config;
}

void versionMessage() {
    std::cout << "FedoraGold v" << PROJECT_VERSION << " Simplewallet"
              << std::endl;
}

void helpMessage()
{
    versionMessage();

    std::cout << std::endl << "simplewallet [--version] [--help] "
              << "[--remote-daemon <url>] [--wallet-file <file>] "
              << "[--password <pass>]"
              << "[--rpc-bind-port <port>]"
              << std::endl << std::endl
              << "Commands:" << std::endl 
              << "  -h, " << std::left << std::setw(25) << "--help" << "Display this help message and exit"
              << std::endl << "  -v, " << std::left << std::setw(25)
              << "--version" << "Display the version information and exit"
              << std::endl << "      " << std::left << std::setw(25)
              << "--remote-daemon <url>" << "Connect to the remote daemon at "
              << "<url>:<port>"
              << std::endl << "      " << std::left << std::setw(25)
              << "--wallet-file <file>" << "Open the wallet <file>"
              << std::endl << "      " << std::left << std::setw(25)
              << "--password <pass>" << "Use the password <pass> to open the "
              << "wallet" << std::endl << std::left << std::setw(25)
              << "--rpc-bind-port" << "Run in SimpleWalletRPC mode on this port" << std::endl;
}

