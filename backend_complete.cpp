#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <random>
#include <string>
#include <vector>

using namespace std;

const string INTERNAL_KEY = "only_rahat_opens";
bool LOADING_MODE = false;

// ── Forward declarations ──────────────────────────────────────────────────────
class Blockchain;
class Wallet;
class TxnStack;
void save_blockchain(vector < unsigned char > data);
string calculate_hash(int index, string data, string previoushash, string timestamp, int nonce);

// ── Hash / key helpers ────────────────────────────────────────────────────────
string calculate_hash(int index, string data, string previoushash, string timestamp, int nonce) {
  string input = to_string(index) + data + previoushash + timestamp + to_string(nonce);
  long long hash = 0;
  for (char c : input)
    hash = (hash * 71 + c) % 1000000009LL;
  string result = to_string(hash);
  while (result.length() < 10)
    result = "0" + result;
  return result;
}

string public_key(string secret, string enc_key) {
  string res = "PUB_" + calculate_hash(0, secret + ":" + enc_key, "DECENTRALIZED_ROOT", "STABLE_ID", 0);
  return res;
}

// ── AES helpers ───────────────────────────────────────────────────────────────
vector < unsigned char > aes_encrypt(string data, string key) {
  EVP_CIPHER_CTX * ctx = EVP_CIPHER_CTX_new();
  unsigned char iv[16] = {
    0
  };
  unsigned char keyBytes[16] = {
    0
  };
  for (int i = 0; i < (int) key.size() && i < 16; i++)
    keyBytes[i] = key[i];

  vector < unsigned char > output(data.size() + 32);
  int len = 0, totalLen = 0;
  EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, keyBytes, iv);
  EVP_EncryptUpdate(ctx, output.data(), & len, (unsigned char * ) data.c_str(), (int) data.size());
  totalLen = len;
  EVP_EncryptFinal_ex(ctx, output.data() + len, & len);
  totalLen += len;
  EVP_CIPHER_CTX_free(ctx);
  output.resize(totalLen);
  return output;
}

string aes_decrypt(vector < unsigned char > data, string key) {
  EVP_CIPHER_CTX * ctx = EVP_CIPHER_CTX_new();
  unsigned char iv[16] = {
    0
  };
  unsigned char keyBytes[16] = {
    0
  };
  for (int i = 0; i < (int) key.size() && i < 16; i++)
    keyBytes[i] = key[i];

  vector < unsigned char > output(data.size() + 16);
  int len = 0, totalLen = 0;
  EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, keyBytes, iv);
  EVP_DecryptUpdate(ctx, output.data(), & len, data.data(), (int) data.size());
  totalLen = len;
  if (EVP_DecryptFinal_ex(ctx, output.data() + len, & len) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return "DECRYPT_ERROR";
  }
  totalLen += len;
  EVP_CIPHER_CTX_free(ctx);
  return string(output.begin(), output.begin() + totalLen);
}

// ── TxnStack ──────────────────────────────────────────────────────────────────
class TxnStack {
  vector < string > data;

  public:
    void push(string txn) {
      this -> data.push_back(txn);
    }
  void display() {
    cout << "\n=== STACK TRANSACTION HISTORY ===\n";
    for (int i = (int) this -> data.size() - 1; i >= 0; i--)
      cout << this -> data[i] << "\n----------------------\n";
    if (this -> data.empty()) cout << "No transactions found.\n";
  }
  int size() {
    return (int) this -> data.size();
  }
};

// ── Wallet ────────────────────────────────────────────────────────────────────
class Wallet {
  string privatekey;
  string creation_time;
  string encryption_key;

  public:
    string publickey;

  friend void transfer(Blockchain & bc, Wallet * wallet);
  friend Wallet * create_acc(Blockchain & bc);
  friend Wallet * login_wallet(Blockchain & bc);
  friend Wallet * create_agent(Blockchain & bc);

  Wallet(string secret, string enc_key, string ts = "") {
    this -> privatekey = secret;
    this -> encryption_key = enc_key;
    if (ts.empty()) {
      time_t now = time(0);
      this -> creation_time = to_string(now);
    } else {
      this -> creation_time = ts;
    }
    this -> publickey = public_key(this -> privatekey, this -> encryption_key);
  }
  string getCreationTime() {
    return this -> creation_time;
  }
  string getPublicKey() {
    return this -> publickey;
  }
  string getencyptionkey() {
    return this -> encryption_key;
  }
  string getPrivateKey() {
    return this -> privatekey;
  }
};

