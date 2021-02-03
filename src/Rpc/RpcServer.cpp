// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "RpcServer.h"
#include "ITransaction.h"
#include "CryptoNoteCore/TransactionPrefixImpl.h"

#include <future>
#include <unordered_map>
#include <iostream>
#include <sstream>

#include "version.h"

#include "Common/Base58.h"

// CryptoNote
#include "Common/StringTools.h"
#include "CryptoNoteCore/Blockchain.h"
#include "CryptoNoteCore/CryptoNoteTools.h"
#include "Common/StringTools.h"
#include "CryptoNoteCore/Core.h"
#include "CryptoNoteCore/IBlock.h"
#include "CryptoNoteCore/Miner.h"
#include "CryptoNoteCore/TransactionExtra.h"

#include "CryptoNoteProtocol/ICryptoNoteProtocolQuery.h"

#include "P2p/NetNode.h"

#include "CoreRpcServerErrorCodes.h"
#include "BlockchainExplorerData.h"
#include "JsonRpc.h"

#undef ERROR

using namespace Logging;
using namespace Crypto;
using namespace Common;

namespace CryptoNote {

namespace {

template <typename Command>
RpcServer::HandlerFunction binMethod(bool (RpcServer::*handler)(typename Command::request const&, typename Command::response&)) {
  return [handler](RpcServer* obj, const HttpRequest& request, HttpResponse& response) {

    boost::value_initialized<typename Command::request> req;
    boost::value_initialized<typename Command::response> res;

    if (!loadFromBinaryKeyValue(static_cast<typename Command::request&>(req), request.getBody())) {
      return false;
    }

    bool result = (obj->*handler)(req, res);
    response.setBody(storeToBinaryKeyValue(res.data()));
    return result;
  };
}

template <typename Command>
RpcServer::HandlerFunction jsonMethod(bool (RpcServer::*handler)(typename Command::request const&, typename Command::response&)) {
  return [handler](RpcServer* obj, const HttpRequest& request, HttpResponse& response) {

    boost::value_initialized<typename Command::request> req;
    boost::value_initialized<typename Command::response> res;

    if (!loadFromJson(static_cast<typename Command::request&>(req), request.getBody())) {
      return false;
    }

    bool result = (obj->*handler)(req, res);
    response.setBody(storeToJson(res.data()));
    return result;
  };
}

}
  
std::unordered_map<std::string, RpcServer::RpcHandler<RpcServer::HandlerFunction>> RpcServer::s_handlers = {
  
  // binary handlers
  { "/getblocks.bin", { binMethod<COMMAND_RPC_GET_BLOCKS_FAST>(&RpcServer::on_get_blocks_bin), false } },
  { "/queryblocks.bin", { binMethod<COMMAND_RPC_QUERY_BLOCKS>(&RpcServer::on_query_blocks), false } },
  { "/queryblockslite.bin", { binMethod<COMMAND_RPC_QUERY_BLOCKS_LITE>(&RpcServer::on_query_blocks_lite), false } },
  { "/get_o_indexes.bin", { binMethod<COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES>(&RpcServer::on_get_indexes), false } },
  { "/getrandom_outs.bin", { binMethod<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS>(&RpcServer::on_get_random_outs), false } },
  { "/get_pool_changes.bin", { binMethod<COMMAND_RPC_GET_POOL_CHANGES>(&RpcServer::onGetPoolChanges), false } },
  { "/get_pool_changes_lite.bin", { binMethod<COMMAND_RPC_GET_POOL_CHANGES_LITE>(&RpcServer::onGetPoolChangesLite), false } },

  { "/get_block_details_by_height", { jsonMethod<COMMAND_RPC_GET_BLOCK_DETAILS_BY_HEIGHT>(&RpcServer::on_get_block_details_by_height), false } },
  { "/get_block_details_by_hash", { jsonMethod<COMMAND_RPC_GET_BLOCK_DETAILS_BY_HASH>(&RpcServer::on_get_block_details_by_hash), false } },
  { "/get_blocks_details_by_heights", { jsonMethod<COMMAND_RPC_GET_BLOCKS_DETAILS_BY_HEIGHTS>(&RpcServer::on_get_blocks_details_by_heights), false } },
  { "/get_blocks_details_by_hashes", { jsonMethod<COMMAND_RPC_GET_BLOCKS_DETAILS_BY_HASHES>(&RpcServer::on_get_blocks_details_by_hashes), false } },
  { "/get_transaction_details_by_hashes", { jsonMethod<COMMAND_RPC_GET_TRANSACTIONS_DETAILS_BY_HASHES>(&RpcServer::on_get_transaction_details_by_hashes), false } },
  { "/get_transaction_details_by_hash", { jsonMethod<COMMAND_RPC_GET_TRANSACTION_DETAILS_BY_HASH>(&RpcServer::on_get_transaction_details_by_hash), false } },
  { "/get_transaction_hashes_by_payment_id", { jsonMethod<COMMAND_RPC_GET_TRANSACTION_HASHES_BY_PAYMENT_ID>(&RpcServer::on_get_transaction_hashes_by_paymentid), false } },

  // json handlers
  { "/getinfo", { jsonMethod<COMMAND_RPC_GET_INFO>(&RpcServer::on_get_info), true } },
  { "/getheight", { jsonMethod<COMMAND_RPC_GET_HEIGHT>(&RpcServer::on_get_height), true } },
  { "/iscoreready", { jsonMethod<COMMAND_RPC_GET_ISCOREREADY>(&RpcServer::on_get_iscoreready), true } },                        

  { "/gettransactionspool", { jsonMethod<COMMAND_RPC_GET_TRANSACTIONS_POOL_SHORT>(&RpcServer::on_get_transactions_pool_short), true } },
  { "/gettransactionsinpool", { jsonMethod<COMMAND_RPC_GET_TRANSACTIONS_POOL>(&RpcServer::on_get_transactions_pool), true } },
  { "/getrawtransactionspool", { jsonMethod<COMMAND_RPC_GET_RAW_TRANSACTIONS_POOL>(&RpcServer::on_get_transactions_pool_raw), true } },

  { "/getblockchainindexes", { jsonMethod<COMMAND_RPC_GET_BLOCK_INDEXES>(&RpcServer::on_get_blockindexes), false } },
  { "/getblock", { jsonMethod<COMMAND_RPC_GET_BLOCK>(&RpcServer::on_get_block), false } },

  { "/start_mining", { jsonMethod<COMMAND_RPC_START_MINING>(&RpcServer::on_start_mining), false } },
  { "/stop_mining", { jsonMethod<COMMAND_RPC_STOP_MINING>(&RpcServer::on_stop_mining), false } },
  { "/stop_daemon", { jsonMethod<COMMAND_RPC_STOP_DAEMON>(&RpcServer::on_stop_daemon), true } },

  { "/getblockcount", { jsonMethod<COMMAND_RPC_GETBLOCKCOUNT>(&RpcServer::on_getblockcount), true } },
  { "/getblockhash", { jsonMethod<COMMAND_RPC_GETBLOCKHASH>(&RpcServer::on_getblockhash), false } },
  { "/getblocktemplate", { jsonMethod<COMMAND_RPC_GETBLOCKTEMPLATE>(&RpcServer::on_getblocktemplate), false } },
  { "/getcurrencyid", { jsonMethod<COMMAND_RPC_GET_CURRENCY_ID>(&RpcServer::on_get_currency_id), true } },
  { "/submitblock", { jsonMethod<COMMAND_RPC_SUBMITBLOCK>(&RpcServer::on_submitblock), false } },
  { "/getlastblockheader", { jsonMethod<COMMAND_RPC_GET_LAST_BLOCK_HEADER>(&RpcServer::on_get_last_block_header), false } },
  { "/getblockheaderbyhash", { jsonMethod<COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH>(&RpcServer::on_get_block_header_by_hash), false } },
  { "/getblockheaderbyheight", { jsonMethod<COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT>(&RpcServer::on_get_block_header_by_height), false } },
  { "/gettransaction", { jsonMethod<COMMAND_RPC_GET_TRANSACTION>(&RpcServer::on_get_transaction), false } },
  { "/gettransactions", { jsonMethod<COMMAND_RPC_GET_TRANSACTIONS>(&RpcServer::on_get_transactions), false } },
  { "/sendrawtransaction", { jsonMethod<COMMAND_RPC_SEND_RAW_TX>(&RpcServer::on_send_raw_tx), false } },

  { "/checktransactionproof", { jsonMethod<COMMAND_RPC_CHECK_TX_PROOF>(&RpcServer::on_check_transaction_proof), false } },
  { "/checktransactionkey", { jsonMethod<COMMAND_RPC_CHECK_TX_KEY>(&RpcServer::on_check_transaction_key), false } },
  { "/checktransactionviewkey", { jsonMethod<COMMAND_RPC_CHECK_TX_WITH_PRIVATE_VIEW_KEY>(&RpcServer::on_check_transaction_view_key), false } },

  // json rpc
  { "/json_rpc", { std::bind(&RpcServer::processJsonRpcRequest, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), true } }
};

RpcServer::RpcServer(System::Dispatcher& dispatcher, Logging::ILogger& log, core& c, NodeServer& p2p, ICryptoNoteProtocolQuery& protocolQuery) : 
  HttpServer(dispatcher, log), logger(log, "RpcServer"), 
  m_core(c), m_p2p(p2p), m_protocolQuery(protocolQuery),
  blockchainExplorerDataBuilder(c, protocolQuery, logger) {
}

void RpcServer::processRequest(const HttpRequest& request, HttpResponse& response) {

  lastUrl = getHostnm(request);

  auto url = request.getUrl();
  logger(INFO) << "processRequest: " << url;

  auto it = s_handlers.find(url);
  if (it == s_handlers.end()) {
    response.setStatus(HttpResponse::STATUS_404);
    return;
  }

  if (!it->second.allowBusyCore && !isCoreReady()) {
    response.setStatus(HttpResponse::STATUS_500);
    response.setBody("Core is busy");
    return;
  }

  it->second.handler(this, request, response);
}

std::string RpcServer::getHostnm(const HttpRequest& request) {
  std::string urll = "";
  auto M = request.getHeaders();
  const std::string delimiter = "#";
    auto logstr = std::accumulate(M.begin(), M.end(), std::string(),
      [delimiter](const std::string& s, const std::pair<const std::string, std::string>& p) {
        return s + (s.empty() ? std::string() : delimiter) + p.first;
      });

  auto search = M.find("host");
  if (search != M.end()) {
    urll = search->second;
  } else {
    urll = "";
  }

  return urll;
}

bool RpcServer::processJsonRpcRequest(const HttpRequest& request, HttpResponse& response) {

  using namespace JsonRpc;

  response.addHeader("Content-Type", "application/json");
  response.addHeader("Access-Control-Allow-Origin", "*");

  JsonRpcRequest jsonRequest;
  JsonRpcResponse jsonResponse;

  try {
    jsonRequest.parseRequest(request.getBody());
    jsonResponse.setId(jsonRequest.getId()); 

    static std::unordered_map<std::string, RpcServer::RpcHandler<JsonMemberMethod>> jsonRpcHandlers = {
      { "getblockchainindexes", { makeMemberMethod(&RpcServer::on_get_blockindexes), false } },
      { "getblock", { makeMemberMethod(&RpcServer::on_get_block), false } },
      { "getblockcount", { makeMemberMethod(&RpcServer::on_getblockcount), true } },
      { "getblockhash", { makeMemberMethod(&RpcServer::on_getblockhash), false } },
      { "getblocktemplate", { makeMemberMethod(&RpcServer::on_getblocktemplate), false } },
      { "getcurrencyid", { makeMemberMethod(&RpcServer::on_get_currency_id), true } },
      { "submitblock", { makeMemberMethod(&RpcServer::on_submitblock), false } },
      { "getlastblockheader", { makeMemberMethod(&RpcServer::on_get_last_block_header), false } },
      { "getblockheaderbyhash", { makeMemberMethod(&RpcServer::on_get_block_header_by_hash), false } },
      { "getblockheaderbyheight", { makeMemberMethod(&RpcServer::on_get_block_header_by_height), false } },
      { "gettransactionspool", { makeMemberMethod(&RpcServer::on_get_transactions_pool_short), false } },
      { "gettransactionsinpool", { makeMemberMethod(&RpcServer::on_get_transactions_pool), false } },
      { "getrawtransactionspool", { makeMemberMethod(&RpcServer::on_get_transactions_pool_raw), false } },
      { "gettransaction", { makeMemberMethod(&RpcServer::on_get_transaction), false } },
      { "gettransactions", { makeMemberMethod(&RpcServer::on_get_transactions), false } },
      { "sendrawtransaction", { makeMemberMethod(&RpcServer::on_send_raw_tx), false } },
      { "stopdaemon", {makeMemberMethod(&RpcServer::on_stop_daemon), true } },
      { "get_block_details_by_height", {makeMemberMethod(&RpcServer::on_get_block_details_by_height), false } },
      { "get_block_details_by_hash", {makeMemberMethod(&RpcServer::on_get_block_details_by_hash), false } },
      { "get_blocks_details_by_heights", {makeMemberMethod(&RpcServer::on_get_blocks_details_by_heights), false } },
      { "get_blocks_details_by_hashes", {makeMemberMethod(&RpcServer::on_get_blocks_details_by_hashes), false } },
      { "get_transaction_details_by_hashes", {makeMemberMethod(&RpcServer::on_get_transaction_details_by_hashes), false } },
      { "get_transaction_details_by_hash", {makeMemberMethod(&RpcServer::on_get_transaction_details_by_hash), false } },
      { "get_transaction_hashes_by_payment_id", {makeMemberMethod(&RpcServer::on_get_transaction_hashes_by_paymentid), false } },

      { "checktransactionproof", {makeMemberMethod(&RpcServer::on_check_transaction_proof), false } },
      { "checktransactionkey", {makeMemberMethod(&RpcServer::on_check_transaction_key), false } },
      { "checktransactionviewkey", {makeMemberMethod(&RpcServer::on_check_transaction_view_key), false } }
    };

    logger(INFO) << "jsonRequst: " << jsonRequest.getMethod();

    auto it = jsonRpcHandlers.find(jsonRequest.getMethod());
    if (it == jsonRpcHandlers.end()) {
      throw JsonRpcError(JsonRpc::errMethodNotFound);
    }

    if (!it->second.allowBusyCore && !isCoreReady()) {
      throw JsonRpcError(CORE_RPC_ERROR_CODE_CORE_BUSY, "Core is busy");
    }

    lastUrl = getHostnm(request); 
    it->second.handler(this, jsonRequest, jsonResponse);

  } catch (const JsonRpcError& err) {
    jsonResponse.setError(err);
  } catch (const std::exception& e) {
    jsonResponse.setError(JsonRpcError(JsonRpc::errInternalError, e.what()));
  }

  response.setBody(jsonResponse.getBody());
  logger(TRACE) << "JSON-RPC response: " << jsonResponse.getBody();
  return true;
}

//jojapoppa, add later for VPN charges, see Karbo code
//bool RpcServer::setFeeAddress(const std::string& fee_address, const AccountPublicAddress& fee_acc) {
//  m_fee_address = fee_address;
//  m_fee_acc = fee_acc;
//  return true;
//}

bool RpcServer::setContactInfo(const std::string& contact) {
  m_contact_info = contact;
  return true;
}

bool RpcServer::isCoreReady() {
  return m_core.currency().isTestnet() || m_p2p.get_payload_object().isSynchronized();
}

bool RpcServer::checkIncomingTransactionForFee(const BinaryArray& tx_blob) {
        Crypto::Hash tx_hash = NULL_HASH;
        Crypto::Hash tx_prefixt_hash = NULL_HASH;
        Transaction tx;
        if (!parseAndValidateTransactionFromBinaryArray(tx_blob, tx, tx_hash, tx_prefixt_hash)) {
                logger(Logging::INFO) << "Could not parse tx from blob";
                return false;
        }

        // always relay fusion transactions
        uint64_t inputs_amount = 0;
        get_inputs_money_amount(tx, inputs_amount);
        uint64_t outputs_amount = get_outs_money_amount(tx);

        const uint64_t fee = inputs_amount - outputs_amount;
        if (fee == 0 && m_core.currency().isFusionTransaction(tx, tx_blob.size())) {
          logger(Logging::DEBUGGING) << "Masternode received fusion transaction, relaying with no fee check";
          return true;
        }

        CryptoNote::TransactionPrefix transaction = *static_cast<const TransactionPrefix*>(&tx);
        std::vector<uint32_t> out;

        // This is for sending fees to a masternode - use this to pay for VPN service later...
        //uint64_t amount=0;
        //jojapoppa, add later to support VPN charges (see Karbo code)
        //if (!CryptoNote::findOutputsToAccount(transaction, m_fee_acc, m_view_key, out, amount)) {
        //  if (amount < m_fee_amout)
        //  {
        //    logger(Logging::INFO) << "Could not find outputs to masternode fee address";
        //    return false;
        //  }
        //}

        if (fee > 0) {
          logger(Logging::INFO) << "Relayed transaction fee: " << m_core.currency().formatAmount(fee) << " FED";
          return true;
        }

        return false;
}

//
// Binary handlers
//

bool RpcServer::on_get_blocks_bin(const COMMAND_RPC_GET_BLOCKS_FAST::request& req, COMMAND_RPC_GET_BLOCKS_FAST::response& res) {
  // TODO code duplication see InProcessNode::doGetNewBlocks()
  if (req.block_ids.empty()) {
    res.status = "Failed";
    return false;
  }

  if (req.block_ids.back() != m_core.getBlockIdByHeight(0)) {
    res.status = "Failed";
    return false;
  }

  uint32_t totalBlockCount;
  uint32_t startBlockIndex;
  std::vector<Crypto::Hash> supplement = m_core.findBlockchainSupplement(req.block_ids, COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT, totalBlockCount, startBlockIndex);

  res.current_height = totalBlockCount;
  res.start_height = startBlockIndex;

  for (const auto& blockId : supplement) {
    assert(m_core.have_block(blockId));
    auto completeBlock = m_core.getBlock(blockId);
    assert(completeBlock != nullptr);

    res.blocks.resize(res.blocks.size() + 1);
    res.blocks.back().block = asString(toBinaryArray(completeBlock->getBlock()));

    res.blocks.back().txs.reserve(completeBlock->getTransactionCount());
    for (size_t i = 0; i < completeBlock->getTransactionCount(); ++i) {
      res.blocks.back().txs.push_back(asString(toBinaryArray(completeBlock->getTransaction(i))));
    }
  }

  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_query_blocks(const COMMAND_RPC_QUERY_BLOCKS::request& req, COMMAND_RPC_QUERY_BLOCKS::response& res) {
  uint32_t startHeight;
  uint32_t currentHeight;
  uint32_t fullOffset;

  if (!m_core.queryBlocks(req.block_ids, req.timestamp, startHeight, currentHeight, fullOffset, res.items)) {
    res.status = "Failed to perform query";
    return false;
  }

  res.start_height = startHeight;
  res.current_height = currentHeight;
  res.full_offset = fullOffset;
  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_query_blocks_lite(const COMMAND_RPC_QUERY_BLOCKS_LITE::request& req, COMMAND_RPC_QUERY_BLOCKS_LITE::response& res) {
  uint32_t startHeight;
  uint32_t currentHeight;
  uint32_t fullOffset;
  if (!m_core.queryBlocksLite(req.blockIds, req.timestamp, startHeight, currentHeight, fullOffset, res.items)) {
    res.status = "Failed to perform query";
    return false;
  }

  res.startHeight = startHeight;
  res.currentHeight = currentHeight;
  res.fullOffset = fullOffset;
  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_get_indexes(const COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request& req, COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response& res) {
  std::vector<uint32_t> outputIndexes;
  if (!m_core.get_tx_outputs_gindexs(req.txid, outputIndexes)) {
    res.status = "Failed";
    return true;
  }

  res.o_indexes.assign(outputIndexes.begin(), outputIndexes.end());
  res.status = CORE_RPC_STATUS_OK;
  logger(TRACE) << "COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES: [" << res.o_indexes.size() << "]";
  return true;
}

bool RpcServer::on_get_random_outs(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res) {
  res.status = "Failed";
  if (!m_core.get_random_outs_for_amounts(req, res)) {
    return true;
  }

  res.status = CORE_RPC_STATUS_OK;

  std::stringstream ss;
  typedef COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount outs_for_amount;
  typedef COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::out_entry out_entry;

  std::for_each(res.outs.begin(), res.outs.end(), [&](outs_for_amount& ofa)  {
    ss << "[" << ofa.amount << "]:";

    assert(ofa.outs.size() && "internal error: ofa.outs.size() is empty");

    std::for_each(ofa.outs.begin(), ofa.outs.end(), [&](out_entry& oe)
    {
      ss << oe.global_amount_index << " ";
    });
    ss << ENDL;
  });
  std::string s = ss.str();
  logger(TRACE) << "COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS: " << ENDL << s;
  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::onGetPoolChanges(const COMMAND_RPC_GET_POOL_CHANGES::request& req, COMMAND_RPC_GET_POOL_CHANGES::response& rsp) {
  rsp.status = CORE_RPC_STATUS_OK;
  std::vector<CryptoNote::Transaction> addedTransactions;
  rsp.isTailBlockActual = m_core.getPoolChanges(req.tailBlockId, req.knownTxsIds, addedTransactions, rsp.deletedTxsIds);
  for (auto& tx : addedTransactions) {
    BinaryArray txBlob;
    if (!toBinaryArray(tx, txBlob)) {
      rsp.status = "Internal error";
      break;;
    }

    rsp.addedTxs.emplace_back(std::move(txBlob));
  }
  return true;
}


bool RpcServer::onGetPoolChangesLite(const COMMAND_RPC_GET_POOL_CHANGES_LITE::request& req, COMMAND_RPC_GET_POOL_CHANGES_LITE::response& rsp) {
  rsp.status = CORE_RPC_STATUS_OK;
  rsp.isTailBlockActual = m_core.getPoolChangesLite(req.tailBlockId, req.knownTxsIds, rsp.addedTxs, rsp.deletedTxsIds);

  return true;
}

bool RpcServer::on_get_blocks_details_by_heights(const COMMAND_RPC_GET_BLOCKS_DETAILS_BY_HEIGHTS::request& req, COMMAND_RPC_GET_BLOCKS_DETAILS_BY_HEIGHTS::response& rsp) {

  try {
    if (req.blockHeights.size() > BLOCK_LIST_MAX_COUNT) {
      throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM,
        std::string("Requested blocks count: ") + std::to_string(req.blockHeights.size()) + " exceeded max limit of "      + std::to_string(BLOCK_LIST_MAX_COUNT) };
    }

    std::vector<BlockDetails> blockDetails;
    for (const uint32_t& height : req.blockHeights) {

      if (m_core.get_current_blockchain_height() <= height) {
        throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_TOO_BIG_HEIGHT,
          std::string("To big height: ") + std::to_string(height) + ", current blockchain height = " + std::to_string     (m_core.get_current_blockchain_height() - 1) };
      }
      Crypto::Hash block_hash = m_core.getBlockIdByHeight(height);
      Block blk;

      if (!m_core.getBlockByHash(block_hash, blk)) {
        throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Internal error: can't get block by height "      + std::to_string(height) + '.' };
      }

      BlockDetails detail;
      if (!blockchainExplorerDataBuilder.fillBlockDetails(blk, detail)) {
        logger(Logging::INFO) << "error filling in block details";
        throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Internal error: can't fill block details."      };
      }

      blockDetails.push_back(detail);
    } 

    rsp.blocks = std::move(blockDetails);
  } 
  catch (std::system_error& e) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, e.what() };
    return false;
  }
  catch (std::exception& e) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Error: " + std::string(e.what()) };
    return false;
  }
  rsp.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_get_blocks_details_by_hashes(const COMMAND_RPC_GET_BLOCKS_DETAILS_BY_HASHES::request& req, COMMAND_RPC_GET_BLOCKS_DETAILS_BY_HASHES::response& rsp) {
  try {
    if (req.blockHashes.size() > BLOCK_LIST_MAX_COUNT) {
      throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM,
        std::string("Requested blocks count: ") + std::to_string(req.blockHashes.size()) + " exceeded max limit of "      + std::to_string(BLOCK_LIST_MAX_COUNT) };
    }
    std::vector<BlockDetails> blockDetails;
    for (const Crypto::Hash& hash : req.blockHashes) {
      Block blk;
      if (!m_core.getBlockByHash(hash, blk)) {
        throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Internal error: can't get block by hash " +      Common::podToHex(hash) + '.' };
      }
      BlockDetails detail;
      if (!blockchainExplorerDataBuilder.fillBlockDetails(blk, detail)) {
        throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Internal error: can't fill block details."      };
      }
      blockDetails.push_back(detail);
    }
    rsp.blocks = std::move(blockDetails);
  }
  catch (std::system_error& e) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, e.what() };
    return false;
  }
  catch (std::exception& e) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Error: " + std::string(e.what()) };
    return false;
  }
  rsp.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_get_block_details_by_height(const COMMAND_RPC_GET_BLOCK_DETAILS_BY_HEIGHT::request& req, COMMAND_RPC_GET_BLOCK_DETAILS_BY_HEIGHT::response& rsp) {
  try {
    BlockDetails blockDetails;
    if (m_core.get_current_blockchain_height() <= req.blockHeight) {
      throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_TOO_BIG_HEIGHT,
        std::string("To big height: ") + std::to_string(req.blockHeight) + ", current blockchain height = " + std::to_string(m_core.get_current_blockchain_height() - 1) };
    }
    Crypto::Hash block_hash = m_core.getBlockIdByHeight(req.blockHeight);
    Block blk;
    if (!m_core.getBlockByHash(block_hash, blk)) {
      throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR,
        "Internal error: can't get block by height " + std::to_string(req.blockHeight) + '.' };
  }
    if (!blockchainExplorerDataBuilder.fillBlockDetails(blk, blockDetails)) {
      throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Internal error: can't fill block details." };
    }
    rsp.block = blockDetails;
  }
  catch (std::system_error& e) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, e.what() };
    return false;
  }
  catch (std::exception& e) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Error: " + std::string(e.what()) };
    return false;
  }
  rsp.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_get_block_details_by_hash(const COMMAND_RPC_GET_BLOCK_DETAILS_BY_HASH::request& req, COMMAND_RPC_GET_BLOCK_DETAILS_BY_HASH::response& rsp) {
  try {
    BlockDetails blockDetails;
    Crypto::Hash block_hash;
    if (!parse_hash256(req.hash, block_hash)) {
      throw JsonRpc::JsonRpcError{
        CORE_RPC_ERROR_CODE_WRONG_PARAM,
        "Failed to parse hex representation of block hash. Hex = " + req.hash + '.' };
    }
    Block blk;
    if (!m_core.getBlockByHash(block_hash, blk)) {
      throw JsonRpc::JsonRpcError{
        CORE_RPC_ERROR_CODE_INTERNAL_ERROR,
        "Internal error: can't get block by hash. Hash = " + req.hash + '.' };
    }
    if (!blockchainExplorerDataBuilder.fillBlockDetails(blk, blockDetails)) {
      throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Internal error: can't fill block details." };
    }
    rsp.block = blockDetails;
  }
  catch (std::system_error& e) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, e.what() };
    return false;
  }
  catch (std::exception& e) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Error: " + std::string(e.what()) };
    return false;
  }
  rsp.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_get_transactions_pool_short(const COMMAND_RPC_GET_TRANSACTIONS_POOL_SHORT::request& req, COMMAND_RPC_GET_TRANSACTIONS_POOL_SHORT::response& res) {
  auto pool = m_core.getPoolTransactionsWithReceiveTime();
  for (const auto txrt : pool) {
	transaction_pool_response transaction_short;
	Transaction tx = txrt.first;
    uint64_t amount_in = getInputAmount(tx);
    uint64_t amount_out = getOutputAmount(tx);

    transaction_short.hash = Common::podToHex(getObjectHash(tx));
    transaction_short.fee = amount_in - amount_out;
    transaction_short.amount_out = amount_out;
    transaction_short.size = getObjectBinarySize(tx);
    transaction_short.receive_time = txrt.second;
    res.transactions.push_back(transaction_short);
  }

  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_get_transactions_pool(const COMMAND_RPC_GET_TRANSACTIONS_POOL::request& req, COMMAND_RPC_GET_TRANSACTIONS_POOL::response& res) {

  auto pool = m_core.getTransactionDetails();
  logger(INFO) << "on_get_transactions_pool size: " << pool.size() << ENDL;
  for (const auto transactionDetails: pool) {
    try {
      PoolTransactionDetailsData datum;
      datum.id = transactionDetails.id;
      datum.tx = transactionDetails.tx;
      datum.blobSize = transactionDetails.blobSize;
      datum.fee = transactionDetails.fee;
      datum.keptByBlock = transactionDetails.keptByBlock;
      datum.receiveTime = transactionDetails.receiveTime;

      res.transactions.push_back(datum);
    }
    catch (std::exception& e) {
      throw JsonRpc::JsonRpcError(CORE_RPC_ERROR_CODE_INTERNAL_ERROR, std::string(e.what()));
      return true;
    }
  }

  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_get_transactions_pool_raw(const COMMAND_RPC_GET_RAW_TRANSACTIONS_POOL::request& req, COMMAND_RPC_GET_RAW_TRANSACTIONS_POOL::response& res) {
  auto pool = m_core.getPoolTransactionsWithReceiveTime();
  for (const auto txrt : pool) {
    try {
      res.transactions.push_back(tx_with_output_global_indexes());
      tx_with_output_global_indexes &e = res.transactions.back();

      e.hash = getObjectHash(txrt.first);
      e.height = boost::value_initialized<uint32_t>();
      e.block_hash = boost::value_initialized<Crypto::Hash>();
      e.timestamp = txrt.second;
      e.transaction = *static_cast<const TransactionPrefix*>(&txrt.first);
      e.fee = getInputAmount(txrt.first) - getOutputAmount(txrt.first);
    }
    catch (std::exception& e) {
      throw JsonRpc::JsonRpcError(CORE_RPC_ERROR_CODE_INTERNAL_ERROR, std::string(e.what()));
      return true;
    }
  }

  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_get_transaction_details_by_hashes(const COMMAND_RPC_GET_TRANSACTIONS_DETAILS_BY_HASHES::request& req, COMMAND_RPC_GET_TRANSACTIONS_DETAILS_BY_HASHES::response& rsp) {
  try {
    std::vector<TransactionDetails> transactionsDetails;
    transactionsDetails.reserve(req.transactionHashes.size());

    std::list<Crypto::Hash> missed_txs;
    std::list<Transaction> txs;
    m_core.getTransactions(req.transactionHashes, txs, missed_txs, true);

    if (!txs.empty()) {
      for (const Transaction& tx : txs) {
        TransactionDetails txDetails;
        if (!blockchainExplorerDataBuilder.fillTransactionDetails(tx, txDetails)) {
          throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR,
            "Internal error: can't fill transaction details." };
        }
        transactionsDetails.push_back(txDetails);
      }

      rsp.transactions = std::move(transactionsDetails);
      rsp.status = CORE_RPC_STATUS_OK;
    }
    if (txs.empty() || !missed_txs.empty()) {
      std::ostringstream ss;
      std::string separator;
      for (auto h : missed_txs) {
        ss << separator << Common::podToHex(h);
        separator = ",";
      }
      rsp.status = "transaction(s) not found: " + ss.str() + ".";
    }
  }
  catch (std::system_error& e) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, e.what() };
    return false;
  }
  catch (std::exception& e) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Error: " + std::string(e.what()) };
    return false;
  }
  return true;
}

