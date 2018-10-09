// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ITransaction.h"
#include "TransactionApiExtra.h"
#include "TransactionUtils.h"

#include "Account.h"
#include "CryptoNoteCore/CryptoNoteTools.h"
#include "CryptoNoteConfig.h"

#include <boost/optional.hpp>
#include <numeric>
#include <unordered_set>

using namespace Crypto;

namespace {

  using namespace CryptoNote;

  void derivePublicKey(const AccountPublicAddress& to, const SecretKey& txKey, size_t outputIndex, PublicKey& ephemeralKey) {
    KeyDerivation derivation;
    generate_key_derivation(to.viewPublicKey, txKey, derivation);
    derive_public_key(derivation, outputIndex, to.spendPublicKey, ephemeralKey);
  }

}

namespace CryptoNote {

  using namespace Crypto;

  ////////////////////////////////////////////////////////////////////////
  // class Transaction declaration
  ////////////////////////////////////////////////////////////////////////

  class TransactionImpl : public ITransaction {
  public:
    TransactionImpl();
    TransactionImpl(const BinaryArray& txblob);
    TransactionImpl(const CryptoNote::Transaction& tx);
  
    // ITransactionReader
    virtual Hash getTransactionHash() const override;
    virtual Hash getTransactionPrefixHash() const override;
    virtual PublicKey getTransactionPublicKey() const override;
    virtual uint64_t getUnlockTime() const override;
    virtual bool getPaymentId(Hash& hash) const override;
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

    virtual size_t getRequiredSignaturesCount(size_t index) const override;
    virtual bool findOutputsToAccount(const AccountPublicAddress& addr, const SecretKey& viewSecretKey, std::vector<uint32_t>& outs, uint64_t& outputAmount) const override;

    // various checks
    virtual bool validateInputs() const override;
    virtual bool validateOutputs() const override;
    virtual bool validateSignatures() const override;

    // get serialized transaction
    virtual BinaryArray getTransactionData() const override;

    // ITransactionWriter

    virtual void setUnlockTime(uint64_t unlockTime) override;
    virtual void setPaymentId(const Hash& hash) override;
    virtual void setExtraNonce(const BinaryArray& nonce) override;
    virtual void appendExtra(const BinaryArray& extraData) override;

    // Inputs/Outputs 
    virtual size_t addInput(const KeyInput& input) override;
    virtual size_t addInput(const MultisignatureInput& input) override;
    virtual size_t addInput(const AccountKeys& senderKeys, const TransactionTypes::InputKeyInfo& info, KeyPair& ephKeys) override;

    virtual size_t addOutput(uint64_t amount, const AccountPublicAddress& to) override;
    virtual size_t addOutput(uint64_t amount, const std::vector<AccountPublicAddress>& to, uint32_t requiredSignatures) override;
    virtual size_t addOutput(uint64_t amount, const KeyOutput& out) override;
    virtual size_t addOutput(uint64_t amount, const MultisignatureOutput& out) override;

    virtual void signInputKey(size_t input, const TransactionTypes::InputKeyInfo& info, const KeyPair& ephKeys) override;
    virtual void signInputMultisignature(size_t input, const PublicKey& sourceTransactionKey, size_t outputIndex, const AccountKeys& accountKeys) override;
    virtual void signInputMultisignature(size_t input, const KeyPair& ephemeralKeys) override;


    // secret key
    virtual bool getTransactionSecretKey(SecretKey& key) const override;
    virtual void setTransactionSecretKey(const SecretKey& key) override;

  private:

    void invalidateHash();

    std::vector<Signature>& getSignatures(size_t input);

    const SecretKey& txSecretKey() const {
      if (!secretKey) {
        throw std::runtime_error("Operation requires transaction secret key");
      }
      return *secretKey;
    }

    void checkIfSigning() const {
      if (!transaction.signatures.empty()) {
        throw std::runtime_error("Cannot perform requested operation, since it will invalidate transaction signatures");
      }
    }

    CryptoNote::Transaction transaction;
    boost::optional<SecretKey> secretKey;
    mutable boost::optional<Hash> transactionHash;
    TransactionExtra extra;
  };
}