// ── Node ──────────────────────────────────────────────────────────────────────
class Node {
  friend class Blockchain;
  friend void show_all_wallet(Blockchain & bc);
  friend Wallet * login_wallet(Blockchain & bc);
  friend int get_balance(Blockchain & bc, string pub);
  friend int get_balance(Blockchain & bc, string pub, string enc_key);
  friend bool wallet_exist(Blockchain & bc, string pub);
  friend TxnStack build_stack(Blockchain & bc, Wallet * wallet);

  int index;
  vector < unsigned char > data; // always encrypted with INTERNAL_KEY
  string previoushash;
  string nexthash;
  string hash;
  Node * next;
  Node * prev;
  string timestamp;
  int nonce;

  Node(int idx, vector < unsigned char > d, string prevhash) {
    time_t now = time(0);
    char * dt = ctime( & now);
    this -> timestamp = dt ? dt : "";
    if (!this -> timestamp.empty() && this -> timestamp.back() == '\n')
      this -> timestamp.pop_back();

    this -> index = idx;
    this -> data = d;
    this -> previoushash = prevhash;
    this -> nexthash = "";
    this -> next = this -> prev = NULL;
    this -> nonce = 0;
    string dataStr(d.begin(), d.end());
    this -> hash = calculate_hash(this -> index, dataStr, prevhash, this -> timestamp, 0);
  }
};

// ── Helpers ───────────────────────────────────────────────────────────────────
string generate_secret_key(int length = 32) {
  const string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";
  random_device rd;
  mt19937 gen(rd());
  uniform_int_distribution <> dis(0, (int) chars.size() - 1);
  string key;
  for (int i = 0; i < length; i++)
    key += chars[dis(gen)];
  return key;
}

// ── Blockchain ────────────────────────────────────────────────────────────────
class Blockchain {
  friend void show_all_wallet(Blockchain & bc);
  friend int get_balance(Blockchain & bc, string pub);
  friend int get_balance(Blockchain & bc, string pub, string enc_key);
  friend TxnStack build_stack(Blockchain & bc, Wallet * wallet);
  friend void load_blockchain(Blockchain & bc);
  friend Wallet * login_wallet(Blockchain & bc);
  friend void transfer(Blockchain & bc, Wallet * wallet);
  friend Wallet * create_acc(Blockchain & bc);
  friend Wallet * create_agent(Blockchain & bc);
  friend bool wallet_exist(Blockchain & bc, string pub);

  Node * head;
  Node * tail;
  int size;

  public:
    Blockchain() {
      this -> head = NULL;
      this -> tail = NULL;
      this -> size = 0;
    }
  Node * getHead() {
    return this -> head;
  }

  vector < string > getmerklehashes() {
    vector < string > hashes;
    Node * t = this -> head;
    while (t) {
      hashes.push_back(t -> hash);
      t = t -> next;
    }
    return hashes;
  }

  string merkleroot() {
    vector < string > hashes = this -> getmerklehashes();
    if (hashes.empty()) return "EMPTY";
    while (hashes.size() > 1) {
      vector < string > lvl;
      for (int i = 0; i < (int) hashes.size(); i += 2) {
        if (i + 1 < (int) hashes.size())
          lvl.push_back(calculate_hash(0, hashes[i] + hashes[i + 1], "", "", 0));
        else
          lvl.push_back(hashes[i]);
      }
      hashes = lvl;
    }
    return hashes[0];
  }

  void insert_plaintext(string plaintext) {
    vector < unsigned char > enc = aes_encrypt(plaintext, INTERNAL_KEY);
    this -> insert_tail(enc);
  }

  void insert_tail(vector < unsigned char > encData) {
    string prevhash = this -> tail ? this -> tail -> hash : "0";
    Node * nn = new Node(this -> size, encData, prevhash);
    string dataStr(encData.begin(), encData.end());

    if (LOADING_MODE) {
      nn -> nonce = 0;
      nn -> hash = calculate_hash(nn -> index, dataStr, nn -> previoushash, nn -> timestamp, 0);
    } else {
      const int DIFFICULTY = 3;
      cout << "Mining block " << this -> size << "..." << endl;
      while (true) {
        nn -> hash = calculate_hash(nn -> index, dataStr, nn -> previoushash, nn -> timestamp, nn -> nonce);
        bool ok = true;
        for (int i = 0; i < DIFFICULTY && i < (int) nn -> hash.size(); i++)
          if (nn -> hash[i] != '0') {
            ok = false;
            break;
          }
        if (ok) break;
        if (++nn -> nonce % 100000 == 0)
          cout << "Mining... Nonce: " << nn -> nonce << endl;
      }
      cout << "Block Mined! Hash: " << nn -> hash << " (Nonce: " << nn -> nonce << ")" << endl;
    }

    if (!this -> head) {
      this -> head = this -> tail = nn;
    } else {
      this -> tail -> nexthash = nn -> hash;
      this -> tail -> next = nn;
      nn -> prev = this -> tail;
      nn -> previoushash = this -> tail -> hash;
      this -> tail = nn;
    }
    this -> size++;
    if (!LOADING_MODE)
      save_blockchain(encData);
  }

