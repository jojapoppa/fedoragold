// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <ios>
#include "CryptoNoteTools.h"
#include "CryptoNoteFormatUtils.h"

namespace CryptoNote {
template<>
bool toBinaryArray(const BinaryArray& object, BinaryArray& binaryArray) {
  try {
    Common::VectorOutputStream stream(binaryArray);
    BinaryOutputStreamSerializer serializer(stream);
    std::string oldBlob = Common::asString(object);
    serializer(oldBlob, "");
  } catch (std::exception&) {
    return false;
  }

  return true;
}

void getBinaryArrayHash(const BinaryArray& binaryArray, Crypto::Hash& hash) {
  cn_fast_hash(binaryArray.data(), binaryArray.size(), hash);
}

Crypto::Hash getBinaryArrayHash(const BinaryArray& binaryArray) {
  Crypto::Hash hash;
  getBinaryArrayHash(binaryArray, hash);
  return hash;
}

uint64_t getInputAmount(const Transaction& transaction) {
  uint64_t amount = 0;
  for (auto& input : transaction.inputs) {
    if (input.type() == typeid(KeyInput)) {
      amount += boost::get<KeyInput>(input).amount;
    } else if (input.type() == typeid(MultisignatureInput)) {
      amount += boost::get<MultisignatureInput>(input).amount;
    }
  }

  return amount;
}

std::vector<uint64_t> getInputsAmounts(const Transaction& transaction) {
  std::vector<uint64_t> inputsAmounts;
  inputsAmounts.reserve(transaction.inputs.size());

  for (auto& input: transaction.inputs) {
    if (input.type() == typeid(KeyInput)) {
      inputsAmounts.push_back(boost::get<KeyInput>(input).amount);
    } else if (input.type() == typeid(MultisignatureInput)) {
      inputsAmounts.push_back(boost::get<MultisignatureInput>(input).amount);
    } else {
      inputsAmounts.push_back(0); // retains ordinal position
    }
  }

  return inputsAmounts;
}

std::vector<KeyInput> getInputsKeyObjs(const Transaction& transaction) {
  std::vector<KeyInput> inputsAmounts;
  inputsAmounts.reserve(transaction.inputs.size());

  for (auto& input: transaction.inputs) {
    if (input.type() == typeid(KeyInput)) {
      const KeyInput& keyInput = boost::get<KeyInput>(input);
      inputsAmounts.push_back(keyInput);
    } else {
      inputsAmounts.push_back(*(new KeyInput()));
    }
  }

  return inputsAmounts;
}

std::vector<MultisignatureInput> getInputsMultisigObjs(const Transaction& transaction) {
  std::vector<MultisignatureInput> inputsAmounts;
  inputsAmounts.reserve(transaction.inputs.size());

  for (auto& input: transaction.inputs) {
    if (input.type() == typeid(MultisignatureInput)) {
      const MultisignatureInput& multisigInput = boost::get<MultisignatureInput>(input);
      inputsAmounts.push_back(multisigInput);
    } else {
      inputsAmounts.push_back(*(new MultisignatureInput()));
    }
  }

  return inputsAmounts;
}

std::vector<Crypto::PublicKey> getOutputsKeys(const Transaction& transaction) {
  std::vector<Crypto::PublicKey> outputsKeys;
  outputsKeys.reserve(transaction.outputs.size());

  for (auto& output: transaction.outputs) {
    if (output.target.type() == typeid(KeyOutput)) {
      outputsKeys.push_back(boost::get<KeyOutput>(output.target).key);
    } else if (output.target.type() == typeid(MultisignatureOutput)) {
      std::vector<Crypto::PublicKey> keys = boost::get<MultisignatureOutput>(output.target).keys;
      for (auto& key: keys) {
        outputsKeys.push_back(key);
      }
    }
  }

  return outputsKeys;
}

std::vector<uint64_t> getOutputsAmounts(const Transaction& transaction) {
  std::vector<uint64_t> outputsAmounts;
  outputsAmounts.reserve(transaction.outputs.size());

  for (auto& output: transaction.outputs) {
      outputsAmounts.push_back(output.amount);
  }

  return outputsAmounts;
}

std::vector<KeyOutput> getOutputsKeyObjs(const Transaction& transaction) {
  std::vector<KeyOutput> outputsAmounts;
  outputsAmounts.reserve(transaction.outputs.size());

  for (auto& output: transaction.outputs) {
    if (output.target.type() == typeid(KeyOutput)) {
      const KeyOutput& keyOutput = boost::get<KeyOutput>(output.target);
      outputsAmounts.push_back(keyOutput);
    } else {
      outputsAmounts.push_back(*(new KeyOutput()));
    }
  }

  return outputsAmounts;
}

std::vector<MultisignatureOutput> getOutputsMultisigObjs(const Transaction& transaction) {
  std::vector<MultisignatureOutput> outputsAmounts;
  outputsAmounts.reserve(transaction.outputs.size());

  for (auto& output: transaction.outputs) {
    if (output.target.type() == typeid(MultisignatureOutput)) {
      const MultisignatureOutput& multisigOutput = boost::get<MultisignatureOutput>(output.target);
      outputsAmounts.push_back(multisigOutput);
    } else {
      outputsAmounts.push_back(*(new MultisignatureOutput()));
    }
  }

  return outputsAmounts;
}

uint64_t getOutputAmount(const Transaction& transaction) {
  uint64_t amount = 0;
  for (auto& output : transaction.outputs) {
    amount += output.amount;
  }

  return amount;
}

void decomposeAmount(uint64_t amount, uint64_t dustThreshold, std::vector<uint64_t>& decomposedAmounts) {
  decompose_amount_into_digits(amount, dustThreshold,
    [&](uint64_t amount) {
    decomposedAmounts.push_back(amount);
  },
    [&](uint64_t dust) {
    decomposedAmounts.push_back(dust);
  }
  );
}

}
