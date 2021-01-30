// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "CryptoNoteProtocol/CryptoNoteProtocolDefinitions.h"
#include "CryptoNoteCore/CryptoNoteBasic.h"
#include "CryptoNoteCore/Difficulty.h"
#include "CryptoNoteCore/TransactionPool.h"
#include "crypto/hash.h"

#include "Serialization/SerializationOverloads.h"
#include "Serialization/BlockchainExplorerDataSerialization.h"

namespace CryptoNote {
//-----------------------------------------------
#define CORE_RPC_STATUS_OK "OK"
#define CORE_RPC_STATUS_BUSY "BUSY"

struct EMPTY_STRUCT {
  void serialize(ISerializer &s) {}
};

struct STATUS_STRUCT {
  std::string status;

  void serialize(ISerializer &s) {
    KV_MEMBER(status)
  }
};

struct COMMAND_RPC_GET_HEIGHT {
  struct request {
    void serialize(ISerializer &s) {
    }
  };

  struct response {
    uint64_t height;
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(height)
      KV_MEMBER(status)
    }
  };
};

struct COMMAND_RPC_GET_ISCOREREADY { 
  struct request {                                                    
    void serialize(ISerializer &s) {                               
    }                               
  };                                             
                                        
  struct response {                                                                                                     
    bool iscoreready;                
    std::string status;                   
                                                   
    void serialize(ISerializer &s) {
      KV_MEMBER(iscoreready) 
      KV_MEMBER(status) 
    }                                           
  };                                         
}; 

struct COMMAND_RPC_GET_BLOCKS_FAST {

    struct request {
    std::vector<Crypto::Hash> block_ids;
    /*first 10 blocks id goes sequential, next goes in pow(2,n) offset, like 2, 4, 8, 16, 32, 64 and so on, and the last one is always genesis block */

    void serialize(ISerializer &s) {
      serializeAsBinary(block_ids, "block_ids", s);
    }
  };

  struct response {
    std::vector<block_complete_entry> blocks;
    uint64_t start_height;
    uint64_t current_height;
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(blocks)
      KV_MEMBER(start_height)
      KV_MEMBER(current_height)
      KV_MEMBER(status)
    }
  };
};

//-----------------------------------------------

struct COMMAND_RPC_GET_BLOCKS_DETAILS_BY_HEIGHTS {
  struct request {
    std::vector<uint32_t> blockHeights;

    void serialize(ISerializer& s) {
      KV_MEMBER(blockHeights);
    }
  };

  struct response {
    std::vector<BlockDetails> blocks;
    std::string status;

    void serialize(ISerializer& s) {
      KV_MEMBER(blocks)
      KV_MEMBER(status)
    }
  };
};

struct COMMAND_RPC_GET_BLOCKS_DETAILS_BY_HASHES {
  struct request {
    std::vector<Crypto::Hash> blockHashes;

    void serialize(ISerializer& s) {
      KV_MEMBER(blockHashes);
    }
  };

  struct response {
    std::vector<BlockDetails> blocks;
    std::string status;

    void serialize(ISerializer& s) {
      KV_MEMBER(status)
      KV_MEMBER(blocks)
    }
  };
};

struct COMMAND_RPC_GET_BLOCK_DETAILS_BY_HEIGHT {
  struct request {
    uint32_t blockHeight;

    void serialize(ISerializer& s) {
      KV_MEMBER(blockHeight)
    }
  };

  struct response {
    BlockDetails block;
    std::string status;

    void serialize(ISerializer& s) {
      KV_MEMBER(status)
      KV_MEMBER(block)
    }
  };
};

struct COMMAND_RPC_GET_BLOCK_DETAILS_BY_HASH {
  struct request {
    std::string hash;

    void serialize(ISerializer& s) {
      KV_MEMBER(hash)
    }
  };

  struct response {
    BlockDetails block;
    std::string status;

    void serialize(ISerializer& s) {
      KV_MEMBER(status)
      KV_MEMBER(block)
    }
  };
};

struct COMMAND_RPC_GET_BLOCKS_HASHES_BY_TIMESTAMPS {
  struct request {
    uint64_t timestampBegin;
    uint64_t timestampEnd;
    uint32_t limit;

    void serialize(ISerializer &s) {
      KV_MEMBER(timestampBegin)
      KV_MEMBER(timestampEnd)
      KV_MEMBER(limit)
    }
  };