bool RpcServer::on_get_transaction_details_by_hash(const COMMAND_RPC_GET_TRANSACTION_DETAILS_BY_HASH::request& req, COMMAND_RPC_GET_TRANSACTION_DETAILS_BY_HASH::response& rsp) {
  try {
    std::list<Crypto::Hash> missed_txs;
    std::list<Transaction> txs;
    std::vector<Crypto::Hash> hashes;
    Crypto::Hash tx_hash;
    if (!parse_hash256(req.hash, tx_hash)) {
      throw JsonRpc::JsonRpcError{
        CORE_RPC_ERROR_CODE_WRONG_PARAM,
        "Failed to parse hex representation of transaction hash. Hex = " + req.hash + '.' };
    }
    hashes.push_back(tx_hash);
    m_core.getTransactions(hashes, txs, missed_txs, true);

    if (txs.empty() || !missed_txs.empty()) {
      std::string hash_str = Common::podToHex(missed_txs.back());
      throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM,
        "transaction wasn't found. Hash = " + hash_str + '.' };
    }

    TransactionDetails transactionsDetails;
    if (!blockchainExplorerDataBuilder.fillTransactionDetails(txs.back(), transactionsDetails)) {
      throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR,
        "Internal error: can't fill transaction details." };
    }
    rsp.transaction = std::move(transactionsDetails);
  }
  catch (std::system_error& e) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, e.what() };
    return false;
  }
  catch (std::exception& e) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Error: " + std::string(e.what()) };
    return false;
  }
  rsp.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_get_transaction_hashes_by_paymentid(const COMMAND_RPC_GET_TRANSACTION_HASHES_BY_PAYMENT_ID::request& req, COMMAND_RPC_GET_TRANSACTION_HASHES_BY_PAYMENT_ID::response& rsp) {
  Crypto::Hash pid_hash;
  if (!parse_hash256(req.paymentId, pid_hash)) {
    throw JsonRpc::JsonRpcError{
      CORE_RPC_ERROR_CODE_WRONG_PARAM,
      "Failed to parse hex representation of payment id. Hex = " + req.paymentId + '.' };
  }
  try {
    rsp.transactionHashes = m_core.getTransactionHashesByPaymentId(pid_hash);
  }
  catch (std::system_error& e) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, e.what() };
    return false;
  }
  catch (std::exception& e) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Error: " + std::string(e.what()) };
    return false;
  }
  rsp.status = CORE_RPC_STATUS_OK;
  return true;
}

