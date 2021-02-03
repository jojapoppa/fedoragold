// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cstdint>
#include <initializer_list>

namespace CryptoNote {
namespace parameters {

//jojapoppa, fix these size_t's (to uint64_t) at next Soft Fork
//  also this MAX_BLOCK_NUMBER is too small... think 50 years
const uint64_t CRYPTONOTE_MAX_BLOCK_NUMBER                   = 500000000;
const size_t   CRYPTONOTE_MAX_BLOCK_BLOB_SIZE                = 500000000;
const size_t   CRYPTONOTE_MAX_TX_SIZE                        = 1000000000;
// Currency-specific address prefix
const uint64_t CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX       = 127;
// Choose maturity period for your currency
const size_t   CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW          = 48;
const uint64_t CRYPTONOTE_BLOCK_FUTURE_TIME_LIMIT            = 60 * 60 * 2;

const size_t   BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW             = 60;

// Specify total number of available coins
// ((uint64_t)(-1)) equals to 18446744073709551616 coins
// or you can define number explicitly UINT64_C(858986905600000000)
const uint64_t MONEY_SUPPLY                                  = 10000000000000000000U;
const unsigned EMISSION_SPEED_FACTOR                         = 26;
static_assert(EMISSION_SPEED_FACTOR <= 8 * sizeof(uint64_t), "Bad EMISSION_SPEED_FACTOR");
const uint64_t TAIL_EMISSION_REWARD                          = UINT64_C(1000000000000);

//TODO Define number of blocks for block size median calculation
const size_t   CRYPTONOTE_REWARD_BLOCKS_WINDOW               = 100;
const size_t   CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE     = 100000; //size of block (bytes) after which reward for block calculated using block size
const size_t   CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE        = 600;
// Define number of digits
const size_t   CRYPTONOTE_DISPLAY_DECIMAL_POINT              = 8;
// Define minimum fee for transactions
const uint64_t MINIMUM_FEE                                   = 1000000;
const uint64_t MAXIMUM_FEE                                   = UINT64_C(100000000000);
const uint64_t DEFAULT_DUST_THRESHOLD                        = 0;

// Define preferred block's target time
const uint64_t DIFFICULTY_TARGET                             = 25; // seconds
const uint64_t EXPECTED_NUMBER_OF_BLOCKS_PER_DAY             = 17; // 24 * 60 * 60 / DIFFICULTY_TARGET;
// There are options to tune CryptoNote's difficulty retargeting function.
// We recommend not to change it.
const size_t   DIFFICULTY_WINDOW                             = EXPECTED_NUMBER_OF_BLOCKS_PER_DAY; // blocks
const size_t   DIFFICULTY_CUT                                = 0;  // timestamps to cut after sorting
const size_t   DIFFICULTY_LAG                                = 0;
static_assert(2 * DIFFICULTY_CUT <= DIFFICULTY_WINDOW - 2, "Bad DIFFICULTY_WINDOW or DIFFICULTY_CUT");

const size_t   MAX_BLOCK_SIZE_INITIAL                        = 100000; //20 * 1024;
const uint64_t MAX_BLOCK_SIZE_GROWTH_SPEED_NUMERATOR         = 100 * 1024;
const uint64_t MAX_BLOCK_SIZE_GROWTH_SPEED_DENOMINATOR       = 365 * 24 * 60 * 60 / DIFFICULTY_TARGET;

const uint64_t CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS     = 1;
const uint64_t CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS    = DIFFICULTY_TARGET * CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS;

const uint64_t CRYPTONOTE_MEMPOOL_TX_LIVETIME                = 60 * 60 * 24;     //seconds, one day
const uint64_t CRYPTONOTE_MEMPOOL_TX_FROM_ALT_BLOCK_LIVETIME = 60 * 60 * 24 * 7; //seconds, one week
const uint64_t CRYPTONOTE_NUMBER_OF_PERIODS_TO_FORGET_TX_DELETED_FROM_POOL = 7;  // CRYPTONOTE_NUMBER_OF_PERIODS_TO_FORGET_TX_DELETED_FROM_POOL * CRYPTONOTE_MEMPOOL_TX_LIVETIME = time to forget tx

const uint64_t MAX_TRANSACTION_SIZE_LIMIT                    = CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE / 4 - CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE;

const size_t   FUSION_TX_MAX_SIZE                            = CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE * 30 / 100;
const size_t   FUSION_TX_MIN_INPUT_COUNT                     = 6;
const size_t   FUSION_TX_MIN_IN_OUT_COUNT_RATIO              = 3;

const char     CRYPTONOTE_BLOCKS_FILENAME[]                  = "blocks.dat";
const char     CRYPTONOTE_BLOCKINDEXES_FILENAME[]            = "blockindexes.dat";
const char     CRYPTONOTE_BLOCKSCACHE_FILENAME[]             = "blockscache.dat";
const char     CRYPTONOTE_POOLDATA_FILENAME[]                = "poolstate.bin";
const char     P2P_NET_DATA_FILENAME[]                       = "p2pstate.bin";
const char     CRYPTONOTE_BLOCKCHAIN_INDICES_FILENAME[]      = "blockchainindices.dat";
const char     MINER_CONFIG_FILE_NAME[]                      = "miner_conf.json";
} // parameters

// Put here the name of your currency
const char     CRYPTONOTE_NAME[]                             = "fedoragold";

const char GENESIS_COINBASE_TX_HEX[] = "013001ff0001a282a38eab04029b2e4c0281c0b02e7c53291a94d1d0cbff8883f8024f5142ee494ffbbd088071210199f0e78c761f6b6788ec218e9cc572417265612339fb56369ee6105b85e3de01";

const uint8_t  CURRENT_TRANSACTION_VERSION                   =  1;
const uint8_t  BLOCK_MAJOR_VERSION_1                         =  1;
const uint8_t  BLOCK_MINOR_VERSION_0                         =  0;

//jojapoppa, how Turtlecoin does a soft fork... for later
//const std::unordered_map<uint8_t, std::function<void(const void *data, size_t length, Crypto::Hash &hash)>>
//  HASHING_ALGORITHMS_BY_BLOCK_VERSION = {
//    {BLOCK_MAJOR_VERSION_1, Crypto::cn_slow_hash_v0}, /* From zero */
//    {BLOCK_MAJOR_VERSION_6, Crypto::chukwa_slow_hash} /* UPGRADE_HEIGHT_V6 */
//};

// default: blocks ids count in synchronizing
const size_t   BLOCKS_IDS_SYNCHRONIZING_DEFAULT_COUNT        =  10000; 
// default: blocks count in blocks downloading 
const size_t   BLOCKS_SYNCHRONIZING_DEFAULT_COUNT            =  75;

const size_t   CURRENCY_PROTOCOL_MAX_OBJECT_REQUEST_COUNT    =  2000;
const size_t   COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT         =  1000;

// This port will be used by the daemon to establish connections with p2p network
const int      P2P_DEFAULT_PORT                              = 30158;
// This port will be used by the daemon to interact with simlewallet
const int      RPC_DEFAULT_PORT                              = 30159;

const size_t   P2P_LOCAL_WHITE_PEERLIST_LIMIT                =  1000;
const size_t   P2P_LOCAL_GRAY_PEERLIST_LIMIT                 =  5000;

const size_t   P2P_CONNECTION_MAX_WRITE_BUFFER_SIZE          = 20 * 1024 * 1024; // 16 MB //20
const uint32_t P2P_DEFAULT_CONNECTIONS_COUNT                 = 16;
const size_t   P2P_DEFAULT_WHITELIST_CONNECTIONS_PERCENT     = 70;
const uint32_t P2P_DEFAULT_HANDSHAKE_INTERVAL                = 60;            // seconds
const uint32_t P2P_DEFAULT_PACKET_MAX_SIZE                   = 50000000;      // 50000000 bytes maximum packet size
const uint32_t P2P_DEFAULT_PEERS_IN_HANDSHAKE                = 250;
const uint32_t P2P_DEFAULT_CONNECTION_TIMEOUT                = 10000;         // 10 seconds 
const uint32_t P2P_DEFAULT_PING_CONNECTION_TIMEOUT           = 5000;
const uint64_t P2P_DEFAULT_INVOKE_TIMEOUT                    = 60 * 3 * 1000; // 3 minutes 
const size_t   P2P_DEFAULT_HANDSHAKE_INVOKE_TIMEOUT          = 9000;
const char     P2P_STAT_TRUSTED_PUB_KEY[]                    = "";

// Add here your network seed nodes
const std::initializer_list<const char*> SEED_NODES = {
  "202.182.106.252:30158",  // blockexplorer & seed
  "213.136.89.252:30158"   // pool seed 
};

struct CheckpointData {
  uint32_t height;
  const char* blockId;
};

#ifdef __GNUC__
__attribute__((unused))
#endif

const uint32_t UPGRADE_HEIGHT_V2 = 2815800; // Coinbase spend check - closing exploit

// You may add here other checkpoints using the following format:
// {<block height>, "<block hash>"},
const std::initializer_list<CheckpointData> CHECKPOINTS = {
  { 0,       "c01db05b73a7bdb03cff6ce9b9104096523b542ee40944e7726c3f589eff618d" },
  { 1,       "9fabc5641fe3f9333b60d1fc295975e790d6bf9876e9b48899b00b1c00d9dfbd" },
  { 1192156, "d4d514f0ebeecd9f0847f4357a3baaa133a73fd27a4837759d939d4f152197a0" },
  { 1192782, "728766de7334a5151e3d9f1cf3677e5284403b53d72a1b4f8d892580123578a6" },
  { 1192783, "33c8862c59fc65e1dea0e5e708437e2ae2284bca387e52cd627431d7ab00cf6b" },
  { 1192784, "e53958acc1414a66f187f45532126a4db4cf1dbc67c5066493d8f561034d2b2f" },
  { 1192785, "28ab24d976431e5cdedc33dbe612034688b43ba3fdcf9522cdaf4f09c2d44fa6" },
  { 1386085, "6d7be65f3d02a1c38782284b82d09be4a613ab7c216a1fc41f8767a3c7948ee8" },
  { 1391474, "1caab6ce6a4e52c09ae84da074359702ffae362acbda03a09d875d563e421a0e" },
  { 1391475, "ba660437996fdc103f9bb630536bb3d8b1689916f8370cac315eec95b4a1410d" },
  { 1410011, "4b42ebeff2876543858a9528244ba5bec4521572b7f6c692576e6814850a8e26" },
  { 1410477, "fcd10d86ece5633869e25c6e54d7525602d17f0c43c322ba44c89d7f440ce44a" },
  { 1410478, "94e49a37962f12b88d54de76a035254ef82e1a952e982569ea418c4f8b4539d6" },
  { 1412056, "16743416f33f6b923e268b693a89533860d2f80bdb30835bb5b34a401c14e9ef" },
  { 1414791, "e8b934217d40120d6ee47167fd39febf3c7286121c53cf4edf8ff7bb371b8522" },
  { 1415284, "ea812d907def2f7d41eba3ccab2851b58c924d703f485a3a4cfc0822b0bb12ce" },
  { 1418082, "db1a346269efcb3a2aff3e6a4aeb2c79a91abc33d85207d577c02b177b5156da" },
  { 1461743, "723955cf87a939652b03f4d66a1c1fe9f7cafb2e43fb3ac200b0a48787189f50" },
  { 1483325, "9cffd18845f94b7fd8a7a067732473e807c9a4abedbd952f72188a2bd6668639" },
  { 1561192, "c42dfb93904f41b411e3f003c6c0ec87fdae33a39496f8ab92d2e5e211554477" },
  { 1561193, "982d2347a78b5f6304cc1e4691dfdfb5f9aafd94d261ff732a617edb80526c86" },
  { 1600182, "855343c2f5071a4247999233a6572db4d9754f9ffad403f6e1bc0de706f8df50" },
  { 1624996, "e814ac64a10eae90e18b6c5da3a10c75bd51c12cd1973e01b49df17b93cb76a7" },
  { 2140644, "5c264156e9df8648d0e1313be71647ee55afe9d5ebd530dc1146dbcf469b76c6" },
  { 2140645, "41588dca905c858454f79b44e58f773bfe66eafff66c42c67fc211fbd84b94ec" },
  { 2163935, "a924ca448707a025d428777778e130adf8dc9a9d3a018717ced6c98bab096b73" },
  { 2163936, "251e476e86b66a45623606ae5c690ace50d07dc4356be22add25cd8e9528d0bf" },
  { 2164043, "9b041e44b6a8b78eecb2ef24fc47e5ccfed63333fe6b98bb6ec298a241240686" },
  { 2164045, "bc181ea1429c8d5e50ca253fed2bf42aaea1c152c271a17a68a3435048685a0c" },
  { 2502350, "883ea69d5e7574c57d319156f3e6c41e005033ad2cfe0b5209e910b923c05327" },
  { 2776398, "7a3ffbb938d4d18d290f0bfe7b3a6de14abc2b7da5844e08b7688e110c3dda6d" },
  { 2776399, "11e3185bd558244cd7327e2540782ab786489c42908ddc3a71b874beca7d0cb0" },
  { 2782955, "31ca55792dfe1c02c3787f9a6a135cbbad110edd2b51e11f40289f1a57880cfc" }
};
} // CryptoNote

#define ALLOW_DEBUG_COMMANDS
