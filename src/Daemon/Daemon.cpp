// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "version.h"

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include "DaemonCommandsHandler.h"

#include "Common/SignalHandler.h"
#include "Common/PathTools.h"
#include "Common/CommandLine.h"
#include "crypto/hash.h"
#include "CryptoNoteCore/Core.h"
#include "CryptoNoteCore/CoreConfig.h"
#include "CryptoNoteCore/CryptoNoteTools.h"
#include "CryptoNoteCore/Currency.h"
#include "CryptoNoteCore/MinerConfig.h"
#include "CryptoNoteProtocol/CryptoNoteProtocolHandler.h"
#include "P2p/NetNode.h"
#include "P2p/NetNodeConfig.h"
#include "Rpc/RpcServer.h"
#include "Rpc/RpcServerConfig.h"
#include "version.h"

#if defined(__APPLE__)
#define _XOPEN_SOURCE
#include <stdio.h>
#include <signal.h>
#include "execinfo.h"
#include <ucontext.h>
#include <unistd.h>
#include <dlfcn.h>
#endif

#include "Logging/LoggerManager.h"

#if defined(WIN32)
#include <crtdbg.h>
#include <io.h>
#else
#include <unistd.h>
#endif

using Common::JsonValue;
using namespace CryptoNote;
using namespace Logging;

namespace po = boost::program_options;

namespace
{
  const command_line::arg_descriptor<std::string> arg_config_file = {"config-file", "Specify configuration file", std::string(CryptoNote::CRYPTONOTE_NAME) + ".conf"};
  const command_line::arg_descriptor<bool>        arg_os_version  = {"os-version", ""};
  const command_line::arg_descriptor<std::string> arg_log_file    = {"log-file", "", ""};
  const command_line::arg_descriptor<int>         arg_log_level   = {"log-level", "", 2}; // info level
  const command_line::arg_descriptor<bool>        arg_console     = {"no-console", "Disable daemon console commands"};
  const command_line::arg_descriptor<bool>        arg_testnet_on  = {"testnet", "Used to deploy test nets. Checkpoints and hardcoded seeds are ignored, "
    "network id is changed. Use it with --data-dir flag. The wallet must be launched with --testnet flag.", false};
  const command_line::arg_descriptor<bool>        arg_print_genesis_tx = { "print-genesis-tx", "Prints genesis' block tx hex to insert it to config and exits" };
}

bool command_line_preprocessor(const boost::program_options::variables_map& vm, LoggerRef& logger);

void print_genesis_tx_hex(Logging::LoggerRef &logger) {
  CryptoNote::Transaction tx = CryptoNote::CurrencyBuilder(logger.getLogger()).generateGenesisTransaction();
  CryptoNote::BinaryArray txb = CryptoNote::toBinaryArray(tx);
  std::string tx_hex = Common::toHex(txb);

  std::cout << "Insert this line into your coin configuration file as is: " << std::endl;
  std::cout << "const char GENESIS_COINBASE_TX_HEX[] = \"" << tx_hex << "\";" << std::endl;

  return;
}

JsonValue buildLoggerConfiguration(Level level, const std::string& logfile) {
  JsonValue loggerConfiguration(JsonValue::OBJECT);
  loggerConfiguration.insert("globalLevel", static_cast<int64_t>(level));

  JsonValue& cfgLoggers = loggerConfiguration.insert("loggers", JsonValue::ARRAY);

  JsonValue& fileLogger = cfgLoggers.pushBack(JsonValue::OBJECT);
  fileLogger.insert("type", "file");
  fileLogger.insert("filename", logfile);
  fileLogger.insert("level", static_cast<int64_t>(TRACE));

  JsonValue& consoleLogger = cfgLoggers.pushBack(JsonValue::OBJECT);
  consoleLogger.insert("type", "console");
  consoleLogger.insert("level", static_cast<int64_t>(TRACE));
  consoleLogger.insert("pattern", "%T %L ");

  return loggerConfiguration;
}