//
// JSON handlers
//

namespace {
  uint64_t get_block_reward(const Block& blk) {
    uint64_t reward = 0;
    for (const TransactionOutput& out : blk.baseTransaction.outputs) {
      reward += out.amount;
    }
    return reward;
  }
}

bool RpcServer::on_get_info(const COMMAND_RPC_GET_INFO::request& req, COMMAND_RPC_GET_INFO::response& res) {
  //logger(INFO) << "on_get_info() called";

  res.height = m_core.get_current_blockchain_height();
  res.difficulty = m_core.getNextBlockDifficulty();
  res.tx_count = m_core.get_blockchain_total_transactions() - res.height; //without coinbase
  res.tx_pool_size = m_core.get_pool_transactions_count();
  res.alt_blocks_count = m_core.get_alternative_blocks_count();
  uint64_t total_conn = m_p2p.get_connections_count();
  res.outgoing_connections_count = m_p2p.get_outgoing_connections_count();
  res.incoming_connections_count = total_conn - res.outgoing_connections_count;

  res.rpc_connections_count = get_connections_count();
  res.white_peerlist_size = m_p2p.getPeerlistManager().get_white_peers_count();
  res.grey_peerlist_size = m_p2p.getPeerlistManager().get_gray_peers_count();
  res.last_known_block_index = std::max(static_cast<uint32_t>(1), m_protocolQuery.getObservedHeight()) - 1;
  Crypto::Hash last_block_hash = m_core.getBlockIdByHeight(res.height - 1);
  res.top_block_hash = Common::podToHex(last_block_hash);
  res.version = PROJECT_VERSION_LONG;
  res.contact = m_contact_info.empty() ? std::string() : m_contact_info;
  res.min_fee = m_core.getMinimalFee();
  res.start_time = (uint64_t)m_core.getStartTime();
  
  uint64_t alreadyGeneratedCoins = m_core.getTotalGeneratedAmount();

  size_t medianSize=0;
  m_core.getBlockSize(last_block_hash, medianSize);

  // the totalamt number can overflow a JSON value, so it needs to be passed as a string
  std::ostringstream oss;
  oss << alreadyGeneratedCoins;
  std::string totalSuppl(oss.str()); 
  res.already_generated_coins = totalSuppl;

  res.block_major_version = CryptoNote::BLOCK_MAJOR_VERSION_1; // no forks
  res.max_cumulative_block_size = (uint64_t)m_core.currency().maxBlockCumulativeSize(res.height);

  Block block;
  if (! m_core.getBlockByHash(last_block_hash, block)) {
    res.next_reward = 0; 
  } else {
    res.next_reward = get_block_reward(block);
  }

  if (!m_core.getBlockCumulativeDifficulty(res.height - 1, res.cumulative_difficulty)) {
    throw JsonRpc::JsonRpcError{
      CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Internal error: can't get last cumulative difficulty." };
  }

  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_get_height(const COMMAND_RPC_GET_HEIGHT::request& req, COMMAND_RPC_GET_HEIGHT::response& res) {

  res.height = m_core.get_current_blockchain_height();
  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_get_iscoreready(const COMMAND_RPC_GET_ISCOREREADY::request& req, COMMAND_RPC_GET_ISCOREREADY::response& res) {
  res.iscoreready = isCoreReady();
  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::checkLocal() {

  logger(INFO) << "Checking for local request: " << lastUrl;

  bool localRequest = false;
  if (lastUrl.length() > 0) {
    localRequest = lastUrl.find("127.0.0.1") != std::string::npos;
    if (! localRequest) {
      localRequest = lastUrl.find("localhost") != std::string::npos;
    }
  }

  if (!localRequest) {
    logger(INFO) << "  invalid request from IP other than localhost...";
    return false;
  }

  return true;
}

bool RpcServer::on_get_blockindexes(const COMMAND_RPC_GET_BLOCK_INDEXES::request& req, COMMAND_RPC_GET_BLOCK_INDEXES::response& res) {

  if (!checkLocal()) {
    logger(INFO) << "on_get_blockindexes only runs from localhost";
    return false;
  }

  m_core.print_blockchain_index();
  return true;
}

bool RpcServer::on_get_block(const COMMAND_RPC_GET_BLOCK::request& req, COMMAND_RPC_GET_BLOCK::response& res) {

  std::list<Block> blocks;
  std::list<Transaction> txs;

  // height is 32bit in the network protocol for CN
  if (!m_core.get_blocks((uint32_t)req.height, 1, blocks, txs)) {
    logger(INFO) << "block not found at: " << req.height;
    return false;
  }
  Block block = blocks.front();

  logger(INFO) << "found block at height: " << req.height;
  logger(INFO) << "transaction list size: " << txs.size();

  //if (! m_core.getBlockReward(size_t medianSize, size_t currentBlockSize, uint64_t alreadyGeneratedCoins, uint64_t fee, uint64_t& reward, int64_t& emissionChange)

  res.major_version = block.majorVersion;
  res.minor_version = block.minorVersion;
  res.timestamp = block.timestamp;
  res.prev_hash = Common::podToHex(block.previousBlockHash);
  res.nonce = block.nonce;
  res.reward = get_block_reward(block);

  res.depth = m_core.get_current_blockchain_height() - req.height - 1;
  res.currentmediansize = m_core.getcurrentmediansize();
  res.alreadyGeneratedCoins = m_core.getTotalGeneratedAmount();

  // height is 32bit in the network protocol for CN
  res.hash = m_core.getBlockIdByHeight((uint32_t)req.height);
  size_t size=0;
  m_core.getBlockSize(res.hash, size);
  res.blocksize = size;
  m_core.getBlockDifficulty(static_cast<uint32_t>(req.height), res.difficulty);

  res.transactionHashes = block.transactionHashes;
  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_get_transaction(const COMMAND_RPC_GET_TRANSACTION::request& req, COMMAND_RPC_GET_TRANSACTION::response& res) {

  CryptoNote::Transaction tx;
  Blockchain::TransactionEntry te;

  if (req.blknum > 0 && req.txnum > 0) {
    if (! m_core.transactionByOrdinal(req.blknum, req.txnum, te)) {
      logger(INFO) << "could not find transaction at ordinal positions given";
      return false;
    }
  } else {
    Crypto::Hash tx_hash;
    if (!parse_hash256(req.hash, tx_hash)) {
      throw JsonRpc::JsonRpcError{
        CORE_RPC_ERROR_CODE_WRONG_PARAM,
        "Failed to parse hex representation of transaction hash. Hex = " + req.hash + '.' };
    } else if (! m_core.transactionByHash(tx_hash, te)) {
      logger(INFO) << "could not get transaction with hash:" << tx_hash;
      return false;
    }
  }

  tx = (CryptoNote::Transaction) te.tx; 

  res.global_output_indexes = te.m_global_output_indexes;
  res.hash = getObjectHash(tx);

  Crypto::Hash blockId;
  uint32_t blockH;
  logger(INFO) << "check for block containing tx ";
  if (!m_core.getBlockContainingTx(res.hash, blockId, blockH)) {
    logger(INFO) << "could not locate block for transaction: " << res.hash;
    return false;
  }

  Block block;
  logger(INFO) << "get block hash blockid: " << blockId;
  if (! m_core.getBlockByHash(blockId, block)) {
    logger(INFO) << "could not locate block with blockhash: " << blockId;
    return false;
  }

  res.fee = get_tx_fee(tx);
  res.txsize = getObjectBinarySize(tx);
  res.block = blockId;

  uint32_t blockHeight = 0;
  logger(INFO) << "get block height";
  if (! m_core.getBlockHeight(blockId, blockHeight)) {
    logger(INFO) << "could not determine block height for hash: " << blockId;
    return false;
  }

  logger(INFO) << "check for orphans";

  res.blockheight = blockHeight;
  res.orphan_status = false;
  std::vector<Block> orphanBlocks;
  if (m_core.getOrphanBlocksByHeight(blockHeight, orphanBlocks)) {
    for (Block oblk : orphanBlocks) {
      if (oblk.timestamp == block.timestamp) {
        res.orphan_status = true;
      }
    }
  }

  res.inputamt = getInputAmount(tx);
  res.outputamt = getOutputAmount(tx);

  std::vector<CryptoNote::KeyInput> keyinpt = getInputsKeyObjs(tx);
  for (CryptoNote::KeyInput inp : keyinpt) {
    CryptoNote::keyinput_response rsp;
    rsp.amount = inp.amount;
    rsp.key = Common::podToHex(inp.keyImage);
    res.inputskeyobjs.push_back(rsp);
  }

  std::vector<CryptoNote::MultisignatureInput> mltinpt = getInputsMultisigObjs(tx);
  for (CryptoNote::MultisignatureInput inp : mltinpt) {
    CryptoNote::multisiginput_response rsp;
    rsp.amount = inp.amount;
    rsp.signatureCount = inp.signatureCount;
    rsp.outputIndex = inp.outputIndex;
    res.inputsmultisigobjs.push_back(rsp);
  }

  std::vector<CryptoNote::KeyOutput> keyoutpt = getOutputsKeyObjs(tx);
  for (CryptoNote::KeyOutput out : keyoutpt) {
    CryptoNote::keyoutput_response rsp;
    rsp.keyOutput = Common::podToHex(out.key);
    res.outputskeyobjs.push_back(rsp);
  }

  std::vector<CryptoNote::MultisignatureOutput> mltoutpt = getOutputsMultisigObjs(tx);
  for (CryptoNote::MultisignatureOutput out : mltoutpt) {
    CryptoNote::multisigoutput_response rsp;
    rsp.requiredSignatureCount = out.requiredSignatureCount;

    for (Crypto::PublicKey keyy : out.keys) {
      rsp.keys.push_back(Common::podToHex(keyy));
    }

    res.outputsmultisigobjs.push_back(rsp);
  }

  res.inputsamts = getInputsAmounts(tx);
  res.outputsamts = getOutputsAmounts(tx);
  res.outputskeys = getOutputsKeys(tx);

  res.prefix = getObjectHash(*static_cast<const TransactionPrefix*>(&tx));
  res.unlocktime = tx.unlockTime;
  res.version = tx.version;

  logger(INFO) << "get payment Id from extra";

  Crypto::Hash paymentID;
  if (! getPaymentIdFromTxExtra(tx.extra, paymentID)) {
    logger(INFO) << "No paymentID found for transaction";
  } 

  logger(INFO) << "done";

  res.extra = Common::toHex(tx.extra);
  res.paymentid = paymentID;
  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_get_transactions(const COMMAND_RPC_GET_TRANSACTIONS::request& req, COMMAND_RPC_GET_TRANSACTIONS::response& res) {

  if (!checkLocal()) {
    logger(INFO) << "on_get_transacdtions only runs from localhost";
    return false;
  }

  std::vector<Hash> vh;
  for (const auto& tx_hex_str : req.txs_hashes) {
    BinaryArray b;
    if (!fromHex(tx_hex_str, b)) {
      res.status = "Failed to parse hex representation of transaction hash";
      return true;
    }

    if (b.size() != sizeof(Hash)) {
      res.status = "Failed, size of data mismatch";
    }

    vh.push_back(*reinterpret_cast<const Hash*>(b.data()));
  }

  std::list<Crypto::Hash> missed_txs;
  std::list<Transaction> txs;
  m_core.getTransactions(vh, txs, missed_txs);

  if (!txs.empty()) {
    for (const Transaction& tx : txs) {
      res.txs_as_hex.push_back(Common::podToHex(tx));
    }
  }

  if (!missed_txs.empty()) {
    for (const auto& miss_tx : missed_txs) {
      res.missed_txs.push_back(Common::podToHex(miss_tx));
    }
  }

  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_send_raw_tx(const COMMAND_RPC_SEND_RAW_TX::request& req, COMMAND_RPC_SEND_RAW_TX::response& res) {
  BinaryArray tx_blob;
  if (!fromHex(req.tx_as_hex, tx_blob))
  {
    logger(INFO) << "[on_send_raw_tx]: Failed to parse tx from hexbuff: " << req.tx_as_hex;
    res.status = "Failed";
    return true;
  }

  Crypto::Hash transactionHash = Crypto::cn_fast_hash(tx_blob.data(), tx_blob.size());
  logger(Logging::DEBUGGING) << "transaction " << transactionHash << " came in on_send_raw_tx";

  tx_verification_context tvc = boost::value_initialized<tx_verification_context>();
  if (!m_core.handle_incoming_tx(tx_blob, tvc, false))
  {
    logger(INFO) << "[on_send_raw_tx]: Failed to process tx";
    res.status = "Failed";
    return true;
  }

  if (tvc.m_verifivation_failed)
  {
    logger(INFO) << "[on_send_raw_tx]: tx verification failed";
    res.status = "Failed";
    return true;
  }

  if (!tvc.m_should_be_relayed)
  {
    logger(INFO) << "[on_send_raw_tx]: tx accepted, but not relayed";
    res.status = "Not relayed";
    return true;
  }

  if (!checkIncomingTransactionForFee(tx_blob)) {
    logger(Logging::INFO) << "Transaction not relayed due to lack of node fee";
    res.status = "Not relayed due to lack of node fee";
    return true;
  }

  NOTIFY_NEW_TRANSACTIONS::request r;
  r.txs.push_back(asString(tx_blob));
  m_core.get_protocol()->relay_transactions(r);
  //std::cout << "called m_core.get_procol()->relay_transaction(r)\n";
  //TODO: make sure that tx has reached other nodes here, probably wait to receive reflections from other nodes
  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_start_mining(const COMMAND_RPC_START_MINING::request& req, COMMAND_RPC_START_MINING::response& res) {
  AccountPublicAddress adr;
  if (!m_core.currency().parseAccountAddressString(req.miner_address, adr)) {
    res.status = "Failed, wrong address";
    return true;
  }

  if (!m_core.get_miner().start(adr, static_cast<size_t>(req.threads_count))) {
    res.status = "Failed, mining not started";
    return true;
  }

  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_stop_mining(const COMMAND_RPC_STOP_MINING::request& req, COMMAND_RPC_STOP_MINING::response& res) {
  if (!m_core.get_miner().stop()) {
    res.status = "Failed, mining not stopped";
    return true;
  }
  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_stop_daemon(const COMMAND_RPC_STOP_DAEMON::request& req, COMMAND_RPC_STOP_DAEMON::response& res) {

  if (checkLocal()) {
    logger(INFO) << "Sending stop signal...";
    m_core.stop();
    m_p2p.sendStopSignal();
    res.status = CORE_RPC_STATUS_OK;
  } else {
    res.status = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------------------------------------------------------
// JSON RPC methods
//------------------------------------------------------------------------------------------------------------------------------
bool RpcServer::on_getblockcount(const COMMAND_RPC_GETBLOCKCOUNT::request& req, COMMAND_RPC_GETBLOCKCOUNT::response& res) {
  res.count = m_core.get_current_blockchain_height();
  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_getblockhash(const COMMAND_RPC_GETBLOCKHASH::request& req, COMMAND_RPC_GETBLOCKHASH::response& res) {
  if (m_core.get_current_blockchain_height() <= req.height) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_TOO_BIG_HEIGHT,
      std::string("To big height: ") + std::to_string(req.height) + ", current blockchain height = " + std::to_string(m_core.get_current_blockchain_height()) };
  }

  Hash blockId = m_core.getBlockIdByHeight(req.height);
  if (blockId == NULL_HASH) {
    throw JsonRpc::JsonRpcError{ 
      CORE_RPC_ERROR_CODE_TOO_BIG_HEIGHT,
      std::string("To big height: ") + std::to_string(req.height) + ", current blockchain height = " + std::to_string(m_core.get_current_blockchain_height())
    };
  }

  res.hash = Common::podToHex(blockId);
  return true;
}

namespace {
  uint64_t slow_memmem(void* start_buff, size_t buflen, void* pat, size_t patlen)
  {
    void* buf = start_buff;
    void* end = (char*)buf + buflen - patlen;
    while ((buf = memchr(buf, ((char*)pat)[0], buflen)))
    {
      if (buf>end)
        return 0;
      if (memcmp(buf, pat, patlen) == 0)
        return (char*)buf - (char*)start_buff;
      buf = (char*)buf + 1;
    }
    return 0;
  }
}

bool RpcServer::on_getblocktemplate(const COMMAND_RPC_GETBLOCKTEMPLATE::request& req, COMMAND_RPC_GETBLOCKTEMPLATE::response& res) {
  if (req.reserve_size > TX_EXTRA_NONCE_MAX_COUNT) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_TOO_BIG_RESERVE_SIZE, "To big reserved size, maximum 255" };
  }

  AccountPublicAddress acc = boost::value_initialized<AccountPublicAddress>();

  if (!req.wallet_address.size() || !m_core.currency().parseAccountAddressString(req.wallet_address, acc)) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_WALLET_ADDRESS, "Failed to parse wallet address" };
  }

  Block b = boost::value_initialized<Block>();
  CryptoNote::BinaryArray blob_reserve;
  blob_reserve.resize(req.reserve_size, 0);
  if (!m_core.get_block_template(b, acc, res.difficulty, res.height, blob_reserve)) {
    logger(ERROR) << "Failed to create block template";
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Internal error: failed to create block template" };
  }

  BinaryArray block_blob = toBinaryArray(b);
  PublicKey tx_pub_key = CryptoNote::getTransactionPublicKeyFromExtra(b.baseTransaction.extra);
  if (tx_pub_key == NULL_PUBLIC_KEY) {
    logger(ERROR) << "Failed to find tx pub key in coinbase extra";
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Internal error: failed to find tx pub key in coinbase extra" };
  }

  if (0 < req.reserve_size) {
    res.reserved_offset = slow_memmem((void*)block_blob.data(), block_blob.size(), &tx_pub_key, sizeof(tx_pub_key));
    if (!res.reserved_offset) {
      logger(ERROR) << "Failed to find tx pub key in blockblob";
      throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Internal error: failed to create block template" };
    }
    res.reserved_offset += sizeof(tx_pub_key) + 3; //3 bytes: tag for TX_EXTRA_TAG_PUBKEY(1 byte), tag for TX_EXTRA_NONCE(1 byte), counter in TX_EXTRA_NONCE(1 byte)
    if (res.reserved_offset + req.reserve_size > block_blob.size()) {
      logger(ERROR) << "Failed to calculate offset for reserved bytes";
      throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Internal error: failed to create block template" };
    }
  } else {
    res.reserved_offset = 0;
  }

  res.blocktemplate_blob = toHex(block_blob);
  res.status = CORE_RPC_STATUS_OK;

  return true;
}

bool RpcServer::on_get_currency_id(const COMMAND_RPC_GET_CURRENCY_ID::request& /*req*/, COMMAND_RPC_GET_CURRENCY_ID::response& res) {
  Hash currencyId = m_core.currency().genesisBlockHash();
  res.currency_id_blob = Common::podToHex(currencyId);
  return true;
}

bool RpcServer::on_submitblock(const COMMAND_RPC_SUBMITBLOCK::request& req, COMMAND_RPC_SUBMITBLOCK::response& res) {
  if (req.size() != 1) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM, "Wrong param" };
  }

  BinaryArray blockblob;
  if (!fromHex(req[0], blockblob)) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_BLOCKBLOB, "Wrong block blob" };
  }

  block_verification_context bvc = boost::value_initialized<block_verification_context>();

  m_core.handle_incoming_block_blob(blockblob, bvc, true, true);

  if (!bvc.m_added_to_main_chain) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_BLOCK_NOT_ACCEPTED, "Block not accepted" };
  }

  res.status = CORE_RPC_STATUS_OK;
  return true;
}

void RpcServer::fill_block_header_response(const Block& blk, bool orphan_status, uint64_t height, const Hash& hash, block_header_response& responce) {
  responce.major_version = blk.majorVersion;
  responce.minor_version = blk.minorVersion;
  responce.timestamp = blk.timestamp;
  responce.prev_hash = Common::podToHex(blk.previousBlockHash);
  responce.nonce = blk.nonce;
  responce.orphan_status = orphan_status;
  responce.height = height;
  responce.depth = m_core.get_current_blockchain_height() - height - 1;
  responce.hash = Common::podToHex(hash);
  m_core.getBlockDifficulty(static_cast<uint32_t>(height), responce.difficulty);
  responce.reward = get_block_reward(blk);
}

bool RpcServer::on_get_last_block_header(const COMMAND_RPC_GET_LAST_BLOCK_HEADER::request& req, COMMAND_RPC_GET_LAST_BLOCK_HEADER::response& res) {

  uint32_t last_block_height;
  Hash last_block_hash;
  m_core.get_blockchain_top(last_block_height, last_block_hash);

  Block last_block;
  if (!m_core.getBlockByHash(last_block_hash, last_block)) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Internal error: can't get last block hash." };
  }
  
  fill_block_header_response(last_block, false, last_block_height, last_block_hash, res.block_header);
  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_get_block_header_by_hash(const COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH::request& req, COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH::response& res) {
  Hash block_hash;

  if (!parse_hash256(req.hash, block_hash)) {
    throw JsonRpc::JsonRpcError{
      CORE_RPC_ERROR_CODE_WRONG_PARAM,
      "Failed to parse hex representation of block hash. Hex = " + req.hash + '.' };
  }

  Block blk;
  if (!m_core.getBlockByHash(block_hash, blk)) {
    throw JsonRpc::JsonRpcError{
      CORE_RPC_ERROR_CODE_INTERNAL_ERROR,
      "Internal error: can't get block by hash. Hash = " + req.hash + '.' };
  }

  if (blk.baseTransaction.inputs.front().type() != typeid(BaseInput)) {
    throw JsonRpc::JsonRpcError{
      CORE_RPC_ERROR_CODE_INTERNAL_ERROR,
      "Internal error: coinbase transaction in the block has the wrong type" };
  }

  uint64_t block_height = boost::get<BaseInput>(blk.baseTransaction.inputs.front()).blockIndex;
  fill_block_header_response(blk, false, block_height, block_hash, res.block_header);
  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_get_block_header_by_height(const COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT::request& req, COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT::response& res) {
  if (m_core.get_current_blockchain_height() <= req.height) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_TOO_BIG_HEIGHT,
      std::string("To big height: ") + std::to_string(req.height) + ", current blockchain height = " + std::to_string(m_core.get_current_blockchain_height()) };
  }

  Hash block_hash = m_core.getBlockIdByHeight(static_cast<uint32_t>(req.height));
  Block blk;
  if (!m_core.getBlockByHash(block_hash, blk)) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR,
      "Internal error: can't get block by height. Height = " + std::to_string(req.height) + '.' };
  }

  fill_block_header_response(blk, false, req.height, block_hash, res.block_header);
  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_check_transaction_proof(const COMMAND_RPC_CHECK_TX_PROOF::request& req, COMMAND_RPC_CHECK_TX_PROOF::response& res) {

  // parse txid
  Crypto::Hash txid;
  if (!parse_hash256(req.transaction_id, txid)) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM, "Failed to parse txid" };
  }
  // parse address
  CryptoNote::AccountPublicAddress address;
  if (!m_core.currency().parseAccountAddressString(req.destination_address, address)) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM, "Failed to parse address " + req.destination_address + '.' };
  }
  // parse pubkey r*A & signature
  std::string decoded_data;
  uint64_t prefix;
  if (!Tools::Base58::decode_addr(req.signature, prefix, decoded_data) || prefix != CryptoNote::parameters::CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM, "Transaction proof decoding error" };
  }
  Crypto::PublicKey rA;
  Crypto::Signature sig;
  
  std::string rA_decoded = decoded_data.substr(0, sizeof(Crypto::PublicKey));
  std::string sig_decoded = decoded_data.substr(sizeof(Crypto::PublicKey), sizeof(Crypto::Signature));

  memcpy(&rA, rA_decoded.data(), sizeof(Crypto::PublicKey));
  memcpy(&sig, sig_decoded.data(), sizeof(Crypto::Signature));

  // fetch tx pubkey
  Transaction tx;

  std::vector<uint32_t> out;
  std::vector<Crypto::Hash> tx_ids;
  tx_ids.push_back(txid);

  std::list<Crypto::Hash> missed_txs; 
  std::list<Transaction> txs;

  m_core.getTransactions(tx_ids, txs, missed_txs);
  tx = txs.front();

  CryptoNote::TransactionPrefix transaction = *static_cast<const TransactionPrefix*>(&tx);

  Crypto::PublicKey R = getTransactionPublicKeyFromExtra(transaction.extra);
  if (R == NULL_PUBLIC_KEY)
  {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Tx pubkey was not found" };
  }

  // check signature
  bool r = Crypto::check_tx_proof(txid, R, address.viewPublicKey, rA, sig);
  res.signature_valid = r;

  if (r) {

    // obtain key derivation by multiplying scalar 1 to the pubkey r*A included in the signature
    Crypto::KeyDerivation derivation;
    if (!Crypto::generate_key_derivation(rA, Crypto::EllipticCurveScalar2SecretKey(Crypto::I), derivation)) {
      throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Failed to generate key derivation" };
    }

    // look for outputs
    uint64_t received(0);
    size_t keyIndex(0);
    std::vector<TransactionOutput> outputs;
    try {
      for (const TransactionOutput& o : transaction.outputs) {
        if (o.target.type() == typeid(KeyOutput)) {
          const KeyOutput out_key = boost::get<KeyOutput>(o.target);
          Crypto::PublicKey pubkey;
          derive_public_key(derivation, keyIndex, address.spendPublicKey, pubkey);
          if (pubkey == out_key.key) {
            received += o.amount;
            outputs.push_back(o);
          }
        }
        ++keyIndex;
      }
    }
    catch (...)
    {
      throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Unknown error" };
    }
    res.received_amount = received;
    res.outputs = outputs;

    CryptoNote::tx_memory_pool::PoolTransactionDetails transactionDetails = m_core.getTransactionDetails(txid);
    res.confirmations = m_protocolQuery.getObservedHeight() - transactionDetails.maxUsedBlock.height;
  }
  else {
    res.received_amount = 0;
  }

  res.status = CORE_RPC_STATUS_OK;

  return true;
}

