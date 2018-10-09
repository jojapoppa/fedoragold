// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ITransaction.h"

#include <numeric>
#include <system_error>

#include "CryptoNoteCore/CryptoNoteBasic.h"
#include "CryptoNoteCore/TransactionApiExtra.h"
#include "TransactionUtils.h"
#include "CryptoNoteCore/CryptoNoteTools.h"

namespace CryptoNote {

using namespace Crypto;

class TransactionPrefixImpl : public ITransactionReader {
public:
  TransactionPrefixImpl();
  TransactionPrefixImpl(const TransactionPrefix& prefix, const Hash& transactionHash);

  virtual ~TransactionPrefixImpl() { }

  virtual Hash getTransactionHash() const override;
  virtual Hash getTransactionPrefixHash() const override;
  virtual PublicKey getTransactionPublicKey() const override;
  virtual uint64_t getUnlockTime() const override;

  // extra
  virtual bool getPaymentId(Hash& paymentId) const override;
  virtual bool getExtraNonce(BinaryArray& nonce) const override;
  virtual BinaryArray getExtra() const override;

  // inputs
  virtual size_t getInputCount() const override;
  virtual uint64_t getInputTotalAmount() const override;
  virtual TransactionTypes::InputType getInputType(size_t index) const override;
  virtual void getInput(size_t index, KeyInput& input) const override;
  virtual void getInput(size_t index, MultisignatureInput& input) const override;

  // outputs
  virtual size_t getOutputCount() const override;
  virtual uint64_t getOutputTotalAmount() const override;
  virtual TransactionTypes::OutputType getOutputType(size_t index) const override;
  virtual void getOutput(size_t index, KeyOutput& output, uint64_t& amount) const override;
  virtual void getOutput(size_t index, MultisignatureOutput& output, uint64_t& amount) const override;

  // signatures
  virtual size_t getRequiredSignaturesCount(size_t inputIndex) const override;
  virtual bool findOutputsToAccount(const AccountPublicAddress& addr, const SecretKey& viewSecretKey, std::vector<uint32_t>& outs, uint64_t& outputAmount) const override;

  // various checks
  virtual bool validateInputs() const override;
  virtual bool validateOutputs() const override;
  virtual bool validateSignatures() const override;

  // serialized transaction
  virtual BinaryArray getTransactionData() const override;

  virtual bool getTransactionSecretKey(SecretKey& key) const override;

private:
  TransactionPrefix m_txPrefix;
  TransactionExtra m_extra;
  Hash m_txHash;
};

}