#if defined(__APPLE__)
void err(int ernum, const char *s) {
  fprintf(stderr, "error %d: %s\n", ernum, s);
}
/* Resolve symbol name and source location given the path to the executable 
   and an address */
int addr2line(char const * const program_name, void const * const addr)
{
  char addr2line_cmd[512] = {0};
 
  /* have addr2line map the address to the relent line in the code */
  #ifdef __APPLE__
    /* apple does things differently... */
    sprintf(addr2line_cmd,"atos -o %.256s %p", program_name, addr); 
  #else
    sprintf(addr2line_cmd,"addr2line -f -p -e %.256s %p", program_name, addr); 
  #endif
 
  /* This will print a nicely formatted string specifying the
     function and source line of the address */
  return system(addr2line_cmd);
}
#define MAX_STACK_FRAMES 64
static void *stack_traces[MAX_STACK_FRAMES];
void posix_print_stack_trace()
{
  int i, trace_size = 0;
  char **messages = (char **)NULL;
 
  trace_size = backtrace(stack_traces, MAX_STACK_FRAMES);
  messages = backtrace_symbols(stack_traces, trace_size);
 
  /* skip the first couple stack frames (as they are this function and
     our handler) and also skip the last frame as it's (always?) junk. */
  // for (i = 3; i < (trace_size - 1); ++i)
  // we'll use this for now so you can see what's going on
  for (i = 0; i < trace_size; ++i)
  {
    if (addr2line("fedoragold_daemon", stack_traces[i]) != 0)
    {
      printf("  error determining line # for: %s\n", messages[i]);
    }
 
  }
  if (messages) { free(messages); } 
}
void posix_signal_handler(int sig, siginfo_t *siginfo, void *context)
{
  (void)context;
  switch(sig)
  {
    case SIGSEGV:
      fputs("Caught SIGSEGV: Segmentation Fault\n", stderr);
      break;
    case SIGINT:
      fputs("Caught SIGINT: Interactive attention signal, (usually ctrl+c)\n",
            stderr);
      break;
    case SIGFPE:
      switch(siginfo->si_code)
      {
        case FPE_INTDIV:
          fputs("Caught SIGFPE: (integer divide by zero)\n", stderr);
          break;
        case FPE_INTOVF:
          fputs("Caught SIGFPE: (integer overflow)\n", stderr);
          break;
        case FPE_FLTDIV:
          fputs("Caught SIGFPE: (floating-point divide by zero)\n", stderr);
          break;
        case FPE_FLTOVF:
          fputs("Caught SIGFPE: (floating-point overflow)\n", stderr);
          break;
        case FPE_FLTUND:
          fputs("Caught SIGFPE: (floating-point underflow)\n", stderr);
          break;
        case FPE_FLTRES:
          fputs("Caught SIGFPE: (floating-point inexact result)\n", stderr);
          break;
        case FPE_FLTINV:
          fputs("Caught SIGFPE: (floating-point invalid operation)\n", stderr);
          break;
        case FPE_FLTSUB:
          fputs("Caught SIGFPE: (subscript out of range)\n", stderr);
          break;
        default:
          fputs("Caught SIGFPE: Arithmetic Exception\n", stderr);
          break;
      }
    case SIGILL:
      switch(siginfo->si_code)
      {
        case ILL_ILLOPC:
          fputs("Caught SIGILL: (illegal opcode)\n", stderr);
          break;
        case ILL_ILLOPN:
          fputs("Caught SIGILL: (illegal operand)\n", stderr);
          break;
        case ILL_ILLADR:
          fputs("Caught SIGILL: (illegal addressing mode)\n", stderr);
          break;
        case ILL_ILLTRP:
          fputs("Caught SIGILL: (illegal trap)\n", stderr);
          break;
        case ILL_PRVOPC:
          fputs("Caught SIGILL: (privileged opcode)\n", stderr);
          break;
        case ILL_PRVREG:
          fputs("Caught SIGILL: (privileged register)\n", stderr);
          break;
        case ILL_COPROC:
          fputs("Caught SIGILL: (coprocessor error)\n", stderr);
          break;
        case ILL_BADSTK:
          fputs("Caught SIGILL: (internal stack error)\n", stderr);
          break;
        default:
          fputs("Caught SIGILL: Illegal Instruction\n", stderr);
          break;
      }
      break;
    case SIGTERM:
      fputs("Caught SIGTERM: a termination request was sent to the program\n",
            stderr);
      break;
    case SIGABRT:
      fputs("Caught SIGABRT: usually caused by an abort() or assert()\n", stderr);
      break;
    default:
      break;
  }
  posix_print_stack_trace();
  _Exit(1);
}
static uint8_t alternate_stack[SIGSTKSZ];
void set_signal_handler()
{
  /* setup alternate stack */
  {
    stack_t ss = {};
    /* malloc is usually used here, I'm not 100% sure my static allocation
       is valid but it seems to work just fine. */
    ss.ss_sp = (void*)alternate_stack;
    ss.ss_size = SIGSTKSZ;
    ss.ss_flags = 0;
 
    if (sigaltstack(&ss, NULL) != 0) { err(1, "sigaltstack"); }
  }
 
  /* register our signal handlers */
  {
    struct sigaction sig_action = {};
    sig_action.sa_sigaction = posix_signal_handler;
    sigemptyset(&sig_action.sa_mask);
 
    #ifdef __APPLE__
        /* for some reason we backtrace() doesn't work on osx
           when we use an alternate stack */
        sig_action.sa_flags = SA_SIGINFO;
    #else
        sig_action.sa_flags = SA_SIGINFO | SA_ONSTACK;
    #endif
 
    if (sigaction(SIGSEGV, &sig_action, NULL) != 0) { err(1, "sigaction"); }
    if (sigaction(SIGFPE,  &sig_action, NULL) != 0) { err(1, "sigaction"); }
    if (sigaction(SIGINT,  &sig_action, NULL) != 0) { err(1, "sigaction"); }
    if (sigaction(SIGILL,  &sig_action, NULL) != 0) { err(1, "sigaction"); }
    if (sigaction(SIGTERM, &sig_action, NULL) != 0) { err(1, "sigaction"); }
    if (sigaction(SIGABRT, &sig_action, NULL) != 0) { err(1, "sigaction"); }
  }
}
#endif

