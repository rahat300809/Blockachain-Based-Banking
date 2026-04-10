# 🔗 Blockchain-Based Banking System

A **C++ blockchain-based banking system** built from scratch, featuring cryptographic hashing (SHA-256 via OpenSSL), user authentication, transaction management, balance tracking, and audit logging.

---

## 📁 Repository Structure

This repository contains the **complete step-by-step development history** of the project — from early prototypes to the final implementation.

### 🏆 Latest / Final Version
> **[`chain_stack.cpp`](./chain_stack.cpp)** — This is the **most complete and up-to-date version** of the blockchain banking system. All features are fully implemented here.

---

### 🧱 Development Stages (Step-by-Step)

| File | Stage | Description |
|------|-------|-------------|
| `blockchain_encrypted.cpp` | Stage 1 | Basic blockchain structure with encryption |
| `blockchain_timestamp.cpp` | Stage 2 | Added timestamp support to blocks |
| `blockchain_time_core.cpp` | Stage 3 | Core time-based block chaining logic |
| `blockchain_final.cpp` | Stage 4 | First complete standalone version |
| `blockchain_recov.cpp` | Stage 5 | Added account recovery mechanism |
| `blocckhain_save.cpp` | Stage 6 | Persistent save/load for blockchain data |
| `blockchain_login.cpp` | Stage 7 | User login and authentication system |
| `blance_sending.cpp` | Stage 8 | Balance transfer and transaction logic |
| `tranj_solve.cpp` | Stage 9 | Transaction verification and conflict resolution |
| `trnj_verify.cpp` | Stage 10 | Full transaction verification with chain integrity check |
| `tk_load.cpp` | Stage 11 | Token-based session management |
| `bonus.cpp` | Stage 12 | Bonus/reward system integration |
| `logissue.cpp` | Stage 13 | Logging and issue tracking |
| `blochain_verify.cpp` | Stage 14 | Blockchain verification and tamper detection |
| `backend_complete.cpp` | Stage 15 | Complete backend with all modules |
| `new.cpp` | Stage 16 | Refactored codebase |
| `chain_bug.cpp` | Stage 17 | Bug-fix iteration |
| `arranged.cpp`| Stage 18 | Cleaned and arranged version |
| `chain_stack.cpp` | ✅ **FINAL** | **Latest, most complete implementation** |

---

### 🛠️ Other Files

| File | Description |
|------|-------------|
| `compile.bat` / `compile_chain.bat` | Batch scripts to compile with OpenSSL |
| `run_app.bat` | Script to run the application |
| `*.txt` | Test user wallet files / transaction logs used during testing |
| `hello.cpp`, `test.cpp`, `testing.cpp`, `test_env.cpp`, `test_err.cpp` | Small test/utility files used during development |
| `blockchain_time_core.cpp` | Core time utility (used as a module) |

---

## ⚙️ Features (Final Version)

- 🔐 **SHA-256 Cryptographic Hashing** (via OpenSSL)
- 👤 **User Registration & Login** with password hashing
- 💳 **Account Management** — create accounts, check balances
- 💸 **Transactions** — send/receive money with blockchain validation
- 🔗 **Chain Integrity** — tamper detection via hash chain verification
- 💾 **Persistent Storage** — blockchain saved to disk (`blockchain.dat`)
- 📋 **Audit Log** — all operations logged for traceability
- 🔑 **Account Recovery** — recover lost accounts securely

---

## 🧰 Build Requirements

- **Compiler**: `g++` (MinGW or MSYS2 on Windows)
- **Library**: OpenSSL (`libcrypto`, `libssl`)

### Compile (Windows)
```bash
g++ chain_stack.cpp -o chain_stack.exe -lssl -lcrypto -L./openssl_extracted/lib -I./openssl_extracted/include
```

Or use the provided batch script:
```bash
compile_chain.bat
```

### Run
```bash
chain_stack.exe
```

---

## 👨‍💻 Author

**rahat300809** — Built as a learning project to understand blockchain fundamentals from scratch using C++.