  struct response {
    std::vector<Crypto::Hash> blockHashes;
    uint32_t count;
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(status)
      KV_MEMBER(count)
      KV_MEMBER(blockHashes)
    }
  };
};

struct COMMAND_RPC_GET_TRANSACTION_HASHES_BY_PAYMENT_ID {
  struct request {
    std::string paymentId;

    void serialize(ISerializer &s) {
      KV_MEMBER(paymentId)
    }
  };

  struct response {
    std::vector<Crypto::Hash> transactionHashes;
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(status)
      KV_MEMBER(transactionHashes);
    }
  };
};

struct transaction_pool_response {
  std::string hash;
  uint64_t fee;
  uint64_t amount_out;
  uint64_t size;
  uint64_t receive_time;

  void serialize(ISerializer &s) {
    KV_MEMBER(hash)
    KV_MEMBER(fee)
    KV_MEMBER(amount_out)
    KV_MEMBER(size)
    KV_MEMBER(receive_time)
  }
};

struct COMMAND_RPC_GET_TRANSACTIONS_POOL_SHORT {
  typedef EMPTY_STRUCT request;

  struct response {
    std::vector<transaction_pool_response> transactions; //transactions blobs as hex
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(transactions)
      KV_MEMBER(status)
    }
  };
};

struct tx_with_output_global_indexes {
  TransactionPrefix transaction;
  Crypto::Hash hash;
  Crypto::Hash block_hash;
  uint32_t height;
  uint64_t fee;
  uint64_t timestamp;
  std::vector<uint32_t> output_indexes;

  void serialize(ISerializer &s)
  {
    KV_MEMBER(transaction)
    KV_MEMBER(hash)
    KV_MEMBER(block_hash)
    KV_MEMBER(height)
    KV_MEMBER(fee)
    KV_MEMBER(timestamp)
    KV_MEMBER(output_indexes)
  }
};

struct COMMAND_RPC_GET_RAW_TRANSACTIONS_POOL {
  typedef EMPTY_STRUCT request;

  struct response {
    std::vector<tx_with_output_global_indexes> transactions;
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(transactions)
      KV_MEMBER(status)
    }
  };
};

struct PoolTransactionDetailsData {
  Crypto::Hash id;
  Transaction tx;
  uint64_t blobSize;
  uint64_t fee;
  bool keptByBlock;
  uint64_t receiveTime;

  void serialize(ISerializer &s) {
    KV_MEMBER(id)
    KV_MEMBER(tx)
    KV_MEMBER(blobSize)
    KV_MEMBER(fee)
    KV_MEMBER(keptByBlock)
    KV_MEMBER(receiveTime)
  }
};

struct COMMAND_RPC_GET_TRANSACTIONS_POOL {
  typedef EMPTY_STRUCT request;

  struct response {
    std::vector<PoolTransactionDetailsData> transactions;
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(transactions)
      KV_MEMBER(status)
    }
  };
};

struct COMMAND_RPC_GET_TRANSACTIONS_DETAILS_BY_HASHES {
  struct request {
    std::vector<Crypto::Hash> transactionHashes;

    void serialize(ISerializer &s) {
      KV_MEMBER(transactionHashes);
    }
  };

  struct response {
    std::vector<TransactionDetails> transactions;
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(status)
      KV_MEMBER(transactions)
    }
  };
};

struct COMMAND_RPC_GET_TRANSACTION_DETAILS_BY_HASH {
  struct request {
    std::string hash;

    void serialize(ISerializer &s) {
      KV_MEMBER(hash);
    }
  };

  struct response {
    TransactionDetails transaction;
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(status)
      KV_MEMBER(transaction)
    }
  };
};

struct COMMAND_RPC_GET_BLOCK_INDEXES {
  typedef EMPTY_STRUCT request;

  struct response {
//    std::vector<std::string> txs_as_hex; //transactions blobs as hex
//    std::vector<std::string> missed_tx;  //not found transactions
//    std::string status;

    void serialize(ISerializer &s) {
//      KV_MEMBER(txs_as_hex)
//      KV_MEMBER(missed_tx)
//      KV_MEMBER(status)
    }
  };
};

struct COMMAND_RPC_GET_BLOCK {
  struct request {
    //Crypto::Hash blockhash; 
    //std::string blockhash;
    uint64_t height;

