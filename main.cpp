/**
 * @file main.cpp
 * @author Chen Enhua (陈恩华)
 * @brief CEH-Orbit Blockchain Fire Seed V1 - 可视化调试版
 *
 * 说明：
 * 1. 本文件为单文件完整演示版
 * 2. 包含：
 *    - CEH-Orbit KeyGen / Sign / Verify
 *    - Wallet / Address
 *    - Transaction / TxID
 *    - Block / Header / Merkle
 *    - Simplified PoW
 *    - Account State Machine
 *    - Local Gossip Simulation
 *    - Qt 参数调试面板
 *    - Qt 实时图表渲染
 *
 * 严正声明：
 * - 本实现为研究原型，不是商用品
 * - 未完成形式化安全证明
 * - 未完成侧信道防护
 * - 未兼容现有主流公链
 * - 核心基于 CEH_Orbit 非商业许可协议
 */

#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QTimer>
#include <QGroupBox>
#include <QScrollArea>
#include <QTableWidget>
#include <QHeaderView>
#include <QDateTime>
#include <QPainter>
#include <QPainterPath>
#include <QRandomGenerator>
#include <QSizePolicy>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QChart>

#include <openssl/sha.h>

#include <vector>
#include <array>
#include <string>
#include <sstream>
#include <iomanip>
#include <random>
#include <chrono>
#include <algorithm>
#include <set>
#include <map>
#include <unordered_map>
#include <cmath>
#include <cstdint>
#include <cstring>



using namespace std;

// ============================================================
// 一、CEH-Orbit Blockchain Fire Seed 核心
// ============================================================
namespace CEH_Orbit_Blockchain_FireSeed {

// ------------------------------------------------------------
// 协议元数据
// ------------------------------------------------------------
static const char* PROTOCOL_NAME    = "CEH-Orbit Blockchain Fire Seed V1";
static const char* ARCH_IDENTIFIER  = "CEH_ORBIT_BLOCKCHAIN_FIRESEED_V1_BY_CHEN_ENHUA";
static const char* AUTHOR_NAME      = "Chen Enhua";
static const char* AUTHOR_EMAIL     = "a106079595@qq.com";

static const char* DOMAIN_CHALLENGE = "CEH_ORBIT_CHALLENGE_V1";
static const char* DOMAIN_BIND      = "CEH_ORBIT_BIND_V1";
static const char* DOMAIN_ADDRESS   = "CEH_ORBIT_ADDRESS_V1";
static const char* DOMAIN_TXID      = "CEH_ORBIT_TXID_V1";
static const char* DOMAIN_BLOCK     = "CEH_ORBIT_BLOCK_V1";

// ------------------------------------------------------------
// 参数
// ------------------------------------------------------------
struct Params {
    int N = 128;
    int Q = 3329;
    int NAV_ZONES = 16;
    int DELTA = 32;
    int SIG_BOUND = 260;
    int CHALLENGE_WT = 8;
    int Y_MIN = -200;
    int Y_MAX = 200;

    int genesis_balance = 1000;
    int mining_reward = 50;
    int pow_difficulty = 2;

    int attack_rounds = 10000;
    int collision_trials = 2000;
    int basin_rounds = 500;
};

// ------------------------------------------------------------
// 基础类型
// ------------------------------------------------------------
using Poly = std::vector<int>;

struct OrbitHead {
    uint64_t lsh0 = 0;
    uint64_t lsh1 = 0;
    std::vector<int> phase;
};

struct KeyPair {
    Poly a;
    Poly s;
    Poly t;
};

struct Signature {
    std::vector<int> z;
    OrbitHead head;
    Poly c;
    uint64_t bind_hash = 0;
};

struct VerifyResult {
    bool ok = false;
    bool z_bound_ok = false;
    bool bind_ok = false;
    bool head_ok = false;
    bool chal_ok = false;

    int lsh_dist = -1;
    int phase_dist = -1;

    Poly recovered_w;
    OrbitHead recovered_head;
    Poly recovered_c;
};

enum class TxType {
    TRANSFER = 0,
    COINBASE = 1
};

struct Transaction {
    TxType type = TxType::TRANSFER;
    std::string from;
    std::string to;
    uint64_t amount = 0;
    uint64_t nonce = 0;
    uint64_t timestamp_ms = 0;
    std::string memo;
    Signature sig;
    std::string txid;
};

struct BlockHeader {
    uint64_t height = 0;
    std::string prev_hash;
    std::string merkle_root;
    uint64_t timestamp_ms = 0;
    uint64_t difficulty = 0;
    uint64_t nonce = 0;
};

struct Block {
    BlockHeader header;
    std::vector<Transaction> txs;
    std::string block_hash;
};

struct AccountState {
    uint64_t balance = 0;
    uint64_t nonce = 0;
};

struct Wallet {
    std::string owner;
    KeyPair keypair;
    std::string address;
};

struct BenchmarkResult {
    double keygen_ms = 0.0;
    double sign_ms = 0.0;
    double verify_ms = 0.0;
    int serialized_size = 0;
};

struct AttackResult {
    int success = 0;
    int rounds = 0;
};

struct BasinPoint {
    int amplitude = 0;
    int pass = 0;
    int rounds = 0;
};

// ------------------------------------------------------------
// 工具函数
// ------------------------------------------------------------
inline int mod_q(long long x, int Q) {
    int r = static_cast<int>(x % Q);
    if (r < 0) r += Q;
    return r;
}

inline int centered(int x, int Q) {
    x = mod_q(x, Q);
    if (x > Q / 2) x -= Q;
    return x;
}

template<typename T>
std::string prefix_vec(const std::vector<T>& v, int cnt = 8) {
    std::ostringstream oss;
    int lim = std::min<int>(cnt, static_cast<int>(v.size()));
    for (int i = 0; i < lim; ++i) {
        if (i) oss << ", ";
        oss << v[i];
    }
    return oss.str();
}

uint64_t now_ms() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

// ------------------------------------------------------------
// SHA256
// ------------------------------------------------------------
std::vector<uint8_t> sha256_bytes(const std::vector<uint8_t>& input) {
    uint8_t hash[SHA256_DIGEST_LENGTH];
    SHA256(input.data(), input.size(), hash);
    return std::vector<uint8_t>(hash, hash + SHA256_DIGEST_LENGTH);
}

std::string sha256_hex(const std::vector<uint8_t>& input) {
    auto digest = sha256_bytes(input);
    std::ostringstream oss;
    for (unsigned char c : digest) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)c;
    }
    return oss.str();
}

std::string sha256_hex_str(const std::string& s) {
    std::vector<uint8_t> in(s.begin(), s.end());
    return sha256_hex(in);
}

uint64_t sha256_u64(const std::vector<uint8_t>& input) {
    auto digest = sha256_bytes(input);
    uint64_t out = 0;
    for (int i = 7; i >= 0; --i) {
        out = (out << 8) | digest[i];
    }
    return out;
}

void append_u16_modq(std::vector<uint8_t>& out, int v, int Q) {
    int x = mod_q(v, Q);
    out.push_back(static_cast<uint8_t>(x & 0xff));
    out.push_back(static_cast<uint8_t>((x >> 8) & 0xff));
}

void append_u64_le(std::vector<uint8_t>& out, uint64_t x) {
    for (int i = 0; i < 8; ++i) {
        out.push_back(static_cast<uint8_t>((x >> (8 * i)) & 0xff));
    }
}

void append_ascii(std::vector<uint8_t>& out, const std::string& s) {
    for (unsigned char ch : s) out.push_back(ch);
}

void append_message(std::vector<uint8_t>& out, const std::string& s) {
    for (unsigned char ch : s) out.push_back(ch);
}