bool RpcServer::on_check_transaction_key(const COMMAND_RPC_CHECK_TX_KEY::request& req, COMMAND_RPC_CHECK_TX_KEY::response& res) {

  // parse txid
  Crypto::Hash txid;
  if (!parse_hash256(req.transaction_id, txid)) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM, "Failed to parse txid" };
  }
  // parse address
  CryptoNote::AccountPublicAddress address;
  if (!m_core.currency().parseAccountAddressString(req.address, address)) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM, "Failed to parse address " + req.address + '.' };
  }
  // parse txkey
  Crypto::Hash tx_key_hash;
  size_t size;
  if (!Common::fromHex(req.transaction_key, &tx_key_hash, sizeof(tx_key_hash), size) || size != sizeof(tx_key_hash)) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM, "Failed to parse txkey" };
  }
  Crypto::SecretKey tx_key = *(struct Crypto::SecretKey *) &tx_key_hash;

  // fetch tx
  Transaction tx;
  std::vector<Crypto::Hash> tx_ids;
  tx_ids.push_back(txid);

  std::list<Crypto::Hash> missed_txs;
  std::list<Transaction> txs;

  m_core.getTransactions(tx_ids, txs, missed_txs);
  tx = txs.front();

  CryptoNote::TransactionPrefix transaction = *static_cast<const TransactionPrefix*>(&tx);

  // obtain key derivation
  Crypto::KeyDerivation derivation;
  if (!Crypto::generate_key_derivation(address.viewPublicKey, tx_key, derivation))
  {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM, "Failed to generate key derivation from supplied parameters" };
  }

  // look for outputs
  uint64_t received(0);
  size_t keyIndex(0);
  std::vector<TransactionOutput> outputs;
  try {
    for (const TransactionOutput& o : transaction.outputs) {
      if (o.target.type() == typeid(KeyOutput)) {
        const KeyOutput out_key = boost::get<KeyOutput>(o.target);
        Crypto::PublicKey pubkey;
        derive_public_key(derivation, keyIndex, address.spendPublicKey, pubkey);
        if (pubkey == out_key.key) {
          received += o.amount;
          outputs.push_back(o);
        }
      }
      ++keyIndex;
    }
  }
  catch (...)
  {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Unknown error" };
  }
  res.amount = received;
  res.outputs = outputs;
  res.status = CORE_RPC_STATUS_OK;

  return true;
}

