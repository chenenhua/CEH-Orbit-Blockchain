# USAGE

This document explains how to use the CEH-Orbit Blockchain Fire Seed prototype.

## 1. What this prototype demonstrates

The prototype demonstrates a minimal end-to-end flow:

1. create wallets
2. generate addresses
3. sign transfer transactions with CEH-Orbit
4. verify transactions at node level
5. place verified transactions into a mempool
6. build a block
7. compute a Merkle root
8. perform simplified proof-of-work
9. update account state

## 2. Typical workflow

### Command-line / single-file mode
A typical run does the following:

- initializes wallets for Alice, Bob, and Miner
- creates a genesis block
- lets Alice sign and submit a payment to Bob
- verifies the signature and state validity
- mines a block that includes the transaction
- updates balances and nonces
- runs benchmark, forgery tests, collision tests, and basin scan

### Qt mode
The Qt visual interface allows the user to:

- tune core parameters
- regenerate wallets
- create and sign transactions
- mine blocks
- run benchmark tests
- run forgery tests
- run acceptance basin scans
- observe real-time charts

## 3. Adjustable parameters

Typical adjustable parameters include:

- `N`
- `Q`
- `NAV_ZONES`
- `DELTA`
- `SIG_BOUND`
- `CHALLENGE_WT`
- `Y_MIN`
- `Y_MAX`
- `genesis_balance`
- `mining_reward`
- `pow_difficulty`
- `attack_rounds`
- `collision_trials`
- `basin_rounds`

### Important notes
- In the current prototype, practical visualization assumes a 128-bit LSH layout.
- If you increase `N`, make sure the implementation and charts are still consistent.
- This repository is a research prototype, so parameter changes are exploratory, not standardized.

## 4. Interpreting the output

### Signature verification
A successful verification generally means:

- `z_bound_ok = true`
- `bind_ok = true`
- `head_ok = true`
- `chal_ok = true`
- final result = `PASS`

### Attack test
A low or zero success rate in simple random-forgery tests indicates that the current strict-mode checks reject naive tampering. This does not constitute a formal proof.

### Collision test
A low observed collision rate for `OrbitHead` means that collisions were not seen frequently under the tested sample size. This does not imply a proven bound.

### Basin scan
In strict orbit-locking mode, even small perturbations may result in zero pass rate. This is expected under a zero-tolerance design.

## 5. Suggested usage scenarios

Suitable for:
- research demonstrations
- internal concept validation
- educational visualization
- protocol/system brainstorming
- experimental blockchain extensions

Not suitable for:
- financial production systems
- custody systems
- public chain deployment
- certified security products
- high-value key custody

## 6. Recommended safe usage policy

When presenting or publishing results, describe the system as:

- a research prototype
- a fire-seed implementation
- an exploratory blockchain application of CEH-Orbit

Do not describe it as:
- production secure
- standard-compliant
- formally proven
- public-chain ready

## 7. Future extensions

Possible extensions include:
- persistence
- network transport
- distributed consensus
- richer address formats
- script/contract layers
- improved parameter presets
- optimized arithmetic