// ------------------------------------------------------------
// 代数核心：负循环卷积
// ------------------------------------------------------------
Poly poly_mul_negacyclic(const Poly& a, const Poly& b, const Params& p) {
    Poly r(p.N, 0);

    for (int i = 0; i < p.N; ++i) {
        if (a[i] == 0) continue;
        for (int j = 0; j < p.N; ++j) {
            if (b[j] == 0) continue;

            long long prod = 1LL * a[i] * b[j];
            int k = i + j;
            if (k < p.N) {
                r[k] = mod_q(r[k] + prod, p.Q);
            } else {
                r[k - p.N] = mod_q(r[k - p.N] - prod, p.Q);
            }
        }
    }
    return r;
}

Poly vec_to_poly(const std::vector<int>& v, const Params& p) {
    Poly poly(p.N, 0);
    for (int i = 0; i < p.N && i < static_cast<int>(v.size()); ++i) {
        poly[i] = v[i];
    }
    return poly;
}

// ------------------------------------------------------------
// OrbitHead
// ------------------------------------------------------------
int phase_cyclic_distance(int a, int b) {
    int d = std::abs(a - b);
    return std::min(d, 4 - d);
}

int lsh_hamming_distance(const OrbitHead& a, const OrbitHead& b) {
    return __builtin_popcountll(a.lsh0 ^ b.lsh0) +
           __builtin_popcountll(a.lsh1 ^ b.lsh1);
}

int phase_total_distance(const OrbitHead& a, const OrbitHead& b) {
    int s = 0;
    int lim = std::min<int>(a.phase.size(), b.phase.size());
    for (int i = 0; i < lim; ++i) {
        s += phase_cyclic_distance(a.phase[i], b.phase[i]);
    }
    return s;
}

OrbitHead build_head(const Poly& w, const Params& p) {
    OrbitHead h;
    h.lsh0 = 0;
    h.lsh1 = 0;
    h.phase.assign(p.NAV_ZONES, 0);

    for (int i = 0; i < p.N; ++i) {
        int v = centered(w[i], p.Q);
        bool bit = ((((v / p.DELTA)) ^ i) & 1) != 0;

        if (i < 64) {
            if (bit) h.lsh0 |= (1ULL << i);
        } else if (i < 128) {
            if (bit) h.lsh1 |= (1ULL << (i - 64));
        }
    }

    int seg_len = std::max(1, p.N / p.NAV_ZONES);
    for (int seg = 0; seg < p.NAV_ZONES; ++seg) {
        int begin = seg * seg_len;
        int end = std::min(p.N, begin + seg_len);
        int sum = 0;
        for (int i = begin; i < end; ++i) {
            sum += centered(w[i], p.Q);
        }
        h.phase[seg] = (sum % 4 + 4) % 4;
    }

    return h;
}

// ------------------------------------------------------------
// Challenge / Bind
// ------------------------------------------------------------
Poly derive_challenge(const OrbitHead& head, const std::string& msg, const Params& p) {
    std::vector<uint8_t> seed_bytes;
    append_ascii(seed_bytes, DOMAIN_CHALLENGE);
    append_ascii(seed_bytes, ARCH_IDENTIFIER);

    append_u64_le(seed_bytes, head.lsh0);
    append_u64_le(seed_bytes, head.lsh1);

    for (int ph : head.phase) {
        seed_bytes.push_back(static_cast<uint8_t>(ph & 0xff));
    }

    append_message(seed_bytes, msg);

    auto digest = sha256_bytes(seed_bytes);

    uint64_t seed = 0;
    for (int i = 0; i < 8; ++i) {
        seed |= (static_cast<uint64_t>(digest[i]) << (8 * i));
    }

    std::mt19937_64 rng(seed);

    Poly c(p.N, 0);
    std::set<int> used;

    while ((int)used.size() < p.CHALLENGE_WT) {
        int pos = static_cast<int>(rng() % p.N);
        if (used.count(pos)) continue;
        used.insert(pos);
        c[pos] = (rng() & 1ULL) ? 1 : (p.Q - 1);
    }

    return c;
}

uint64_t hash_binding(const std::string& msg,
                      const std::vector<int>& z,
                      const Poly& c,
                      const OrbitHead& head,
                      const Params& p) {
    std::vector<uint8_t> bytes;
    append_ascii(bytes, DOMAIN_BIND);
    append_ascii(bytes, ARCH_IDENTIFIER);
    append_message(bytes, msg);

    for (int v : z) append_u16_modq(bytes, v, p.Q);
    for (int v : c) append_u16_modq(bytes, v, p.Q);

    append_u64_le(bytes, head.lsh0);
    append_u64_le(bytes, head.lsh1);
    for (int ph : head.phase) {
        bytes.push_back(static_cast<uint8_t>(ph & 0xff));
    }

    return sha256_u64(bytes);
}

// ------------------------------------------------------------
// KeyGen / Sign / Verify
// ------------------------------------------------------------
KeyPair keygen(std::mt19937_64& rng, const Params& p) {
    KeyPair k;
    k.a.assign(p.N, 0);
    k.s.assign(p.N, 0);
    k.t.assign(p.N, 0);

    std::uniform_int_distribution<int> dist_q(0, p.Q - 1);
    std::uniform_int_distribution<int> dist_s(0, 11);

    for (int i = 0; i < p.N; ++i) {
        k.a[i] = dist_q(rng);
        int r = dist_s(rng);
        if (r == 0) k.s[i] = 1;
        else if (r == 1) k.s[i] = -1;
        else k.s[i] = 0;
    }

    k.t = poly_mul_negacyclic(k.a, k.s, p);
    return k;
}

Signature sign(const std::string& msg, const KeyPair& k, std::mt19937_64& rng, const Params& p) {
    std::uniform_int_distribution<int> dist_y(p.Y_MIN, p.Y_MAX);

    while (true) {
        Poly y(p.N, 0);
        for (int i = 0; i < p.N; ++i) y[i] = dist_y(rng);

        Poly w = poly_mul_negacyclic(k.a, y, p);
        OrbitHead head = build_head(w, p);
        Poly c = derive_challenge(head, msg, p);
        Poly cs = poly_mul_negacyclic(c, k.s, p);

        Signature sig;
        sig.z.resize(p.N);

        int max_abs = 0;
        for (int i = 0; i < p.N; ++i) {
            sig.z[i] = y[i] + centered(cs[i], p.Q);
            max_abs = std::max(max_abs, std::abs(sig.z[i]));
        }

        if (max_abs > p.SIG_BOUND) continue;

        sig.head = head;
        sig.c = c;
        sig.bind_hash = hash_binding(msg, sig.z, sig.c, sig.head, p);
        return sig;
    }
}

VerifyResult verify(const std::string& msg, const KeyPair& k, const Signature& sig, const Params& p) {
    VerifyResult vr;
    vr.recovered_w.assign(p.N, 0);
    vr.recovered_c.assign(p.N, 0);

    vr.z_bound_ok = true;
    for (int v : sig.z) {
        if (std::abs(v) > p.SIG_BOUND) {
            vr.z_bound_ok = false;
            vr.ok = false;
            return vr;
        }
    }

    vr.bind_ok = (hash_binding(msg, sig.z, sig.c, sig.head, p) == sig.bind_hash);
    if (!vr.bind_ok) {
        vr.ok = false;
        return vr;
    }

    Poly z_poly = vec_to_poly(sig.z, p);
    Poly az = poly_mul_negacyclic(k.a, z_poly, p);
    Poly ct = poly_mul_negacyclic(sig.c, k.t, p);

    for (int i = 0; i < p.N; ++i) {
        vr.recovered_w[i] = mod_q(az[i] - ct[i], p.Q);
    }

    vr.recovered_head = build_head(vr.recovered_w, p);
    vr.lsh_dist = lsh_hamming_distance(sig.head, vr.recovered_head);
    vr.phase_dist = phase_total_distance(sig.head, vr.recovered_head);
    vr.head_ok = (vr.lsh_dist == 0 && vr.phase_dist == 0);

    vr.recovered_c = derive_challenge(vr.recovered_head, msg, p);
    vr.chal_ok = true;
    for (int i = 0; i < p.N; ++i) {
        if (vr.recovered_c[i] != sig.c[i]) {
            vr.chal_ok = false;
            break;
        }
    }

    vr.ok = vr.z_bound_ok && vr.bind_ok && vr.head_ok && vr.chal_ok;
    return vr;
}

