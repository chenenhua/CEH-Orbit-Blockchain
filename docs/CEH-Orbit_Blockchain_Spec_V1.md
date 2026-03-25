# CEH-Orbit Blockchain Spec V1

## 1. Scope

This specification describes the **blockchain application layer** built on top of the original CEH-Orbit signature prototype. It does not redefine the full original CEH-Orbit theory; instead, it specifies how that signing flow is used in a minimal blockchain environment.

## 2. Core Entities

### 2.1 Wallet
A wallet consists of:
- owner label
- CEH-Orbit keypair
- derived address

### 2.2 Address
Address is derived by hashing:
- domain separator
- architecture identifier
- public `a`
- public `t`

Output format:
- prefix: `ceh1_`
- truncated hex payload

### 2.3 Transaction
Transaction fields:
- `type`
- `from`
- `to`
- `amount`
- `nonce`
- `timestamp_ms`
- `memo`
- `sig`
- `txid`

### 2.4 BlockHeader
Fields:
- `height`
- `prev_hash`
- `merkle_root`
- `timestamp_ms`
- `difficulty`
- `nonce`

### 2.5 Block
Fields:
- `header`
- `txs`
- `block_hash`

## 3. Signature Usage

Transaction payload is formed as a canonical string:

`type|from|to|amount|nonce|timestamp_ms|memo`

The payload is signed using the CEH-Orbit signing flow.

Verification checks:
1. signature bounds
2. binding hash
3. reconstructed orbit head
4. challenge self-consistency

## 4. Transaction ID

Transaction ID is derived from:
- TXID domain separator
- architecture identifier
- transaction payload
- serialized signature bytes

## 5. Merkle Root

Merkle root is computed from transaction IDs using pairwise hashing. If the number of hashes is odd, the last hash is duplicated.

## 6. Mining

This prototype uses simplified proof-of-work:
- block header is string-encoded
- SHA-256 is computed
- success requires a hex hash with `difficulty` leading `'0'` characters

This is intentionally simplified for demonstration.

## 7. State Machine

The current implementation uses an account-based state model.

### Coinbase
- increases receiver balance

### Transfer
Requires:
- sender account exists
- sufficient balance
- sender nonce equals transaction nonce

Effects:
- sender balance decreases
- sender nonce increments
- receiver balance increases

## 8. Node Behavior

A node:
- receives transactions
- checks duplicates
- verifies signatures
- checks local state validity
- stores valid transactions in mempool
- gossips them locally
- mines blocks from mempool

## 9. Serialization Notes

Signature serialization currently includes:
- header marker/version
- parameter values
- `z`
- `head` (`lsh0`, `lsh1`, `phase`)
- challenge vector
- binding hash

This format is prototype-oriented and not standardized.

## 10. Non-Goals

This specification does not define:
- distributed consensus protocol
- peer transport protocol
- on-disk persistence
- public-chain compatibility
- smart contracts
- formal security proof
