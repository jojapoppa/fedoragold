// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "HttpServer.h"

#include <functional>
#include <unordered_map>

#include <Logging/LoggerRef.h>
#include "CoreRpcServerCommandsDefinitions.h"
#include "BlockchainExplorer/BlockchainExplorerDataBuilder.h"

const uint32_t MAX_NUMBER_OF_BLOCKS_PER_STATS_REQUEST = 10000;
const uint64_t BLOCK_LIST_MAX_COUNT = 1000;

namespace CryptoNote {

class core;
class NodeServer;
class ICryptoNoteProtocolQuery;

class RpcServer : public HttpServer {
public:
  RpcServer(System::Dispatcher& dispatcher, Logging::ILogger& log, core& c, NodeServer& p2p, ICryptoNoteProtocolQuery& protocolQuery);

  typedef std::function<bool(RpcServer*, const HttpRequest& request, HttpResponse& response)> HandlerFunction;

  bool setContactInfo(const std::string& contact);

private:

  std::string lastUrl = "";
  std::string getHostnm(const HttpRequest& request);
  bool checkLocal();

  template <class Handler>
  struct RpcHandler {
    const Handler handler;
    const bool allowBusyCore;
  };

  typedef void (RpcServer::*HandlerPtr)(const HttpRequest& request, HttpResponse& response);
  static std::unordered_map<std::string, RpcHandler<HandlerFunction>> s_handlers;

  virtual void processRequest(const HttpRequest& request, HttpResponse& response) override;
  bool processJsonRpcRequest(const HttpRequest& request, HttpResponse& response);

  //jojapoppa, add later to support VPN charges, see Karbo code
  bool setFeeAddress(const std::string& fee_address, const AccountPublicAddress& fee_acc);

  bool isCoreReady();
  bool checkIncomingTransactionForFee(const BinaryArray& tx_blob);

  // binary handlers
  bool on_get_blocks_bin(const COMMAND_RPC_GET_BLOCKS_FAST::request& req, COMMAND_RPC_GET_BLOCKS_FAST::response& res);
  bool on_query_blocks(const COMMAND_RPC_QUERY_BLOCKS::request& req, COMMAND_RPC_QUERY_BLOCKS::response& res);
  bool on_query_blocks_lite(const COMMAND_RPC_QUERY_BLOCKS_LITE::request& req, COMMAND_RPC_QUERY_BLOCKS_LITE::response& res);
  bool on_get_indexes(const COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request& req, COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response& res);
  bool on_get_random_outs(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res);
  bool onGetPoolChanges(const COMMAND_RPC_GET_POOL_CHANGES::request& req, COMMAND_RPC_GET_POOL_CHANGES::response& rsp);
  bool onGetPoolChangesLite(const COMMAND_RPC_GET_POOL_CHANGES_LITE::request& req, COMMAND_RPC_GET_POOL_CHANGES_LITE::response& rsp);

  // json handlers
  bool on_get_info(const COMMAND_RPC_GET_INFO::request& req, COMMAND_RPC_GET_INFO::response& res);
  bool on_get_height(const COMMAND_RPC_GET_HEIGHT::request& req, COMMAND_RPC_GET_HEIGHT::response& res);
  bool on_get_iscoreready(const COMMAND_RPC_GET_ISCOREREADY::request& req, COMMAND_RPC_GET_ISCOREREADY::response& res);
  bool on_get_transaction(const COMMAND_RPC_GET_TRANSACTION::request& req, COMMAND_RPC_GET_TRANSACTION::response& res);
  bool on_get_transactions(const COMMAND_RPC_GET_TRANSACTIONS::request& req, COMMAND_RPC_GET_TRANSACTIONS::response& res);
  bool on_send_raw_tx(const COMMAND_RPC_SEND_RAW_TX::request& req, COMMAND_RPC_SEND_RAW_TX::response& res);
  bool on_start_mining(const COMMAND_RPC_START_MINING::request& req, COMMAND_RPC_START_MINING::response& res);
  bool on_stop_mining(const COMMAND_RPC_STOP_MINING::request& req, COMMAND_RPC_STOP_MINING::response& res);
  bool on_stop_daemon(const COMMAND_RPC_STOP_DAEMON::request& req, COMMAND_RPC_STOP_DAEMON::response& res);