std::vector<uint8_t> serialize_signature(const Signature& sig, const Params& p) {
    std::vector<uint8_t> out;

    out.push_back('C');
    out.push_back('E');
    out.push_back('H');
    out.push_back('1');
    out.push_back(0x01);

    append_u16_modq(out, p.N, p.Q);
    append_u16_modq(out, p.Q, p.Q);

    append_u16_modq(out, p.N, p.Q);
    for (int v : sig.z) append_u16_modq(out, v, p.Q);

    out.push_back(0x02);
    append_u64_le(out, sig.head.lsh0);
    append_u64_le(out, sig.head.lsh1);

    out.push_back(static_cast<uint8_t>(p.NAV_ZONES));
    for (int ph : sig.head.phase) out.push_back(static_cast<uint8_t>(ph & 0xff));

    append_u16_modq(out, p.N, p.Q);
    for (int v : sig.c) append_u16_modq(out, v, p.Q);

    append_u64_le(out, sig.bind_hash);
    return out;
}

// ------------------------------------------------------------
// Wallet / Address
// ------------------------------------------------------------
std::string derive_address_from_pub(const KeyPair& kp, const Params& p) {
    std::vector<uint8_t> bytes;
    append_ascii(bytes, DOMAIN_ADDRESS);
    append_ascii(bytes, ARCH_IDENTIFIER);

    for (int v : kp.a) append_u16_modq(bytes, v, p.Q);
    for (int v : kp.t) append_u16_modq(bytes, v, p.Q);

    std::string h = sha256_hex(bytes);
    return "ceh1_" + h.substr(0, 40);
}

Wallet create_wallet(const std::string& owner, std::mt19937_64& rng, const Params& p) {
    Wallet w;
    w.owner = owner;
    w.keypair = keygen(rng, p);
    w.address = derive_address_from_pub(w.keypair, p);
    return w;
}

// ------------------------------------------------------------
// Tx / TxID
// ------------------------------------------------------------
std::string tx_payload_string(const Transaction& tx) {
    std::ostringstream oss;
    oss << static_cast<int>(tx.type) << "|"
        << tx.from << "|"
        << tx.to << "|"
        << tx.amount << "|"
        << tx.nonce << "|"
        << tx.timestamp_ms << "|"
        << tx.memo;
    return oss.str();
}

std::string compute_txid(const Transaction& tx, const Params& p) {
    std::vector<uint8_t> bytes;
    append_ascii(bytes, DOMAIN_TXID);
    append_ascii(bytes, ARCH_IDENTIFIER);
    append_message(bytes, tx_payload_string(tx));

    auto sig_bytes = serialize_signature(tx.sig, p);
    bytes.insert(bytes.end(), sig_bytes.begin(), sig_bytes.end());

    return sha256_hex(bytes);
}

Transaction make_signed_transfer(const Wallet& from,
                                 const std::string& to,
                                 uint64_t amount,
                                 uint64_t nonce,
                                 const std::string& memo,
                                 std::mt19937_64& rng,
                                 const Params& p) {
    Transaction tx;
    tx.type = TxType::TRANSFER;
    tx.from = from.address;
    tx.to = to;
    tx.amount = amount;
    tx.nonce = nonce;
    tx.timestamp_ms = now_ms();
    tx.memo = memo;

    std::string payload = tx_payload_string(tx);
    tx.sig = sign(payload, from.keypair, rng, p);
    tx.txid = compute_txid(tx, p);
    return tx;
}

Transaction make_coinbase_tx(const std::string& to, uint64_t amount, uint64_t height) {
    Transaction tx;
    tx.type = TxType::COINBASE;
    tx.from = "COINBASE";
    tx.to = to;
    tx.amount = amount;
    tx.nonce = height;
    tx.timestamp_ms = now_ms();
    tx.memo = "block reward";
    tx.txid = sha256_hex_str("COINBASE|" + to + "|" + std::to_string(amount) + "|" + std::to_string(height));
    return tx;
}

// ------------------------------------------------------------
// Merkle
// ------------------------------------------------------------
std::string merkle_root(std::vector<std::string> hashes) {
    if (hashes.empty()) {
        return sha256_hex_str("EMPTY_MERKLE");
    }

    while (hashes.size() > 1) {
        if (hashes.size() % 2 == 1) hashes.push_back(hashes.back());

        std::vector<std::string> next;
        for (size_t i = 0; i < hashes.size(); i += 2) {
            next.push_back(sha256_hex_str(hashes[i] + hashes[i + 1]));
        }
        hashes = std::move(next);
    }

    return hashes[0];
}

// ------------------------------------------------------------
// Block / PoW
// ------------------------------------------------------------
std::string header_to_string(const BlockHeader& h) {
    std::ostringstream oss;
    oss << h.height << "|"
        << h.prev_hash << "|"
        << h.merkle_root << "|"
        << h.timestamp_ms << "|"
        << h.difficulty << "|"
        << h.nonce;
    return oss.str();
}

bool hash_meets_difficulty(const std::string& hex_hash, uint64_t difficulty) {
    if (difficulty == 0) return true;
    if (difficulty > hex_hash.size()) return false;

    for (uint64_t i = 0; i < difficulty; ++i) {
        if (hex_hash[i] != '0') return false;
    }
    return true;
}

std::string mine_block_hash(BlockHeader& header) {
    while (true) {
        std::string h = sha256_hex_str(header_to_string(header));
        if (hash_meets_difficulty(h, header.difficulty)) return h;
        header.nonce++;
    }
}

// ------------------------------------------------------------
// StateDB
// ------------------------------------------------------------
class StateDB {
public:
    void credit(const std::string& addr, uint64_t amount) {
        states_[addr].balance += amount;
    }

    bool can_spend(const std::string& addr, uint64_t amount, uint64_t nonce) const {
        auto it = states_.find(addr);
        if (it == states_.end()) return false;
        return it->second.balance >= amount && it->second.nonce == nonce;
    }

    bool apply_tx(const Transaction& tx) {
        if (tx.type == TxType::COINBASE) {
            states_[tx.to].balance += tx.amount;
            return true;
        }

        auto it = states_.find(tx.from);
        if (it == states_.end()) return false;
        if (it->second.balance < tx.amount) return false;
        if (it->second.nonce != tx.nonce) return false;

        it->second.balance -= tx.amount;
        it->second.nonce += 1;
        states_[tx.to].balance += tx.amount;
        return true;
    }

    uint64_t balance_of(const std::string& addr) const {
        auto it = states_.find(addr);
        if (it == states_.end()) return 0;
        return it->second.balance;
    }

    uint64_t nonce_of(const std::string& addr) const {
        auto it = states_.find(addr);
        if (it == states_.end()) return 0;
        return it->second.nonce;
    }

    const std::unordered_map<std::string, AccountState>& raw() const {
        return states_;
    }

private:
    std::unordered_map<std::string, AccountState> states_;
};

struct TxVerifyContext {
    const std::unordered_map<std::string, KeyPair>* pub_registry = nullptr;
    const Params* params = nullptr;
};