  void isvalid() {
    Node * t = this -> head;
    if (!t) return;
    while (t) {
      string dataStr(t -> data.begin(), t -> data.end());
      string recalc = calculate_hash(t -> index, dataStr, t -> previoushash, t -> timestamp, t -> nonce);
      if (t -> hash != recalc) {
        cout << "TAMPERED at block " << t -> index << endl;
        this -> writelog("tampered at block " + to_string(t -> index));
        return;
      }
      if (t -> prev && t -> previoushash != t -> prev -> hash) {
        cout << "LINK BROKEN at block " << t -> index << endl;
        this -> writelog("link broken at block " + to_string(t -> index));
        return;
      }
      t = t -> next;
    }
    cout << "Chain valid — all checks passed." << endl;
    this -> writelog("Chain Valid - All OK");
  }

  void writelog(string msg) {
    FILE * f = fopen("audit_log.txt", "a");
    if (f) {
      time_t now = time(0);
      fprintf(f, "%lld | %s\n", (long long) now, msg.c_str());
      fclose(f);
    }
  }
};

// ── File helpers ──────────────────────────────────────────────────────────────
void save_to_file(const string & filename, const string & pub, const string & sec, const string & enc, const string & ts) {
  string path = filename + ".txt";
  FILE * f = fopen(path.c_str(), "w");
  if (f) {
    fprintf(f, "---WALLET BACKUP---\n");
    fprintf(f, "Public Address: %s\n", pub.c_str());
    fprintf(f, "Secret Key: %s\n", sec.c_str());
    fprintf(f, "Encryption Key: %s\n", enc.c_str());
    fprintf(f, "Created At: %s\n", ts.c_str());
    fclose(f);
    cout << ">>> Backup created: " << path << endl;
  } else {
    cout << ">>> Warning: Could not create backup file " << path << endl;
  }
}

void save_blockchain(vector < unsigned char > encData) {
  ofstream f("blockchain.dat", ios::app | ios::binary);
  int sz = (int) encData.size();
  f.write((char * ) & sz, sizeof(sz));
  f.write((char * ) encData.data(), sz);
  f.flush();
  f.close();
}

void load_blockchain(Blockchain & bc) {
  ifstream f("blockchain.dat", ios::binary);
  if (!f.is_open()) {
    cout << "No previous blockchain found.\n";
    return;
  }
  LOADING_MODE = true;
  while (true) {
    int sz = 0;
    if (!f.read((char * ) & sz, sizeof(sz))) break;
    if (sz <= 0 || sz > 1000000) break; // sanity guard
    vector < unsigned char > enc(sz);
    if (!f.read((char * ) enc.data(), sz)) break;
    bc.insert_tail(enc);
  }
  LOADING_MODE = false;
  f.close();
  cout << "Blockchain loaded." << endl;
}

// ── Forward declarations of free functions ────────────────────────────────────
int get_balance(Blockchain & bc, string pub);
bool wallet_exist(Blockchain & bc, string pub);

// ── Account creation ──────────────────────────────────────────────────────────
void give_signup_bonus(Blockchain & bc, string pub) {
  bc.insert_plaintext("SYSTEM|" + pub + "|100");
  cout << ">>> Signup Bonus: 100 tk added!" << endl;
}