    void serialize(ISerializer &s) {
      KV_MEMBER(height)
    }
  };

  struct response {
    //std::vector<std::string> txs;
   
    uint64_t alreadyGeneratedCoins;
    uint64_t blocksize;
    uint64_t difficulty;
    uint64_t currentmediansize;
    Crypto::Hash hash;
    std::vector<Crypto::Hash> transactionHashes;

    uint8_t major_version;
    uint8_t minor_version;
    uint64_t timestamp;
    std::string prev_hash;
    uint32_t nonce;
    uint64_t depth;
    uint64_t reward;

    std::string status;

    void serialize(ISerializer &s) {
      //KV_MEMBER(txs)
      KV_MEMBER(alreadyGeneratedCoins)
      KV_MEMBER(blocksize)
      KV_MEMBER(difficulty)
      KV_MEMBER(currentmediansize)
      KV_MEMBER(hash)
      KV_MEMBER(transactionHashes)
      KV_MEMBER(major_version)
      KV_MEMBER(minor_version)
      KV_MEMBER(timestamp)
      KV_MEMBER(prev_hash)
      KV_MEMBER(nonce)
      KV_MEMBER(depth)
      KV_MEMBER(reward)
      KV_MEMBER(status)
    }
  };
};

struct keyinput_response {
  uint64_t amount;
  std::string key;

  void serialize(ISerializer &s) {
    KV_MEMBER(amount)
    KV_MEMBER(key)
  }
};

struct multisiginput_response {
  uint64_t amount;
  uint64_t signatureCount;
  uint64_t outputIndex;

  void serialize(ISerializer &s) {
    KV_MEMBER(amount)
    KV_MEMBER(signatureCount)
    KV_MEMBER(outputIndex)
  }
};

struct keyoutput_response {
  std::string keyOutput;

  void serialize(ISerializer &s) {
    KV_MEMBER(keyOutput)
  }
};

struct multisigoutput_response {
  uint64_t requiredSignatureCount;
  std::vector<std::string> keys;

  void serialize(ISerializer &s) {
    KV_MEMBER(requiredSignatureCount)
    KV_MEMBER(keys)
  }
};

struct COMMAND_RPC_GET_TRANSACTION {
  struct request {
    std::string hash;
    uint64_t blknum=0;
    uint64_t txnum=0;

    void serialize(ISerializer &s) {
      KV_MEMBER(hash)
      KV_MEMBER(blknum)
      KV_MEMBER(txnum)
    }
  };

  struct response {
    std::string status;
    std::vector<uint32_t> global_output_indexes;
    Crypto::Hash hash;
    Crypto::Hash block;
    uint32_t blockheight;
    bool orphan_status;
    uint64_t inputamt;
    uint64_t outputamt; 
    uint64_t txsize;
    uint64_t fee;

    std::vector<struct keyinput_response> inputskeyobjs;
    std::vector<struct multisiginput_response> inputsmultisigobjs;
    std::vector<struct keyoutput_response> outputskeyobjs;
    std::vector<struct multisigoutput_response> outputsmultisigobjs;

    std::vector<uint64_t> inputsamts;
    std::vector<uint64_t> outputsamts;
    std::vector<Crypto::PublicKey> outputskeys;
    std::string extra;
    Crypto::Hash prefix;
    uint64_t version;
    uint64_t unlocktime;
    Crypto::Hash paymentid;

    void serialize(ISerializer &s) {
      KV_MEMBER(global_output_indexes)
      KV_MEMBER(hash)
      KV_MEMBER(txsize)
      KV_MEMBER(block)
      KV_MEMBER(blockheight)
      KV_MEMBER(orphan_status)
      KV_MEMBER(inputamt)
      KV_MEMBER(outputamt)

      KV_MEMBER(inputskeyobjs)
      KV_MEMBER(inputsmultisigobjs)
      KV_MEMBER(outputskeyobjs)
      KV_MEMBER(outputsmultisigobjs)

      KV_MEMBER(inputsamts)
      KV_MEMBER(outputsamts)
      KV_MEMBER(outputskeys)
      KV_MEMBER(prefix)
      KV_MEMBER(version)
      KV_MEMBER(unlocktime)
      KV_MEMBER(fee)
      KV_MEMBER(paymentid)
      KV_MEMBER(extra)
      KV_MEMBER(status)
    }
  };
};