bool verify_transaction_signature(const Transaction& tx, const TxVerifyContext& ctx) {
    if (tx.type == TxType::COINBASE) return true;

    auto it = ctx.pub_registry->find(tx.from);
    if (it == ctx.pub_registry->end()) return false;

    std::string payload = tx_payload_string(tx);
    auto vr = verify(payload, it->second, tx.sig, *ctx.params);
    return vr.ok;
}

// ------------------------------------------------------------
// Node
// ------------------------------------------------------------
class Node {
public:
    explicit Node(std::string name, const Params& params)
        : name_(std::move(name)), params_(params) {}

    void connect(Node* other) {
        if (other == this) return;
        peers_.push_back(other);
    }

    void set_pub_registry(const std::unordered_map<std::string, KeyPair>* reg) {
        ctx_.pub_registry = reg;
        ctx_.params = &params_;
    }

    void init_genesis(const std::string& treasury, uint64_t amount) {
        if (!chain_.empty()) return;

        Block genesis;
        genesis.header.height = 0;
        genesis.header.prev_hash = "GENESIS";
        genesis.header.timestamp_ms = now_ms();
        genesis.header.difficulty = 1;
        genesis.txs.push_back(make_coinbase_tx(treasury, amount, 0));

        std::vector<std::string> txids;
        for (auto& tx : genesis.txs) txids.push_back(tx.txid);
        genesis.header.merkle_root = merkle_root(txids);
        genesis.block_hash = mine_block_hash(genesis.header);

        chain_.push_back(genesis);
        state_.apply_tx(genesis.txs[0]);
    }

    bool receive_tx(const Transaction& tx) {
        if (seen_txs_.count(tx.txid)) return false;
        seen_txs_.insert(tx.txid);

        if (!verify_transaction_signature(tx, ctx_)) {
            last_reject_reason_ = "signature invalid";
            return false;
        }

        if (tx.type == TxType::TRANSFER && !state_.can_spend(tx.from, tx.amount, tx.nonce)) {
            last_reject_reason_ = "state invalid";
            return false;
        }

        mempool_.push_back(tx);
        gossip_tx(tx);
        return true;
    }

    void mine_one_block(const std::string& miner_addr, uint64_t reward, uint64_t difficulty) {
        Block b;
        b.header.height = chain_.size();
        b.header.prev_hash = chain_.back().block_hash;
        b.header.timestamp_ms = now_ms();
        b.header.difficulty = difficulty;
        b.header.nonce = 0;

        b.txs.push_back(make_coinbase_tx(miner_addr, reward, b.header.height));

        std::vector<Transaction> remain;
        for (const auto& tx : mempool_) {
            if (tx.type == TxType::TRANSFER && state_.can_spend(tx.from, tx.amount, tx.nonce)) {
                b.txs.push_back(tx);
            } else {
                remain.push_back(tx);
            }
        }
        mempool_.swap(remain);

        std::vector<std::string> txids;
        for (const auto& tx : b.txs) txids.push_back(tx.txid);
        b.header.merkle_root = merkle_root(txids);

        auto t0 = std::chrono::high_resolution_clock::now();
        b.block_hash = mine_block_hash(b.header);
        auto t1 = std::chrono::high_resolution_clock::now();
        last_mine_ms_ = std::chrono::duration<double, std::milli>(t1 - t0).count();

        bool all_ok = true;
        for (const auto& tx : b.txs) {
            if (!state_.apply_tx(tx)) {
                all_ok = false;
                break;
            }
        }

        if (!all_ok) {
            last_reject_reason_ = "block apply failed";
            return;
        }

        chain_.push_back(b);
    }

    const std::vector<Block>& chain() const { return chain_; }
    const std::vector<Transaction>& mempool() const { return mempool_; }
    const StateDB& state() const { return state_; }
    const std::string& name() const { return name_; }
    double last_mine_ms() const { return last_mine_ms_; }
    const std::string& last_reject_reason() const { return last_reject_reason_; }

private:
    void gossip_tx(const Transaction& tx) {
        for (auto* p : peers_) {
            if (p) p->receive_tx_no_gossip(tx);
        }
    }

    void receive_tx_no_gossip(const Transaction& tx) {
        if (seen_txs_.count(tx.txid)) return;
        seen_txs_.insert(tx.txid);

        if (!verify_transaction_signature(tx, ctx_)) return;
        if (tx.type == TxType::TRANSFER && !state_.can_spend(tx.from, tx.amount, tx.nonce)) return;

        mempool_.push_back(tx);
    }

private:
    std::string name_;
    Params params_;
    TxVerifyContext ctx_;
    std::vector<Node*> peers_;
    std::vector<Block> chain_;
    std::vector<Transaction> mempool_;
    std::set<std::string> seen_txs_;
    StateDB state_;
    double last_mine_ms_ = 0.0;
    std::string last_reject_reason_;
};

// ------------------------------------------------------------
// Benchmark / Attack / Basin
// ------------------------------------------------------------
BenchmarkResult run_benchmark(const Params& p) {
    BenchmarkResult br;
    std::mt19937_64 rng(static_cast<uint64_t>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count()
    ));
    std::string msg = "CEH_ORBIT_BENCHMARK_MESSAGE";

    auto t0 = std::chrono::high_resolution_clock::now();
    KeyPair k = keygen(rng, p);
    auto t1 = std::chrono::high_resolution_clock::now();
    Signature sig = sign(msg, k, rng, p);
    auto t2 = std::chrono::high_resolution_clock::now();
    auto vr = verify(msg, k, sig, p);
    auto t3 = std::chrono::high_resolution_clock::now();

    br.keygen_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    br.sign_ms = std::chrono::duration<double, std::milli>(t2 - t1).count();
    br.verify_ms = std::chrono::duration<double, std::milli>(t3 - t2).count();
    br.serialized_size = static_cast<int>(serialize_signature(sig, p).size());

    (void)vr;
    return br;
}

AttackResult random_forgery_attack(const Params& p) {
    AttackResult ar;
    ar.rounds = p.attack_rounds;
    ar.success = 0;

    std::mt19937_64 rng(1);
    std::string msg = "CEH_ORBIT_ATTACK_MESSAGE";
    KeyPair k = keygen(rng, p);
    Signature base = sign(msg, k, rng, p);

    for (int i = 0; i < ar.rounds; ++i) {
        Signature fake = base;
        int idx = static_cast<int>(rng() % p.N);
        fake.z[idx] += (rng() & 1ULL) ? 1 : -1;
        fake.bind_hash = hash_binding(msg, fake.z, fake.c, fake.head, p);

        auto vr = verify(msg, k, fake, p);
        if (vr.ok) ar.success++;
    }
    return ar;
}

double head_collision_rate(const Params& p) {
    std::mt19937_64 rng(2);
    std::string msg = "CEH_ORBIT_COLLISION_MESSAGE";
    KeyPair k = keygen(rng, p);

    std::set<std::pair<uint64_t, uint64_t>> seen;
    int collisions = 0;

    for (int i = 0; i < p.collision_trials; ++i) {
        Signature sig = sign(msg, k, rng, p);
        auto hh = std::make_pair(sig.head.lsh0, sig.head.lsh1);
        if (seen.count(hh)) collisions++;
        else seen.insert(hh);
    }

    return static_cast<double>(collisions) / std::max(1, p.collision_trials);
}

std::vector<BasinPoint> basin_scan_single_point(const Params& p) {
    std::mt19937_64 rng(3);
    std::string msg = "CEH_ORBIT_BASIN_MESSAGE";
    KeyPair k = keygen(rng, p);
    Signature sig = sign(msg, k, rng, p);

    std::vector<int> amps = {1, 2, 5, 9, 10};
    std::vector<BasinPoint> out;

    for (int amp : amps) {
        BasinPoint bp;
        bp.amplitude = amp;
        bp.rounds = p.basin_rounds;
        bp.pass = 0;

        for (int i = 0; i < bp.rounds; ++i) {
            Signature fake = sig;
            int idx = static_cast<int>(rng() % p.N);
            int signv = (rng() & 1ULL) ? 1 : -1;
            fake.z[idx] += signv * amp;
            fake.bind_hash = hash_binding(msg, fake.z, fake.c, fake.head, p);

            auto vr = verify(msg, k, fake, p);
            if (vr.ok) bp.pass++;
        }

        out.push_back(bp);
    }
    return out;
}

} // namespace CEH_Orbit_Blockchain_FireSeed

