# CEH-Orbit Blockchain Fire Seed

A minimal end-to-end research prototype that extends the original **CEH-Orbit** project into a blockchain-oriented application layer.

This repository should be understood as an **application-oriented extension** of the original CEH-Orbit work at:

`https://github.com/chenenhua/CEH-orbit`

It does **not** replace the original CEH-Orbit repository. Instead, it demonstrates how the CEH-Orbit signing and orbit-consistency verification ideas can be embedded into a minimal blockchain pipeline.

## 1. Project Positioning

CEH-Orbit Blockchain Fire Seed is:

- a research prototype
- a system-level demonstration
- a minimal blockchain application of CEH-Orbit
- a seed for future cryptographic, system, and blockchain research

It is **not**:

- a production blockchain
- a formally proven cryptographic standard
- a drop-in replacement for Bitcoin or Ethereum
- an audited security product

## 2. Relationship to the Original CEH-Orbit Project

The original CEH-Orbit repository focuses on:

- orbit-based signature/authentication concepts
- orbit mapping
- orbit locking
- signature verification experiments
- protocol-level design and discussion

This blockchain fire-seed version builds on that foundation and adds:

- wallets and addresses
- transactions and transaction IDs
- Merkle root construction
- block and block header structures
- simplified proof-of-work
- account-based state transitions
- local gossip simulation
- Qt-based interactive visualization and parameter tuning

In short:

> Original CEH-Orbit = protocol/core idea  
> Blockchain Fire Seed = one possible application path

## 3. Current Included Components

### Cryptographic Layer
- CEH-Orbit KeyGen / Sign / Verify
- OrbitHead extraction
- LSH + Phase orbit descriptor
- challenge derivation
- binding hash
- signature serialization

### Blockchain Layer
- wallet/address generation
- signed transfer transactions
- transaction payload encoding
- transaction ID generation
- Merkle root computation
- block header and block body
- simplified proof-of-work mining
- account-based state machine
- local node gossip simulation

### Visualization / Demo Layer
- Qt parameter panel
- runtime benchmarking
- attack simulation
- acceptance basin scan
- real-time charts for:
  - LSH bits
  - Phase segments
  - EncodedOrbit_Z
  - RecoveredOrbit_W
  - basin pass-rate trends

## 4. Research Boundaries

This project intentionally keeps a strict research-prototype boundary.

### Not yet provided
- formal security reduction
- side-channel resistance
- constant-time implementation
- fuzz testing
- distributed consensus network
- persistence/database storage
- compatibility with existing public blockchains
- smart contract support

### Experimental assumptions
- current parameters are prototype parameters
- current blockchain layer is minimal and local
- current proof-of-work is simplified
- current security claims are empirical, not formal

## 5. Build

Typical build on macOS/Linux with OpenSSL:

```bash
g++ -std=c++17 main.cpp -lssl -lcrypto -O2 -o ceh_blockchain_fireseed
./ceh_blockchain_fireseed
```

For the Qt version, use your Qt/CMake project configuration with:
- Qt Widgets
- Qt Charts
- OpenSSL

## 6. Recommended Reading Order

1. `USAGE.md`
2. `CEH-Orbit_Blockchain_Spec_V1.md`
3. `CEH-Orbit_Blockchain_Whitepaper_V1.md`
4. `CEH-Orbit_Blockchain_Paper.md`
5. `DISCLAIMER.md`
6. `THIRD_PARTY_NOTICES.md`

## 7. Open Problems

Contributors and researchers may continue in directions such as:

- formal proof attempts
- parameter expansion (`N = 256`, `N = 512`)
- tolerance-mode orbit verification
- better challenge encoding
- more realistic blockchain consensus
- multi-node distributed simulation
- side-channel hardening
- optimized arithmetic / NTT / SIMD / GPU paths

## 8. Author

Chen Enhua  
Email: a106079595@qq.com

## 9. Notice

This project is best described as:

> a working seed, not a finished system.

## License Notice
⚠️ Commercial use is strictly prohibited without authorization.

Contact: a106079595@qq.com