struct COMMAND_RPC_GET_TRANSACTIONS {
  struct request {
    std::vector<std::string> txs_hashes;

    void serialize(ISerializer &s) {
      KV_MEMBER(txs_hashes)
    }
  };

  struct response {
    std::vector<std::string> txs_as_hex; //transactions blobs as hex
    std::vector<std::string> missed_txs;  //not found transactions
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(txs_as_hex)
      KV_MEMBER(missed_txs)
      KV_MEMBER(status)
    }
  };
};

//-----------------------------------------------
struct COMMAND_RPC_GET_POOL_CHANGES {
  struct request {
    Crypto::Hash tailBlockId;
    std::vector<Crypto::Hash> knownTxsIds;

    void serialize(ISerializer &s) {
      KV_MEMBER(tailBlockId)
      serializeAsBinary(knownTxsIds, "knownTxsIds", s);
    }
  };

  struct response {
    bool isTailBlockActual;
    std::vector<BinaryArray> addedTxs;          // Added transactions blobs
    std::vector<Crypto::Hash> deletedTxsIds; // IDs of not found transactions
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(isTailBlockActual)
      KV_MEMBER(addedTxs)
      serializeAsBinary(deletedTxsIds, "deletedTxsIds", s);
      KV_MEMBER(status)
    }
  };
};

struct COMMAND_RPC_GET_POOL_CHANGES_LITE {
  struct request {
    Crypto::Hash tailBlockId;
    std::vector<Crypto::Hash> knownTxsIds;

    void serialize(ISerializer &s) {
      KV_MEMBER(tailBlockId)
      serializeAsBinary(knownTxsIds, "knownTxsIds", s);
    }
  };

  struct response {
    bool isTailBlockActual;
    std::vector<TransactionPrefixInfo> addedTxs;          // Added transactions blobs
    std::vector<Crypto::Hash> deletedTxsIds; // IDs of not found transactions
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(isTailBlockActual)
      KV_MEMBER(addedTxs)
      serializeAsBinary(deletedTxsIds, "deletedTxsIds", s);
      KV_MEMBER(status)
    }
  };
};

//-----------------------------------------------
struct COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES {
  
  struct request {
    Crypto::Hash txid;

    void serialize(ISerializer &s) {
      KV_MEMBER(txid)
    }
  };

  struct response {
    std::vector<uint64_t> o_indexes;
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(o_indexes)
      KV_MEMBER(status)
    }
  };
};
//-----------------------------------------------
struct COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_request {
  std::vector<uint64_t> amounts;
  uint64_t outs_count;

  void serialize(ISerializer &s) {
    KV_MEMBER(amounts)
    KV_MEMBER(outs_count)
  }
};

#pragma pack(push, 1)
struct COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_out_entry {
  uint64_t global_amount_index;
  Crypto::PublicKey out_key;
};
#pragma pack(pop)

struct COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_outs_for_amount {
  uint64_t amount;
  std::vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_out_entry> outs;

  void serialize(ISerializer &s) {
    KV_MEMBER(amount)
    serializeAsBinary(outs, "outs", s);
  }
};

struct COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_response {
  std::vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_outs_for_amount> outs;
  std::string status;

  void serialize(ISerializer &s) {
    KV_MEMBER(outs)
    KV_MEMBER(status)
  }
};

struct COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS {
  typedef COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_request request;
  typedef COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_response response;

  typedef COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_out_entry out_entry;
  typedef COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_outs_for_amount outs_for_amount;
};

//-----------------------------------------------
struct COMMAND_RPC_SEND_RAW_TX {
  struct request {
    std::string tx_as_hex;

    request() {}
    explicit request(const Transaction &);

    void serialize(ISerializer &s) {
      KV_MEMBER(tx_as_hex)
    }
  };

  struct response {
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(status)
    }
  };
};
//-----------------------------------------------
struct COMMAND_RPC_START_MINING {
  struct request {
    std::string miner_address;
    uint64_t threads_count;

    void serialize(ISerializer &s) {
      KV_MEMBER(miner_address)
      KV_MEMBER(threads_count)
    }
  };

  struct response {
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(status)
    }
  };
};
//-----------------------------------------------
struct COMMAND_RPC_GET_INFO {
  typedef EMPTY_STRUCT request;