// ============================================================
// 二、UI 小组件
// ============================================================
class CardFrame : public QFrame {
public:
    explicit CardFrame(const QString& title, QWidget* parent = nullptr) : QFrame(parent) {
        setStyleSheet(
            "QFrame{background:#1b1d1f;border:1px solid #4c4f52;border-radius:10px;}"
            "QLabel{color:#dadde1;}"
        );
        auto* lay = new QVBoxLayout(this);
        lay->setContentsMargins(8, 8, 8, 8);
        lay->setSpacing(6);

        auto* titleLabel = new QLabel(title, this);
        titleLabel->setFixedHeight(28);
        titleLabel->setStyleSheet("QLabel{font-size:13px;font-weight:bold;color:#f1f3f5;}");
        lay->addWidget(titleLabel);

        body = new QWidget(this);
        bodyLay = new QVBoxLayout(body);
        bodyLay->setContentsMargins(0,0,0,0);
        bodyLay->setSpacing(6);
        lay->addWidget(body, 1);
    }

    QWidget* body = nullptr;
    QVBoxLayout* bodyLay = nullptr;
};
namespace Core = CEH_Orbit_Blockchain_FireSeed;
class MainWindow : public QMainWindow {
public:
    MainWindow() {
        setupUi();
        resetEngine();
        updateAllViews();
    }

private:

    Core::Params params;

    std::mt19937_64 rng{20260326};

    Core::Wallet alice;
    Core::Wallet bob;
    Core::Wallet miner;
    std::unordered_map<std::string, Core::KeyPair> registry;

    std::unique_ptr<Core::Node> nodeA;
    std::unique_ptr<Core::Node> nodeB;

    Core::Signature lastSig;
    Core::VerifyResult lastVerify;
    Core::BenchmarkResult lastBench;
    Core::AttackResult lastAttack;
    std::vector<Core::BasinPoint> lastBasin;
    double lastCollisionRate = 0.0;
    std::vector<int> lastW;
    std::vector<int> lastZ;

    // 左侧控件
    QSpinBox *spN=nullptr,*spQ=nullptr,*spZones=nullptr,*spDelta=nullptr,*spBound=nullptr,*spWeight=nullptr,*spYMin=nullptr,*spYMax=nullptr;
    QSpinBox *spGenesis=nullptr,*spReward=nullptr,*spDiff=nullptr,*spAtkRounds=nullptr,*spCollTrials=nullptr,*spBasinRounds=nullptr;
    QLineEdit *editMemo=nullptr,*editAmount=nullptr,*editSeed=nullptr;
    QTextEdit *logEdit=nullptr;
    QLabel *statusLabel=nullptr;

    // 表格
    QTableWidget *walletTable=nullptr;
    QTableWidget *mempoolTable=nullptr;
    QTableWidget *chainTable=nullptr;

    // 图表
    QChartView *chartLSH=nullptr;
    QChartView *chartPhase=nullptr;
    QChartView *chartZ=nullptr;
    QChartView *chartW=nullptr;
    QChartView *chartBasin=nullptr;