Wallet * create_acc(Blockchain & bc) {
  string enc_key, user_filename;
  cout << "\n========== ACCOUNT CREATION ==========\n";
  cout << "Step 1: Set a Transaction Pin : ";
  cin >> enc_key;
  cout << "Step 2: Filename for this Wallet backup: ";
  cin >> user_filename;

  string secret = generate_secret_key();
  cout << ">>> Auto Generated Secret Key: " << secret << endl;
  Wallet * w = new Wallet(secret, enc_key);
  save_to_file(user_filename, w -> publickey, secret, enc_key, w -> getCreationTime());

  bc.insert_plaintext("WALLET|" + w -> publickey + "|" + w -> getCreationTime());

  cout << "\n>>> [SUCCESS] Account Initialized!\n" << ">>> Your Public Address: " << w -> publickey << "\n" << ">>> Created At: " << w -> getCreationTime() << "\n" << ">>> Please login to access your wallet.\n" << "======================================\n\n";
  give_signup_bonus(bc, w -> publickey);
  return w;
}

Wallet * create_agent(Blockchain & bc) {
  string enc_key, filename;
  cout << "\n====== CREATE AGENT ACCOUNT ======\n";
  string secret = generate_secret_key();
  cout << ">>> Agent Secret Key: " << secret << "\n";
  cout << "Enter Pin Key: ";
  cin >> enc_key;
  cout << "Enter Filename: ";
  cin >> filename;

  string agent_pub = public_key(secret, enc_key) + "_AGENT";
  save_to_file(filename, agent_pub, secret, enc_key, "AGENT");
  bc.insert_plaintext("AGENT|" + agent_pub + "|CREATED");

  cout << ">>> [SUCCESS] Agent Account Created!\n" << ">>> Public Key: " << agent_pub << "\n";
  give_signup_bonus(bc, agent_pub);
  return new Wallet(secret, enc_key);
}

// ── Transaction ───────────────────────────────────────────────────────────────
void transfer(Blockchain & bc, Wallet * wallet) {
  string receiver, amount, input_secret;
  cout << "\n========== SEND TRANSACTION ==========\n";
  cout << "Enter Receiver's Public Address: ";
  cin >> receiver;
  cout << "Enter Amount: ";
  cin >> amount;
  cout << "Enter your Secret Key to confirm: ";
  cin >> input_secret;

  if (input_secret != wallet -> getPrivateKey()) {
    cout << ">>> [ERROR] Unauthorized! Private Key mismatch.\n";
    return;
  }
  if (!wallet_exist(bc, receiver)) {
    cout << ">>> [FAILED] Receiver wallet not found!\n";
    return;
  }
  if (amount.find_first_not_of("0123456789") != string::npos || amount.empty()) {
    cout << ">>> [ERROR] Invalid amount!\n";
    return;
  }
  int amt = stoi(amount);
  int bal = get_balance(bc, wallet -> publickey, wallet -> getencyptionkey());
  if (amt > bal) {
    cout << ">>> [FAILED] Insufficient balance! Your balance: " << bal << " tk\n";
    return;
  }
  time_t now = time(0);
  char * dt = ctime( & now);
  string ts = dt ? dt : "";
  if (!ts.empty() && ts.back() == '\n') ts.pop_back();

  string rawdata = wallet -> publickey + "|" + receiver + "|" + amount + "|" + ts;
  string txnID = "TXN" + calculate_hash(0, rawdata, "TXN_SECURITY", ts, 0);
  string sig = calculate_hash(0, rawdata, "TXN_SECURE", ts, 0);

  vector < unsigned char > userEnc = aes_encrypt(rawdata + "|SIG:" + sig, wallet -> getencyptionkey());
  string userEncStr(userEnc.begin(), userEnc.end());
  bc.insert_tail(aes_encrypt(userEncStr, INTERNAL_KEY));

  cout << "\n>>> [NETWORK INFO] Verifying...\n";
  bc.isvalid();
  cout << ">>> [SUCCESS] Transaction Recorded!\n" << ">>> Txn ID: " << txnID << "\n" << "======================================\n\n";
}