  bool on_get_transactions_pool_short(const COMMAND_RPC_GET_TRANSACTIONS_POOL_SHORT::request& req, COMMAND_RPC_GET_TRANSACTIONS_POOL_SHORT::response& res);
  bool on_get_transactions_pool_raw(const COMMAND_RPC_GET_RAW_TRANSACTIONS_POOL::request& req, COMMAND_RPC_GET_RAW_TRANSACTIONS_POOL::response& res);
  bool on_get_transactions_pool(const COMMAND_RPC_GET_TRANSACTIONS_POOL::request& req, COMMAND_RPC_GET_TRANSACTIONS_POOL::response& res);

  bool on_get_blockindexes(const COMMAND_RPC_GET_BLOCK_INDEXES::request& req, COMMAND_RPC_GET_BLOCK_INDEXES::response& res);
  bool on_get_block(const COMMAND_RPC_GET_BLOCK::request& req, COMMAND_RPC_GET_BLOCK::response& res);

  bool on_get_blocks_details_by_heights(const COMMAND_RPC_GET_BLOCKS_DETAILS_BY_HEIGHTS::request& req, COMMAND_RPC_GET_BLOCKS_DETAILS_BY_HEIGHTS::response& rsp);
  bool on_get_blocks_details_by_hashes(const COMMAND_RPC_GET_BLOCKS_DETAILS_BY_HASHES::request& req, COMMAND_RPC_GET_BLOCKS_DETAILS_BY_HASHES::response& rsp);
  bool on_get_block_details_by_height(const COMMAND_RPC_GET_BLOCK_DETAILS_BY_HEIGHT::request& req, COMMAND_RPC_GET_BLOCK_DETAILS_BY_HEIGHT::response& rsp);
  bool on_get_block_details_by_hash(const COMMAND_RPC_GET_BLOCK_DETAILS_BY_HASH::request& req, COMMAND_RPC_GET_BLOCK_DETAILS_BY_HASH::response& rsp);
  bool on_get_transaction_details_by_hashes(const COMMAND_RPC_GET_TRANSACTIONS_DETAILS_BY_HASHES::request& req, COMMAND_RPC_GET_TRANSACTIONS_DETAILS_BY_HASHES::response& rsp);
  bool on_get_transaction_details_by_hash(const COMMAND_RPC_GET_TRANSACTION_DETAILS_BY_HASH::request& req, COMMAND_RPC_GET_TRANSACTION_DETAILS_BY_HASH::response& rsp);
  bool on_get_transaction_hashes_by_paymentid(const COMMAND_RPC_GET_TRANSACTION_HASHES_BY_PAYMENT_ID::request& req, COMMAND_RPC_GET_TRANSACTION_HASHES_BY_PAYMENT_ID::response& rsp);

  // json rpc
  bool on_getblockcount(const COMMAND_RPC_GETBLOCKCOUNT::request& req, COMMAND_RPC_GETBLOCKCOUNT::response& res);
  bool on_getblockhash(const COMMAND_RPC_GETBLOCKHASH::request& req, COMMAND_RPC_GETBLOCKHASH::response& res);
  bool on_getblocktemplate(const COMMAND_RPC_GETBLOCKTEMPLATE::request& req, COMMAND_RPC_GETBLOCKTEMPLATE::response& res);
  bool on_get_currency_id(const COMMAND_RPC_GET_CURRENCY_ID::request& req, COMMAND_RPC_GET_CURRENCY_ID::response& res);
  bool on_submitblock(const COMMAND_RPC_SUBMITBLOCK::request& req, COMMAND_RPC_SUBMITBLOCK::response& res);
  bool on_get_last_block_header(const COMMAND_RPC_GET_LAST_BLOCK_HEADER::request& req, COMMAND_RPC_GET_LAST_BLOCK_HEADER::response& res);
  bool on_get_block_header_by_hash(const COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH::request& req, COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH::response& res);
  bool on_get_block_header_by_height(const COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT::request& req, COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT::response& res);

  void fill_block_header_response(const Block& blk, bool orphan_status, uint64_t height, const Crypto::Hash& hash, block_header_response& responce);

  Logging::LoggerRef logger;
  core& m_core;
  NodeServer& m_p2p;
  BlockchainExplorerDataBuilder blockchainExplorerDataBuilder;
  const ICryptoNoteProtocolQuery& m_protocolQuery;

  //jojapoppa, add later to support VPN charges, see Karbo code
  //std::string m_fee_address;
  //CryptoNote::AccountPublicAddress m_fee_acc;
  
  std::string m_contact_info; 
};

}