  struct response {
    std::string status;
    std::string version;
    uint32_t height;
    std::string top_block_hash;
    uint64_t difficulty;
    uint64_t cumulative_difficulty;
    uint64_t max_cumulative_block_size;
    uint64_t next_reward;
    uint64_t min_fee;
    uint64_t tx_count;
    uint64_t tx_pool_size;
    uint64_t alt_blocks_count;
    uint64_t outgoing_connections_count;
    uint64_t incoming_connections_count;
    uint64_t rpc_connections_count;
    uint64_t white_peerlist_size;
    uint64_t grey_peerlist_size;
    uint32_t last_known_block_index;
    uint64_t start_time;
    uint8_t block_major_version;
    std::string already_generated_coins;
    std::string contact;   

    void serialize(ISerializer &s) {
      KV_MEMBER(status)
      KV_MEMBER(version)
      KV_MEMBER(height)
      KV_MEMBER(top_block_hash)
      KV_MEMBER(difficulty)
      KV_MEMBER(cumulative_difficulty)
      KV_MEMBER(max_cumulative_block_size)
      KV_MEMBER(next_reward)
      KV_MEMBER(min_fee)
      KV_MEMBER(tx_count)
      KV_MEMBER(tx_pool_size)
      KV_MEMBER(alt_blocks_count)
      KV_MEMBER(outgoing_connections_count)
      KV_MEMBER(incoming_connections_count)
      KV_MEMBER(rpc_connections_count)
      KV_MEMBER(white_peerlist_size)
      KV_MEMBER(grey_peerlist_size)
      KV_MEMBER(last_known_block_index)
      KV_MEMBER(start_time)
      KV_MEMBER(block_major_version)
      KV_MEMBER(already_generated_coins)
      KV_MEMBER(contact)   
    }
  };
};

//-----------------------------------------------
struct COMMAND_RPC_STOP_MINING {
  typedef EMPTY_STRUCT request;
  typedef STATUS_STRUCT response;
};

//-----------------------------------------------
struct COMMAND_RPC_STOP_DAEMON {
  typedef EMPTY_STRUCT request;
  typedef STATUS_STRUCT response;
};

//
struct COMMAND_RPC_GETBLOCKCOUNT {
  typedef std::vector<std::string> request;

  struct response {
    uint64_t count;
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(count)
      KV_MEMBER(status)
    }
  };
};

struct COMMAND_RPC_GETBLOCKHASH {
  struct request {
    uint32_t height;

    void serialize(ISerializer &s) {
      KV_MEMBER(height)
    }
  };

  struct response {
    std::string hash;
    void serialize(ISerializer &s) {
      KV_MEMBER(hash)
    } 
  };
};

struct COMMAND_RPC_GETBLOCKTEMPLATE {
  struct request {
    uint64_t reserve_size; //max 255 bytes
    std::string wallet_address;

    void serialize(ISerializer &s) {
      KV_MEMBER(reserve_size)
      KV_MEMBER(wallet_address)
    }
  };

  struct response {
    uint64_t difficulty;
    uint32_t height;
    uint64_t reserved_offset;
    std::string blocktemplate_blob;
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(difficulty)
      KV_MEMBER(height)
      KV_MEMBER(reserved_offset)
      KV_MEMBER(blocktemplate_blob)
      KV_MEMBER(status)
    }
  };
};

struct COMMAND_RPC_GET_CURRENCY_ID {
  typedef EMPTY_STRUCT request;

  struct response {
    std::string currency_id_blob;

    void serialize(ISerializer &s) {
      KV_MEMBER(currency_id_blob)
    }
  };
};

struct COMMAND_RPC_SUBMITBLOCK {
  typedef std::vector<std::string> request;
  typedef STATUS_STRUCT response;
};

struct block_header_response {
  uint8_t major_version;
  uint8_t minor_version;
  uint64_t timestamp;
  std::string prev_hash;
  uint32_t nonce;
  bool orphan_status;
  uint64_t height;
  uint64_t depth;
  std::string hash;
  difficulty_type difficulty;
  uint64_t reward;

  void serialize(ISerializer &s) {
    KV_MEMBER(major_version)
    KV_MEMBER(minor_version)
    KV_MEMBER(timestamp)
    KV_MEMBER(prev_hash)
    KV_MEMBER(nonce)
    KV_MEMBER(orphan_status)
    KV_MEMBER(height)
    KV_MEMBER(depth)
    KV_MEMBER(hash)
    KV_MEMBER(difficulty)
    KV_MEMBER(reward)
  }
};