// ── Login ─────────────────────────────────────────────────────────────────────
Wallet * login_wallet(Blockchain & bc) {
  int login_choice;
  cout << "\n========== LOGIN MENU ==========\n" << "1. Login via Backup File\n" << "2. Manual Login (Secret + Encryption Key)\n" << "Choice: ";
  cin >> login_choice;

  string secret, enc_key, file_pub = "";
  if (login_choice == 1) {
    string filepath;
    cout << "Enter wallet file path: ";
    cin.ignore();
    getline(cin, filepath);
    if (!filepath.empty() && filepath.front() == '"') {
      filepath = filepath.substr(1, filepath.size() - 2);
    }
    ifstream f(filepath);
    if (!f.is_open()) {
      cout << ">>> [ERROR] Could not open file!\n";
      return NULL;
    }
    string line, file_pub = "";
    while (getline(f, line)) {
      if (line.find("Public") != string::npos)
        file_pub = line.substr(line.find(":") + 1);
      else if (line.find("Secret") != string::npos)
        secret = line.substr(line.find(":") + 1);
      else if (line.find("Encryption") != string::npos)
        enc_key = line.substr(line.find(":") + 1);
    }
    f.close();

    auto trim = [](string & s) {
      if (s.empty()) return;
      s.erase(0, s.find_first_not_of(" \t\r\n"));
      s.erase(s.find_last_not_of(" \t\r\n") + 1);
    };
    trim(file_pub);
    trim(secret);
    trim(enc_key);

    if (secret.empty() || enc_key.empty()) {
      cout << ">>> [ERROR] File format is wrong!\n";
      return NULL;
    }
  } else if (login_choice == 2) {
    cout << "Enter your Secret Key: ";
    cin >> secret;
    cout << "Enter your Encryption Key: ";
    cin >> enc_key;
  } else {
    cout << ">>> [ERROR] Invalid choice.\n";
    return NULL;
  }

  string user_pub = public_key(secret, enc_key);
  string agent_pub = user_pub + "_AGENT";

  Node * t = bc.getHead();
  while (t) {
    string plain = aes_decrypt(t -> data, INTERNAL_KEY);
    if (plain != "DECRYPT_ERROR") {
      if (plain.find("AGENT|" + agent_pub) == 0) {
        cout << ">>> AGENT LOGIN SUCCESS\n";
        Wallet * w = new Wallet(secret, enc_key);
        w -> publickey = agent_pub;
        return w;
      }
      if (plain.find("WALLET|" + user_pub) == 0 || (!file_pub.empty() && plain.find("WALLET|" + file_pub) == 0)) {
        if (!file_pub.empty() && plain.find("WALLET|" + file_pub) == 0) user_pub = file_pub;
        cout << ">>> USER LOGIN SUCCESS\n";
        Wallet * w = new Wallet(secret, enc_key);
        w -> publickey = user_pub;
        return w;
      }
    }
    t = t -> next;
  }
  cout << ">>> LOGIN FAILED\n";
  return NULL;
}

// ── Show all wallets ──────────────────────────────────────────────────────────
void show_all_wallet(Blockchain & bc) {
  cout << "\n__All Accounts__\n";
  Node * t = bc.getHead();
  if (!t) {
    cout << "No wallets found.\n";
    return;
  }
  bool found = false;
  while (t) {
    string plain = aes_decrypt(t -> data, INTERNAL_KEY);
    if (plain != "DECRYPT_ERROR" && plain.find("WALLET|") == 0) {
      int p1 = (int) plain.find("|");
      int p2 = (int) plain.find("|", p1 + 1);
      cout << "Wallet: " << plain.substr(p1 + 1, p2 - p1 - 1) << "\n";
      found = true;
    }
    t = t -> next;
  }
  if (!found) cout << "No wallet records found.\n";
  cout << "=====================\n";
}

// ── Balance ───────────────────────────────────────────────────────────────────
int get_balance(Blockchain & bc, string pub) {
  int balance = 0;
  Node * t = bc.getHead();
  while (t) {
    string plain = aes_decrypt(t -> data, INTERNAL_KEY);
    if (plain == "DECRYPT_ERROR") {
      t = t -> next;
      continue;
    }

    if (plain.find("SYSTEM|") == 0) {
      size_t p1 = plain.find('|');
      size_t p2 = plain.find('|', p1 + 1);
      if (p1 != string::npos && p2 != string::npos) {
        string recv = plain.substr(p1 + 1, p2 - p1 - 1);
        if (recv == pub) balance += stoi(plain.substr(p2 + 1));
      }
    }
    t = t -> next;
  }
  return balance;
}