    QTimer *realtimeTimer=nullptr;

private:
    void setupUi() {
        setWindowTitle("CEH-Orbit 区块链火种版");
        resize(1680, 980);

        auto* central = new QWidget(this);
        setCentralWidget(central);

        central->setStyleSheet(R"(
            QWidget { background:#26282a; color:#dadde1; font-size:12px; }
            QLineEdit, QSpinBox, QTextEdit, QTableWidget {
                background:#1b1d1f; color:#dadde1; border:1px solid #5f6163; border-radius:6px;
            }
            QPushButton {
                background:#3a3d40; color:#dadde1; border:1px solid #5f6163; border-radius:6px; padding:6px 10px;
            }
            QPushButton:hover { background:#4b4f53; }
            QHeaderView::section {
                background:#2b2d2f; color:#dadde1; border:1px solid #4c4f52; padding:4px;
            }
            QLabel { color:#dadde1; }
        )");

        auto* root = new QHBoxLayout(central);
        root->setContentsMargins(6,6,6,6);
        root->setSpacing(6);

        // 左侧
        auto* leftScroll = new QScrollArea(this);
        leftScroll->setWidgetResizable(true);
        leftScroll->setFixedWidth(430);

        auto* leftContent = new QWidget(this);
        auto* leftLay = new QVBoxLayout(leftContent);
        leftLay->setContentsMargins(0,0,0,0);
        leftLay->setSpacing(6);
        leftScroll->setWidget(leftContent);

        // 参数卡片
        auto* paramCard = new CardFrame("参数面板", this);
        auto* form = new QFormLayout();
        form->setSpacing(6);

        spN = mkSpin(64, 512, 128);
        spQ = mkSpin(257, 65535, 3329);
        spZones = mkSpin(4, 32, 16);
        spDelta = mkSpin(1, 256, 32);
        spBound = mkSpin(1, 2048, 260);
        spWeight = mkSpin(1, 64, 8);
        spYMin = mkSpin(-5000, 0, -200);
        spYMax = mkSpin(0, 5000, 200);

        spGenesis = mkSpin(1, 1000000000, 1000);
        spReward = mkSpin(1, 1000000000, 50);
        spDiff = mkSpin(0, 6, 2);
        spAtkRounds = mkSpin(10, 200000, 10000);
        spCollTrials = mkSpin(10, 200000, 2000);
        spBasinRounds = mkSpin(10, 100000, 500);

        editMemo = new QLineEdit("Alice pays Bob", this);
        editAmount = new QLineEdit("150", this);
        editSeed = new QLineEdit("20260326", this);

        form->addRow("N", spN);
        form->addRow("Q", spQ);
        form->addRow("NAV_ZONES", spZones);
        form->addRow("DELTA", spDelta);
        form->addRow("SIG_BOUND", spBound);
        form->addRow("CHALLENGE_WT", spWeight);
        form->addRow("Y_MIN", spYMin);
        form->addRow("Y_MAX", spYMax);
        form->addRow("Genesis Balance", spGenesis);
        form->addRow("Mining Reward", spReward);
        form->addRow("PoW Difficulty", spDiff);
        form->addRow("Attack Rounds", spAtkRounds);
        form->addRow("Collision Trials", spCollTrials);
        form->addRow("Basin Rounds", spBasinRounds);
        form->addRow("Transfer Amount", editAmount);
        form->addRow("Transfer Memo", editMemo);
        form->addRow("Random Seed", editSeed);

        paramCard->bodyLay->addLayout(form);

        auto* btnRow1 = new QHBoxLayout();
        auto* btnApply = new QPushButton("应用参数 / 重置链", this);
        auto* btnKey = new QPushButton("重建钱包", this);
        btnRow1->addWidget(btnApply);
        btnRow1->addWidget(btnKey);
        paramCard->bodyLay->addLayout(btnRow1);

        auto* btnRow2 = new QHBoxLayout();
        auto* btnTx = new QPushButton("签名并发送交易", this);
        auto* btnMine = new QPushButton("挖一个块", this);
        btnRow2->addWidget(btnTx);
        btnRow2->addWidget(btnMine);
        paramCard->bodyLay->addLayout(btnRow2);

        auto* btnRow3 = new QHBoxLayout();
        auto* btnBench = new QPushButton("协议 Benchmark", this);
        auto* btnAtk = new QPushButton("攻击测试", this);
        btnRow3->addWidget(btnBench);
        btnRow3->addWidget(btnAtk);
        paramCard->bodyLay->addLayout(btnRow3);

        auto* btnRow4 = new QHBoxLayout();
        auto* btnBasin = new QPushButton("接受域扫描", this);
        auto* btnRealtime = new QPushButton("开始/停止实时", this);
        btnRow4->addWidget(btnBasin);
        btnRow4->addWidget(btnRealtime);
        paramCard->bodyLay->addLayout(btnRow4);

        leftLay->addWidget(paramCard);

        // 状态卡片
        auto* statusCard = new CardFrame("状态总览", this);
        statusLabel = new QLabel(this);
        statusLabel->setWordWrap(true);
        statusLabel->setStyleSheet("QLabel{background:#1b1d1f;border:1px solid #5f6163;border-radius:6px;padding:8px;}");
        statusCard->bodyLay->addWidget(statusLabel);
        leftLay->addWidget(statusCard);

        // 钱包卡片
        auto* walletCard = new CardFrame("钱包 / 余额", this);
        walletTable = new QTableWidget(0, 4, this);
        walletTable->setHorizontalHeaderLabels({"Owner","Address","Balance","Nonce"});
        walletTable->horizontalHeader()->setStretchLastSection(true);
        walletTable->verticalHeader()->setVisible(false);
        walletCard->bodyLay->addWidget(walletTable);
        leftLay->addWidget(walletCard);

        // mempool 卡片
        auto* memCard = new CardFrame("Mempool", this);
        mempoolTable = new QTableWidget(0, 5, this);
        mempoolTable->setHorizontalHeaderLabels({"TXID","From","To","Amount","Nonce"});
        mempoolTable->horizontalHeader()->setStretchLastSection(true);
        mempoolTable->verticalHeader()->setVisible(false);
        memCard->bodyLay->addWidget(mempoolTable);
        leftLay->addWidget(memCard);

        // chain 卡片
        auto* chainCard = new CardFrame("Chain", this);
        chainTable = new QTableWidget(0, 6, this);
        chainTable->setHorizontalHeaderLabels({"Height","Hash","Prev","Merkle","TXs","Diff"});
        chainTable->horizontalHeader()->setStretchLastSection(true);
        chainTable->verticalHeader()->setVisible(false);
        chainCard->bodyLay->addWidget(chainTable);
        leftLay->addWidget(chainCard);

        // 日志
        auto* logCard = new CardFrame("调试日志", this);
        logEdit = new QTextEdit(this);
        logEdit->setReadOnly(true);
        logEdit->setMinimumHeight(260);
        logCard->bodyLay->addWidget(logEdit);
        leftLay->addWidget(logCard, 1);

        // 右侧
        auto* right = new QWidget(this);
        auto* rightLay = new QVBoxLayout(right);
        rightLay->setContentsMargins(0,0,0,0);
        rightLay->setSpacing(6);

        auto* rowTop = new QHBoxLayout();
        chartLSH = makeChartView("LSH 128bit");
        chartPhase = makeChartView("Phase");
        rowTop->addWidget(wrapChart("LSH 渲染", chartLSH), 1);
        rowTop->addWidget(wrapChart("Phase 渲染", chartPhase), 1);
        rightLay->addLayout(rowTop, 1);

        auto* rowMid = new QHBoxLayout();
        chartZ = makeChartView("EncodedOrbit_Z");
        chartW = makeChartView("RecoveredOrbit_W");
        rowMid->addWidget(wrapChart("Z 波形", chartZ), 1);
        rowMid->addWidget(wrapChart("Recovered W 波形", chartW), 1);
        rightLay->addLayout(rowMid, 1);

        chartBasin = makeChartView("Acceptance Basin");
        rightLay->addWidget(wrapChart("接受域 / Basin", chartBasin), 1);

        root->addWidget(leftScroll, 0);
        root->addWidget(right, 1);

        connect(btnApply, &QPushButton::clicked, this, [this]{ applyParamsAndReset(); });
        connect(btnKey, &QPushButton::clicked, this, [this]{ rebuildWalletsOnly(); });
        connect(btnTx, &QPushButton::clicked, this, [this]{ sendSignedTransfer(); });
        connect(btnMine, &QPushButton::clicked, this, [this]{ mineOneBlock(); });
        connect(btnBench, &QPushButton::clicked, this, [this]{ runBenchmarkNow(); });
        connect(btnAtk, &QPushButton::clicked, this, [this]{ runAttackNow(); });
        connect(btnBasin, &QPushButton::clicked, this, [this]{ runBasinNow(); });
        connect(btnRealtime, &QPushButton::clicked, this, [this]{ toggleRealtime(); });

        realtimeTimer = new QTimer(this);
        realtimeTimer->setInterval(1200);
        connect(realtimeTimer, &QTimer::timeout, this, [this]{
            realtimeStep();
        });
    }

    QSpinBox* mkSpin(int lo, int hi, int val) {
        auto* sp = new QSpinBox(this);
        sp->setRange(lo, hi);
        sp->setValue(val);
        return sp;
    }

    CardFrame* wrapChart(const QString& title, QChartView* view) {
        auto* card = new CardFrame(title, this);
        view->setMinimumHeight(260);
        card->bodyLay->addWidget(view);
        return card;
    }

    QChartView* makeChartView(const QString& title) {
        auto* chart = new QChart();
        chart->setTitle(title);
        chart->legend()->hide();
        chart->setBackgroundBrush(QColor("#151719"));
        chart->setTitleBrush(QBrush(QColor("#dadde1")));

        auto* view = new QChartView(chart, this);
        view->setRenderHint(QPainter::Antialiasing);
        view->setStyleSheet("background:#151719;border:none;");
        return view;
    }

    void resetEngine() {
        rng.seed(currentSeed());

        alice = Core::create_wallet("Alice", rng, params);
        bob   = Core::create_wallet("Bob", rng, params);
        miner = Core::create_wallet("Miner", rng, params);

        registry.clear();
        registry[alice.address] = alice.keypair;
        registry[bob.address]   = bob.keypair;
        registry[miner.address] = miner.keypair;

        nodeA = std::make_unique<Core::Node>("Node-A", params);
        nodeB = std::make_unique<Core::Node>("Node-B", params);

        nodeA->connect(nodeB.get());
        nodeB->connect(nodeA.get());

        nodeA->set_pub_registry(&registry);
        nodeB->set_pub_registry(&registry);

        nodeA->init_genesis(alice.address, params.genesis_balance);
        nodeB->init_genesis(alice.address, params.genesis_balance);

        lastSig = Core::Signature();
        lastVerify = Core::VerifyResult();
        lastBench = Core::BenchmarkResult();
        lastAttack = Core::AttackResult();
        lastBasin.clear();
        lastCollisionRate = 0.0;
        lastW.clear();
        lastZ.clear();

        appendLog("已重置 Fire Seed 引擎。");
    }

    uint64_t currentSeed() const {
        bool ok = false;
        uint64_t seed = editSeed->text().toULongLong(&ok);
        if (!ok) return 20260326ULL;
        return seed;
    }

    void applyParamsFromUi() {
        params.N = spN->value();
        if (params.N < 64) params.N = 64;
        if (params.N > 128) params.N = 128; // 当前 LSH 只做 128bit，两块 uint64
        params.Q = spQ->value();
        params.NAV_ZONES = spZones->value();
        if (params.N % params.NAV_ZONES != 0) {
            // 保持整除，更稳
            params.NAV_ZONES = 16;
            spZones->setValue(16);
        }
        params.DELTA = spDelta->value();
        params.SIG_BOUND = spBound->value();
        params.CHALLENGE_WT = std::min(spWeight->value(), params.N);
        params.Y_MIN = spYMin->value();
        params.Y_MAX = spYMax->value();
        if (params.Y_MIN >= params.Y_MAX) {
            params.Y_MIN = -200;
            params.Y_MAX = 200;
            spYMin->setValue(params.Y_MIN);
            spYMax->setValue(params.Y_MAX);
        }

        params.genesis_balance = spGenesis->value();
        params.mining_reward = spReward->value();
        params.pow_difficulty = spDiff->value();
        params.attack_rounds = spAtkRounds->value();
        params.collision_trials = spCollTrials->value();
        params.basin_rounds = spBasinRounds->value();
    }

    void applyParamsAndReset() {
        applyParamsFromUi();
        resetEngine();
        updateAllViews();
    }

    void rebuildWalletsOnly() {
        applyParamsFromUi();
        resetEngine();
        updateAllViews();
    }

    void sendSignedTransfer() {
        applyParamsFromUi();

        bool okAmt = false;
        uint64_t amount = editAmount->text().toULongLong(&okAmt);
        if (!okAmt || amount == 0) {
            appendLog("发送交易失败：金额非法。");
            return;
        }

        uint64_t alice_nonce = nodeA->state().nonce_of(alice.address);
        Core::Transaction tx = Core::make_signed_transfer(
            alice,
            bob.address,
            amount,
            alice_nonce,
            editMemo->text().toStdString(),
            rng,
            params
        );

        std::string payload = Core::tx_payload_string(tx);
        lastSig = tx.sig;
        lastVerify = Core::verify(payload, alice.keypair, tx.sig, params);
        lastW = lastVerify.recovered_w;
        lastZ = lastSig.z;

        bool accepted = nodeA->receive_tx(tx);
        appendLog(QString("发送交易：txid=%1 amount=%2 result=%3")
                  .arg(QString::fromStdString(tx.txid.substr(0, 20)))
                  .arg(amount)
                  .arg(accepted ? "ACCEPT" : "REJECT"));

        if (!accepted) {
            appendLog(QString("拒绝原因：%1").arg(QString::fromStdString(nodeA->last_reject_reason())));
        }

        updateAllViews();
    }

    void mineOneBlock() {
        applyParamsFromUi();
        nodeA->mine_one_block(miner.address, params.mining_reward, params.pow_difficulty);
        appendLog(QString("Node-A 挖块完成，耗时 %1 ms").arg(QString::number(nodeA->last_mine_ms(), 'f', 3)));
        updateAllViews();
    }

    void runBenchmarkNow() {
        applyParamsFromUi();
        lastBench = Core::run_benchmark(params);
        appendLog(QString("Benchmark: KeyGen=%1ms Sign=%2ms Verify=%3ms Size=%4B")
                  .arg(QString::number(lastBench.keygen_ms, 'f', 4))
                  .arg(QString::number(lastBench.sign_ms, 'f', 4))
                  .arg(QString::number(lastBench.verify_ms, 'f', 4))
                  .arg(lastBench.serialized_size));
        updateAllViews();
    }

    void runAttackNow() {
        applyParamsFromUi();
        lastAttack = Core::random_forgery_attack(params);
        lastCollisionRate = Core::head_collision_rate(params);
        appendLog(QString("攻击测试：%1/%2 success, collision=%3")
                  .arg(lastAttack.success)
                  .arg(lastAttack.rounds)
                  .arg(QString::number(lastCollisionRate, 'f', 6)));
        updateAllViews();
    }

    void runBasinNow() {
        applyParamsFromUi();
        lastBasin = Core::basin_scan_single_point(params);
        appendLog("接受域扫描已完成。");
        updateAllViews();
    }

    void toggleRealtime() {
        if (realtimeTimer->isActive()) {
            realtimeTimer->stop();
            appendLog("已停止实时模拟。");
        } else {
            realtimeTimer->start();
            appendLog("已开始实时模拟。");
        }
    }

    void realtimeStep() {
        // 实时模式：随机发送交易 / 随机挖块
        int r = QRandomGenerator::global()->bounded(100);
        if (r < 65) {
            sendSignedTransfer();
        } else {
            mineOneBlock();
        }

        if (QRandomGenerator::global()->bounded(100) < 25) {
            runBenchmarkNow();
        }
        if (QRandomGenerator::global()->bounded(100) < 20) {
            runAttackNow();
        }
        if (QRandomGenerator::global()->bounded(100) < 20) {
            runBasinNow();
        }
    }

    void updateAllViews() {
        updateStatus();
        updateWalletTable();
        updateMempoolTable();
        updateChainTable();
        updateLSHChart();
        updatePhaseChart();
        updateZChart();
        updateWChart();
        updateBasinChart();
    }

    void updateStatus() {
        QString s;
        s += QString("协议：%1\n").arg(Core::PROTOCOL_NAME);
        s += QString("ARCH：%1\n").arg(Core::ARCH_IDENTIFIER);
        s += QString("Alice：%1\n").arg(QString::fromStdString(alice.address));
        s += QString("Bob：%1\n").arg(QString::fromStdString(bob.address));
        s += QString("Miner：%1\n").arg(QString::fromStdString(miner.address));
        s += QString("链高度(Node-A)：%1\n").arg((int)nodeA->chain().size() - 1);
        s += QString("Mempool(Node-A)：%1\n").arg((int)nodeA->mempool().size());

        if (!lastSig.z.empty()) {
            s += QString("最近 Verify：%1\n").arg(lastVerify.ok ? "PASS" : "FAIL");
            s += QString("LSH 距离：%1\n").arg(lastVerify.lsh_dist);
            s += QString("Phase 偏差：%1\n").arg(lastVerify.phase_dist);
            s += QString("Challenge 自洽：%1\n").arg(lastVerify.chal_ok ? "YES" : "NO");
            s += QString("签名字节数：%1\n").arg((int)Core::serialize_signature(lastSig, params).size());
        }

        if (lastBench.serialized_size > 0) {
            s += QString("Bench Sign：%1 ms\n").arg(QString::number(lastBench.sign_ms, 'f', 4));
            s += QString("Bench Verify：%1 ms\n").arg(QString::number(lastBench.verify_ms, 'f', 4));
        }

        if (lastAttack.rounds > 0) {
            s += QString("随机伪造：%1/%2\n").arg(lastAttack.success).arg(lastAttack.rounds);
            s += QString("碰撞率：%1\n").arg(QString::number(lastCollisionRate, 'f', 6));
        }

        statusLabel->setText(s);
    }

    void updateWalletTable() {
        walletTable->setRowCount(3);
        fillWalletRow(0, "Alice", alice.address, nodeA->state().balance_of(alice.address), nodeA->state().nonce_of(alice.address));
        fillWalletRow(1, "Bob",   bob.address,   nodeA->state().balance_of(bob.address),   nodeA->state().nonce_of(bob.address));
        fillWalletRow(2, "Miner", miner.address, nodeA->state().balance_of(miner.address), nodeA->state().nonce_of(miner.address));
        walletTable->resizeColumnsToContents();
    }

    void fillWalletRow(int row, const QString& owner, const std::string& addr, uint64_t bal, uint64_t nonce) {
        walletTable->setItem(row, 0, new QTableWidgetItem(owner));
        walletTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(addr.substr(0, 22))));
        walletTable->setItem(row, 2, new QTableWidgetItem(QString::number((qulonglong)bal)));
        walletTable->setItem(row, 3, new QTableWidgetItem(QString::number((qulonglong)nonce)));
    }

    void updateMempoolTable() {
        const auto& mp = nodeA->mempool();
        mempoolTable->setRowCount((int)mp.size());
        for (int i = 0; i < (int)mp.size(); ++i) {
            mempoolTable->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(mp[i].txid.substr(0, 18))));
            mempoolTable->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(mp[i].from.substr(0, 18))));
            mempoolTable->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(mp[i].to.substr(0, 18))));
            mempoolTable->setItem(i, 3, new QTableWidgetItem(QString::number((qulonglong)mp[i].amount)));
            mempoolTable->setItem(i, 4, new QTableWidgetItem(QString::number((qulonglong)mp[i].nonce)));
        }
        mempoolTable->resizeColumnsToContents();
    }

    void updateChainTable() {
        const auto& chain = nodeA->chain();
        chainTable->setRowCount((int)chain.size());
        for (int i = 0; i < (int)chain.size(); ++i) {
            chainTable->setItem(i, 0, new QTableWidgetItem(QString::number((qulonglong)chain[i].header.height)));
            chainTable->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(chain[i].block_hash.substr(0, 18))));
            chainTable->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(chain[i].header.prev_hash.substr(0, 18))));
            chainTable->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(chain[i].header.merkle_root.substr(0, 18))));
            chainTable->setItem(i, 4, new QTableWidgetItem(QString::number((int)chain[i].txs.size())));
            chainTable->setItem(i, 5, new QTableWidgetItem(QString::number((qulonglong)chain[i].header.difficulty)));
        }
        chainTable->resizeColumnsToContents();
    }

    void updateLSHChart() {
        auto* chart = new QChart();
        chart->setTitle("LSH 128bit 实时渲染");
        chart->legend()->hide();
        chart->setBackgroundBrush(QColor("#151719"));
        chart->setTitleBrush(QBrush(QColor("#dadde1")));

        auto* s0 = new QScatterSeries();
        s0->setMarkerSize(8.0);

        uint64_t l0 = lastSig.head.lsh0;
        uint64_t l1 = lastSig.head.lsh1;

        for (int i = 0; i < 64; ++i) {
            int bit = ((l0 >> i) & 1ULL) ? 1 : 0;
            s0->append(i, bit);
        }
        for (int i = 0; i < 64; ++i) {
            int bit = ((l1 >> i) & 1ULL) ? 1 : 0;
            s0->append(64 + i, bit);
        }

        chart->addSeries(s0);

        auto* axX = new QValueAxis();
        axX->setRange(0, 127);
        axX->setLabelsColor(QColor("#dadde1"));
        axX->setGridLineColor(QColor("#4c4f52"));

        auto* axY = new QValueAxis();
        axY->setRange(-0.2, 1.2);
        axY->setLabelsColor(QColor("#dadde1"));
        axY->setGridLineColor(QColor("#4c4f52"));

        chart->addAxis(axX, Qt::AlignBottom);
        chart->addAxis(axY, Qt::AlignLeft);
        s0->attachAxis(axX);
        s0->attachAxis(axY);

        chartLSH->setChart(chart);
    }

    void updatePhaseChart() {
        auto* chart = new QChart();
        chart->setTitle("Phase 分段柱状图");
        chart->legend()->hide();
        chart->setBackgroundBrush(QColor("#151719"));
        chart->setTitleBrush(QBrush(QColor("#dadde1")));

        auto* set0 = new QBarSet("phase");
        for (int ph : lastSig.head.phase) {
            *set0 << ph;
        }

        auto* series = new QBarSeries();
        series->append(set0);
        chart->addSeries(series);

        QStringList cats;
        for (int i = 0; i < (int)lastSig.head.phase.size(); ++i) {
            cats << QString::number(i);
        }

        auto* axX = new QBarCategoryAxis();
        axX->append(cats);
        axX->setLabelsColor(QColor("#dadde1"));

        auto* axY = new QValueAxis();
        axY->setRange(0, 4);
        axY->setLabelsColor(QColor("#dadde1"));
        axY->setGridLineColor(QColor("#4c4f52"));

        chart->addAxis(axX, Qt::AlignBottom);
        chart->addAxis(axY, Qt::AlignLeft);
        series->attachAxis(axX);
        series->attachAxis(axY);

        chartPhase->setChart(chart);
    }

    void updateZChart() {
        auto* chart = new QChart();
        chart->setTitle("EncodedOrbit_Z 波形");
        chart->legend()->hide();
        chart->setBackgroundBrush(QColor("#151719"));
        chart->setTitleBrush(QBrush(QColor("#dadde1")));

        auto* s = new QLineSeries();
        for (int i = 0; i < (int)lastZ.size(); ++i) {
            s->append(i, lastZ[i]);
        }

        chart->addSeries(s);

        auto* axX = new QValueAxis();
        axX->setRange(0, std::max(1, params.N - 1));
        axX->setLabelsColor(QColor("#dadde1"));
        axX->setGridLineColor(QColor("#4c4f52"));

        auto* axY = new QValueAxis();
        int ymax = std::max(10, params.SIG_BOUND);
        axY->setRange(-ymax, ymax);
        axY->setLabelsColor(QColor("#dadde1"));
        axY->setGridLineColor(QColor("#4c4f52"));

        chart->addAxis(axX, Qt::AlignBottom);
        chart->addAxis(axY, Qt::AlignLeft);
        s->attachAxis(axX);
        s->attachAxis(axY);

        chartZ->setChart(chart);
    }

    void updateWChart() {
        auto* chart = new QChart();
        chart->setTitle("RecoveredOrbit_W 波形");
        chart->legend()->hide();
        chart->setBackgroundBrush(QColor("#151719"));
        chart->setTitleBrush(QBrush(QColor("#dadde1")));

        auto* s = new QLineSeries();
        for (int i = 0; i < (int)lastW.size(); ++i) {
            s->append(i, lastW[i]);
        }

        chart->addSeries(s);

        auto* axX = new QValueAxis();
        axX->setRange(0, std::max(1, params.N - 1));
        axX->setLabelsColor(QColor("#dadde1"));
        axX->setGridLineColor(QColor("#4c4f52"));

        auto* axY = new QValueAxis();
        axY->setRange(0, std::max(10, params.Q));
        axY->setLabelsColor(QColor("#dadde1"));
        axY->setGridLineColor(QColor("#4c4f52"));

        chart->addAxis(axX, Qt::AlignBottom);
        chart->addAxis(axY, Qt::AlignLeft);
        s->attachAxis(axX);
        s->attachAxis(axY);

        chartW->setChart(chart);
    }

    void updateBasinChart() {
        auto* chart = new QChart();
        chart->setTitle("Acceptance Basin / 单点扰动 PASS 率");
        chart->legend()->hide();
        chart->setBackgroundBrush(QColor("#151719"));
        chart->setTitleBrush(QBrush(QColor("#dadde1")));

        auto* s = new QLineSeries();
        for (const auto& bp : lastBasin) {
            double rate = bp.rounds > 0 ? (double)bp.pass / bp.rounds : 0.0;
            s->append(bp.amplitude, rate);
        }

        chart->addSeries(s);

        auto* axX = new QValueAxis();
        axX->setRange(0, 12);
        axX->setLabelsColor(QColor("#dadde1"));
        axX->setGridLineColor(QColor("#4c4f52"));

        auto* axY = new QValueAxis();
        axY->setRange(0.0, 1.0);
        axY->setLabelsColor(QColor("#dadde1"));
        axY->setGridLineColor(QColor("#4c4f52"));

        chart->addAxis(axX, Qt::AlignBottom);
        chart->addAxis(axY, Qt::AlignLeft);
        s->attachAxis(axX);
        s->attachAxis(axY);

        chartBasin->setChart(chart);
    }

    void appendLog(const QString& s) {
        logEdit->append(QString("[%1] %2")
                        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
                        .arg(s));
    }
};

// ============================================================
// 三、Main
// ============================================================
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("CEH-Orbit Blockchain Fire Seed V1");
    app.setOrganizationName("ChenEnhua");

    MainWindow w;
    w.show();

    return app.exec();
}