struct BLOCK_HEADER_RESPONSE {
  std::string status;
  block_header_response block_header;

  void serialize(ISerializer &s) {
    KV_MEMBER(block_header)
    KV_MEMBER(status)
  }
};


struct COMMAND_RPC_GET_LAST_BLOCK_HEADER {
  typedef EMPTY_STRUCT request;
  typedef BLOCK_HEADER_RESPONSE response;
};

struct COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH {
  struct request {
    std::string hash;

    void serialize(ISerializer &s) {
      KV_MEMBER(hash)
    }
  };

  typedef BLOCK_HEADER_RESPONSE response;
};

struct COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT {
  struct request {
    uint32_t height;

    void serialize(ISerializer &s) {
      KV_MEMBER(height)
    }
  };

  typedef BLOCK_HEADER_RESPONSE response;
};

struct COMMAND_RPC_QUERY_BLOCKS {
  struct request {
    std::vector<Crypto::Hash> block_ids; //*first 10 blocks id goes sequential, next goes in pow(2,n) offset, like 2, 4, 8, 16, 32, 64 and so on, and the last one is always genesis block */
    uint64_t timestamp;

    void serialize(ISerializer &s) {
      serializeAsBinary(block_ids, "block_ids", s);
      KV_MEMBER(timestamp)
    }
  };

  struct response {
    std::string status;
    uint64_t start_height;
    uint64_t current_height;
    uint64_t full_offset;
    std::vector<BlockFullInfo> items;

    void serialize(ISerializer &s) {
      KV_MEMBER(status)
      KV_MEMBER(start_height)
      KV_MEMBER(current_height)
      KV_MEMBER(full_offset)
      KV_MEMBER(items)
    }
  };
};

struct COMMAND_RPC_QUERY_BLOCKS_LITE {
  struct request {
    std::vector<Crypto::Hash> blockIds;
    uint64_t timestamp;

    void serialize(ISerializer &s) {
      serializeAsBinary(blockIds, "block_ids", s);
      KV_MEMBER(timestamp)
    }
  };

  struct response {
    std::string status;
    uint64_t startHeight;
    uint64_t currentHeight;
    uint64_t fullOffset;
    std::vector<BlockShortInfo> items;

    void serialize(ISerializer &s) {
      KV_MEMBER(status)
      KV_MEMBER(startHeight)
      KV_MEMBER(currentHeight)
      KV_MEMBER(fullOffset)
      KV_MEMBER(items)
    }
  };
};

struct COMMAND_RPC_CHECK_TX_KEY {
  struct request {
    std::string transaction_id;
    std::string transaction_key;
    std::string address;

    void serialize(ISerializer &s) {
      KV_MEMBER(transaction_id)
      KV_MEMBER(transaction_key)
      KV_MEMBER(address)
    }
  };

  struct response {
    uint64_t amount;
    std::vector<TransactionOutput> outputs;
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(amount)
      KV_MEMBER(outputs)
      KV_MEMBER(status)
    }
  };
};

struct COMMAND_RPC_CHECK_TX_WITH_PRIVATE_VIEW_KEY {
  struct request {
    std::string transaction_id;
    std::string view_key;
    std::string address;

    void serialize(ISerializer &s) {
      KV_MEMBER(transaction_id)
      KV_MEMBER(view_key)
      KV_MEMBER(address)
    }
  };

  struct response {
    uint64_t amount;
    std::vector<TransactionOutput> outputs;
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(amount)
      KV_MEMBER(outputs)
      KV_MEMBER(status)
    }
  };
};

struct COMMAND_RPC_CHECK_TX_PROOF {
  struct request {
    std::string transaction_id;
    std::string destination_address;
    std::string signature;

    void serialize(ISerializer &s) {
      KV_MEMBER(transaction_id)
      KV_MEMBER(destination_address)
      KV_MEMBER(signature)
    }
  };

  struct response {
    bool signature_valid;
    uint64_t received_amount;
    std::vector<TransactionOutput> outputs;
    uint32_t confirmations = 0;
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(signature_valid)
      KV_MEMBER(received_amount)
      KV_MEMBER(outputs)
      KV_MEMBER(confirmations)
      KV_MEMBER(status)
    }
  };
};

}