bool RpcServer::on_check_transaction_view_key(const COMMAND_RPC_CHECK_TX_WITH_PRIVATE_VIEW_KEY::request& req, COMMAND_RPC_CHECK_TX_WITH_PRIVATE_VIEW_KEY::response& res) {

  // parse txid
  Crypto::Hash txid;
  if (!parse_hash256(req.transaction_id, txid)) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM, "Failed to parse txid" };
  }
  // parse address
  CryptoNote::AccountPublicAddress address;
  if (!m_core.currency().parseAccountAddressString(req.address, address)) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM, "Failed to parse address " + req.address + '.' };
  }
  // parse view key
  Crypto::Hash view_key_hash;
  size_t size;
  if (!Common::fromHex(req.view_key, &view_key_hash, sizeof(view_key_hash), size) || size != sizeof(view_key_hash)) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM, "Failed to parse private view key" };
  }
  Crypto::SecretKey viewKey = *(struct Crypto::SecretKey *) &view_key_hash;

  // fetch tx
  Transaction tx;
  std::vector<Crypto::Hash> tx_ids;
  tx_ids.push_back(txid);

  std::list<Crypto::Hash> missed_txs;
  std::list<Transaction> txs;

  m_core.getTransactions(tx_ids, txs, missed_txs);
  tx = txs.front();

  CryptoNote::TransactionPrefix transaction = *static_cast<const TransactionPrefix*>(&tx);

  // get tx pub key
  Crypto::PublicKey txPubKey = getTransactionPublicKeyFromExtra(transaction.extra);

  // obtain key derivation
  Crypto::KeyDerivation derivation;
  if (!Crypto::generate_key_derivation(txPubKey, viewKey, derivation))
  {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM, "Failed to generate key derivation from supplied parameters" };
  }

  // look for outputs
  uint64_t received(0);
  size_t keyIndex(0);
  std::vector<TransactionOutput> outputs;
  try {
    for (const TransactionOutput& o : transaction.outputs) {
      if (o.target.type() == typeid(KeyOutput)) {
        const KeyOutput out_key = boost::get<KeyOutput>(o.target);
        Crypto::PublicKey pubkey;
        derive_public_key(derivation, keyIndex, address.spendPublicKey, pubkey);
        if (pubkey == out_key.key) {
          received += o.amount;
          outputs.push_back(o);
        }
      }
      ++keyIndex;
    }
  }
  catch (...)
  {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Unknown error" };
  }
  res.amount = received;
  res.outputs = outputs;
  res.status = CORE_RPC_STATUS_OK;

  return true;
}

}