int main(int argc, char* argv[])
{

#ifdef WIN32
  _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

#if defined(__APPLE__)
  set_signal_handler();
#endif

  LoggerManager logManager;
  LoggerRef logger(logManager, "daemon");

  try {

    po::options_description desc_cmd_only("Command line options");
    po::options_description desc_cmd_sett("Command line options and settings options");

    command_line::add_arg(desc_cmd_only, command_line::arg_help);
    command_line::add_arg(desc_cmd_only, command_line::arg_version);
    command_line::add_arg(desc_cmd_only, arg_os_version);
    // tools::get_default_data_dir() can't be called during static initialization
    command_line::add_arg(desc_cmd_only, command_line::arg_data_dir, Tools::getDefaultDataDirectory());
    command_line::add_arg(desc_cmd_only, arg_config_file);

    command_line::add_arg(desc_cmd_sett, arg_log_file);
    command_line::add_arg(desc_cmd_sett, arg_log_level);
    command_line::add_arg(desc_cmd_sett, arg_console);
    command_line::add_arg(desc_cmd_sett, arg_testnet_on);
    command_line::add_arg(desc_cmd_sett, arg_print_genesis_tx);

    RpcServerConfig::initOptions(desc_cmd_sett);
    CoreConfig::initOptions(desc_cmd_sett);
    NetNodeConfig::initOptions(desc_cmd_sett);
    MinerConfig::initOptions(desc_cmd_sett);

    po::options_description desc_options("Allowed options");
    desc_options.add(desc_cmd_only).add(desc_cmd_sett);

    po::variables_map vm;
    bool r = command_line::handle_error_helper(desc_options, [&]()
    {
      po::store(po::parse_command_line(argc, argv, desc_options), vm);

      if (command_line::get_arg(vm, command_line::arg_help))
      {
        std::cout << CryptoNote::CRYPTONOTE_NAME << " v" << PROJECT_VERSION_LONG << ENDL << ENDL;
        std::cout << desc_options << std::endl;
        return false;
      }

      if (command_line::get_arg(vm, arg_print_genesis_tx)) {
        print_genesis_tx_hex(logger);
        return false;
      }

      std::string data_dir = command_line::get_arg(vm, command_line::arg_data_dir);
      std::string config = command_line::get_arg(vm, arg_config_file);

      boost::filesystem::path data_dir_path(data_dir);
      boost::filesystem::path config_path(config);
      if (!config_path.has_parent_path()) {
        config_path = data_dir_path / config_path;
      }

      boost::system::error_code ec;
      if (boost::filesystem::exists(config_path, ec)) {
        po::store(po::parse_config_file<char>(config_path.string<std::string>().c_str(), desc_cmd_sett), vm);
      }
      po::notify(vm);
      return true;
    });

    if (!r)
      return 1;

    int lev = (command_line::get_arg)(vm, arg_log_level); 
    Level cfgLogLevel = static_cast<Level>(1 + lev);  // Logging::ERROR is level 1

    auto modulePath = Common::NativePathToGeneric(argv[0]);
    auto cfgLogFile = Common::NativePathToGeneric(command_line::get_arg(vm, arg_log_file));

    // don't create a log file unless log level > 0
    if (cfgLogLevel > 1) { // Logging::ERROR is level 1
      if (cfgLogFile.empty()) {
        cfgLogFile = Common::ReplaceExtenstion(modulePath, ".log");
      } else {
        if (!Common::HasParentPath(cfgLogFile)) {
          cfgLogFile = Common::CombinePath(Common::GetPathDirectory(modulePath), cfgLogFile);
        }
      }

      // configure logging
      logManager.configure(buildLoggerConfiguration(cfgLogLevel, cfgLogFile));
    }

    logger(INFO) << CryptoNote::CRYPTONOTE_NAME << " v" << PROJECT_VERSION_LONG;

    if (command_line_preprocessor(vm, logger)) {
      return 0;
    }

    logger(INFO) << "Module folder: " << argv[0];

    bool testnet_mode = command_line::get_arg(vm, arg_testnet_on);
    if (testnet_mode) {
      logger(INFO) << "Starting in testnet mode!";
    }

    //create objects and link them
    CryptoNote::CurrencyBuilder currencyBuilder(logger.getLogger());
    currencyBuilder.testnet(testnet_mode);

    try {
      currencyBuilder.currency();
    } catch (std::exception&) {
      std::cout << "GENESIS_COINBASE_TX_HEX constant has an incorrect value. Please launch: " << CryptoNote::CRYPTONOTE_NAME << "d --" << arg_print_genesis_tx.name;
      return 1;
    }

    CryptoNote::Currency currency = currencyBuilder.currency();
    CryptoNote::core ccore(currency, nullptr, logManager, true);

    CryptoNote::Checkpoints checkpoints(logManager);
    for (const auto& cp : CryptoNote::CHECKPOINTS) {
      checkpoints.add_checkpoint(cp.height, cp.blockId);
    }

    if (!testnet_mode) {
      ccore.set_checkpoints(std::move(checkpoints));
    }

    CoreConfig coreConfig;
    coreConfig.init(vm);
    NetNodeConfig netNodeConfig;
    netNodeConfig.init(vm);
    netNodeConfig.setTestnet(testnet_mode);
    MinerConfig minerConfig;
    minerConfig.init(vm);
    RpcServerConfig rpcConfig;
    rpcConfig.init(vm);

    if (!coreConfig.configFolderDefaulted) {
      if (!Tools::directoryExists(coreConfig.configFolder)) {
        if (!Tools::create_directories_if_necessary(coreConfig.configFolder)) {
          throw std::runtime_error("Can't create directory: " + coreConfig.configFolder);
        }
        //throw std::runtime_error("Directory does not exist: " + coreConfig.configFolder);
      }
    } else {
      if (!Tools::create_directories_if_necessary(coreConfig.configFolder)) {
        throw std::runtime_error("Can't create directory: " + coreConfig.configFolder);
      }
    }

    System::Dispatcher dispatcher;

    CryptoNote::CryptoNoteProtocolHandler cprotocol(currency, dispatcher, ccore, nullptr, logManager);
    CryptoNote::NodeServer p2psrv(dispatcher, cprotocol, logManager);
    CryptoNote::RpcServer rpcServer(dispatcher, logManager, ccore, p2psrv, cprotocol);

    cprotocol.set_p2p_endpoint(&p2psrv);
    ccore.set_cryptonote_protocol(&cprotocol);
    DaemonCommandsHandler dch(ccore, p2psrv, logManager);

    // initialize objects
    logger(INFO) << "Initializing p2p server...";
    if (!p2psrv.init(netNodeConfig)) {
      logger(static_cast<Level>(1), BRIGHT_RED) << "Failed to initialize p2p server.";  // ERROR is 1
      return 1;
    }
    logger(INFO) << "P2p server initialized OK";

    //logger(INFO) << "Initializing core rpc server...";
    //if (!rpc_server.init(vm)) {
    //  logger(ERROR, BRIGHT_RED) << "Failed to initialize core rpc server.";
    //  return 1;
    //}
    // logger(INFO, BRIGHT_GREEN) << "Core rpc server initialized OK on port: " << rpc_server.get_binded_port();

    // initialize core here
    logger(INFO) << "Initializing core...";
    if (!ccore.init(coreConfig, minerConfig, true)) {
      logger(INFO) << "Failed to initialize core"; 
      return 1;
    }
    logger(INFO) << "Core initialized OK";

    // start components
    if (!command_line::has_arg(vm, arg_console)) {
      dch.start_handling();
    }

    if (!rpcConfig.contactInfo.empty()) {
      rpcServer.setContactInfo(rpcConfig.contactInfo);
    }

    logger(INFO) << "Core on address " << rpcConfig.getBindAddress();
    rpcServer.start(rpcConfig.bindIp, rpcConfig.bindPort);
    logger(INFO) << "Core rpc server started ok";

    Tools::SignalHandler::install([&dch, &p2psrv] {
      dch.stop_handling();
      p2psrv.sendStopSignal();
    });

    logger(INFO) << "Starting p2p net loop...";
    p2psrv.run();
    logger(INFO) << "p2p net loop stopped";

    dch.stop_handling();

    //stop components
    logger(INFO) << "Stopping core rpc server...";
    rpcServer.stop();

    //deinitialize components
    logger(INFO) << "Deinitializing core...";
    ccore.deinit();
    logger(INFO) << "Deinitializing p2p...";
    p2psrv.deinit();

    logger(INFO) << "p2p config has been serialized to disk...";

    logger(INFO) << "remove protocol";
    ccore.set_cryptonote_protocol(NULL);
    logger(INFO) << "remove endpoint";
    cprotocol.set_p2p_endpoint(NULL);

    logger(INFO) << "exiting...";

//    exit(0);
    
  } catch (const std::exception& e) {
    logger(INFO) << "Exception: " << e.what();  
    return 1;
  }

  logger(INFO) << "Node stopped.";
  return 0;
}

bool command_line_preprocessor(const boost::program_options::variables_map &vm, LoggerRef &logger) {
  bool exit = false;

  if (command_line::get_arg(vm, command_line::arg_version)) {
    std::cout << CryptoNote::CRYPTONOTE_NAME << " v" << PROJECT_VERSION_LONG << ENDL;
    exit = true;
  }
  //if (command_line::get_arg(vm, arg_os_version)) {
  //  std::cout << "OS: " << Tools::get_os_version_string() << ENDL;
  //  exit = true;
  //}

  if (exit) {
    return true;
  }

  return false;
}