int get_balance(Blockchain & bc, string pub, string enc_key) {
  int balance = 0;
  Node * t = bc.getHead();
  while (t) {
    string plain = aes_decrypt(t -> data, INTERNAL_KEY);
    if (plain == "DECRYPT_ERROR") {
      t = t -> next;
      continue;
    }

    if (plain.find("SYSTEM|") == 0) {
      size_t p1 = plain.find('|');
      size_t p2 = plain.find('|', p1 + 1);
      if (p1 != string::npos && p2 != string::npos) {
        string recv = plain.substr(p1 + 1, p2 - p1 - 1);
        if (recv == pub) balance += stoi(plain.substr(p2 + 1));
      }
    } else if (plain.find("WALLET|") != 0 && plain.find("AGENT|") != 0) {
      vector < unsigned char > innerVec(plain.begin(), plain.end());
      string inner = aes_decrypt(innerVec, enc_key);
      if (inner != "DECRYPT_ERROR") {
        size_t p1 = inner.find('|');
        size_t p2 = inner.find('|', p1 + 1);
        size_t p3 = inner.find('|', p2 + 1);
        if (p1 != string::npos && p2 != string::npos && p3 != string::npos) {
          string sender = inner.substr(0, p1);
          string recv = inner.substr(p1 + 1, p2 - p1 - 1);
          int amt = stoi(inner.substr(p2 + 1, p3 - p2 - 1));
          if (recv == pub) balance += amt;
          if (sender == pub) balance -= amt;
        }
      }
    }
    t = t -> next;
  }
  return balance;
}

// ── Wallet existence ──────────────────────────────────────────────────────────
bool wallet_exist(Blockchain & bc, string pub) {
  Node * t = bc.getHead();
  while (t) {
    string plain = aes_decrypt(t -> data, INTERNAL_KEY);
    if (plain != "DECRYPT_ERROR" && (plain.find("WALLET|" + pub) == 0 || plain.find("AGENT|" + pub) == 0))
      return true;
    t = t -> next;
  }
  return false;
}

// ── Transaction history ───────────────────────────────────────────────────────
TxnStack build_stack(Blockchain & bc, Wallet * wallet) {
  TxnStack st;
  Node * t = bc.getHead();
  while (t) {
    string step1 = aes_decrypt(t -> data, INTERNAL_KEY);
    if (step1 != "DECRYPT_ERROR") {
      vector < unsigned char > innerVec(step1.begin(), step1.end());
      string inner = aes_decrypt(innerVec, wallet -> getencyptionkey());
      if (inner != "DECRYPT_ERROR" && inner.find(wallet -> publickey) != string::npos)
        st.push(inner);
    }
    t = t -> next;
  }
  return st;
}

// ── Main ──────────────────────────────────────────────────────────────────────
int main() {
  cout << "--- Network Initialization ---\n";
  Blockchain bc;
  load_blockchain(bc);
  Wallet * wallet = NULL;
  int choice, choice2;

  while (true) {
    cout << "\n===== WELCOME TO BLOCKCHAIN NETWORK =====\n" << "1. Create New Wallet\n" << "2. Create Agent Account\n" << "3. Login to Existing Wallet\n" << "4. Exit\n" << "5. Show All Accounts\n" << "Choice: ";
    cin >> choice;

    if (choice == 1) {
      create_acc(bc);
    } else if (choice == 2) {
      create_agent(bc);
    } else if (choice == 3) {
      wallet = login_wallet(bc);
    } else if (choice == 4) {
      break;
    } else if (choice == 5) {
      show_all_wallet(bc);
    }

    if (wallet) {
      cout << "Logged in as: " << wallet -> publickey << "\n";
      bool loggedin = true;
      while (loggedin) {
        cout << "\n---------- USER DASHBOARD ----------\n" << "1. Send Transaction\n" << "2. View Transaction History\n" << "3. Check Balance\n" << "4. Logout\n" << "Choice: ";
        cin >> choice2;
        if (choice2 == 1) {
          transfer(bc, wallet);
        } else if (choice2 == 2) {
          TxnStack st = build_stack(bc, wallet);
          st.display();
        } else if (choice2 == 3) {
          int bal = get_balance(bc, wallet -> publickey, wallet -> getencyptionkey());
          cout << ">>> Current Balance: " << bal << " tk\n";
        } else if (choice2 == 4) {
          loggedin = false;
          wallet = NULL;
          cout << "Logged out successfully!\n";
        }
      }
    }
  }
  return 0;
}

//$env:Path = "C:\msys64\mingw64\bin;C:\msys64\usr\bin;" + $env:Path ; g++ backend_complete.cpp -o backend_complete.exe -I"C:/msys64/mingw64/include" -L"C:/msys64/mingw64/lib" -lssl -lcrypto -std=c++17


//.\backend_complete.exe


/*cd Blockchain
g++ backend_complete.cpp -o backend_complete.exe -IC:\Users\User\anaconda3\Library\include -LC:\Users\User\anaconda3\Library\lib -lssl -lcrypto -lws2_32 -lgdi32 -lcrypt32 -std=c++17
.\backend_complete.exe
*/