// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <array>
#include <string>
#include <vector>

#include "CryptoTypes.h"
#include "CryptoNote.h"

#include <boost/variant.hpp>

namespace CryptoNote {

enum class TransactionRemoveReason : uint8_t 
{ 
  INCLUDED_IN_BLOCK = 0, 
  TIMEOUT = 1
};

struct TransactionOutputToKeyDetails {
  Crypto::PublicKey txOutKey;
};

struct TransactionOutputMultisignatureDetails {
  std::vector<Crypto::PublicKey> keys;
  uint32_t requiredSignatures;
};

struct TransactionOutputDetails {
  uint64_t amount;
  uint32_t globalIndex;

  TransactionOutput outpt; // needed a version that serializes

  boost::variant<
    TransactionOutputToKeyDetails,
    TransactionOutputMultisignatureDetails> output;
};

struct TransactionOutputReferenceDetails {
  Crypto::Hash transactionHash;
  size_t number;
};

struct TransactionInputGenerateDetails {
  uint32_t height;
};

struct TransactionInputToKeyDetails {
  std::vector<uint32_t> outputIndexes;
  Crypto::KeyImage keyImage;
  uint64_t mixin;
  std::vector<TransactionOutputReferenceDetails> output;
};

struct TransactionInputMultisignatureDetails {
  uint32_t signatures;
  TransactionOutputReferenceDetails output;
};

struct KeyInputDetails {
  KeyInput input;
  uint64_t mixin;
  std::vector<TransactionOutputReferenceDetails> outputs;
};

struct TransactionInputDetails {
  uint64_t amount;

  boost::variant<
    TransactionInputGenerateDetails,
    KeyInputDetails,
    TransactionInputMultisignatureDetails> input;
};

struct TransactionExtraDetails {
  std::vector<size_t> padding;
  std::vector<Crypto::PublicKey> publicKey; 
  std::vector<std::string> nonce;
  std::vector<uint8_t> raw;
};

struct TransactionDetails {
  Crypto::Hash hash;
  uint64_t size = 0;
  uint64_t fee = 0;
  uint64_t totalInputsAmount = 0;
  uint64_t totalOutputsAmount = 0;
  uint64_t mixin = 0;
  uint64_t unlockTime = 0;
  uint64_t timestamp = 0;
  uint8_t version = 0;
  Crypto::Hash paymentId;
  bool hasPaymentId = false;
  bool inBlockchain = false;
  Crypto::Hash blockHash;
  uint32_t blockHeight = 0;
  TransactionExtraDetails extra;
  std::vector<std::vector<Crypto::Signature>> signatures;
  std::vector<TransactionInputDetails> inputs;
  std::vector<TransactionOutputDetails> outputs;
};

struct BlockDetails {
  uint8_t majorVersion;
  uint8_t minorVersion;
  uint64_t timestamp;
  Crypto::Hash prevBlockHash;
  uint32_t nonce;
  bool isOrphaned;
  uint32_t height;
  Crypto::Hash hash;
  uint64_t difficulty;
  uint64_t reward;
  uint64_t baseReward;
  uint64_t blockSize;
  uint64_t transactionsCumulativeSize;
  uint64_t alreadyGeneratedCoins;
  uint64_t alreadyGeneratedTransactions;
  uint64_t sizeMedian;
  double penalty;
  uint64_t totalFeeAmount;
  std::vector<TransactionDetails> transactions;
};

}
