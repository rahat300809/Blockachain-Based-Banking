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
void save_blockchain(vector<unsigned char> data);
bool LOADING_MODE = false;

class Blockchain;
class Wallet;
string calculate_hash(int index, string data, string previoushash,
                      string timestamp, int nonce);

string calculate_hash(int index, string data, string previoushash,
                      string timestamp, int nonce) {
  string input =
      to_string(index) + data + previoushash + timestamp + to_string(nonce);
  long long hash = 0;
  for (char c : input) {
    hash = (hash * 71 + c) % 1000000009;
  }
  string result = to_string(hash);

  // padding (leading zero add)
  while (result.length() < 10) {
    result = "0" + result;
  }

  return result;
}

string public_key(string secret, string enc_key) {
  string combined = secret + ":" + enc_key;
  return "PUB_" +
         calculate_hash(0, combined, "DECENTRALIZED_ROOT", "STABLE_ID", 0);
}

class TxnStack {
private:
  vector<string> data;

public:
  void push(string txn) {
    data.push_back(txn); // no pop
  }
  void display() {
    cout << "\n=== STACK TRANSACTION HISTORY ===\n";
    for (int i = data.size() - 1; i >= 0; i--) {
      cout << data[i] << endl;
      cout << "----------------------\n";
    }
  }
  int size() { return data.size(); }
};

class Wallet {
private:
  string privatekey;
  string creation_time;
  string encryption_key;

public:
  string publickey;

  friend void transfer(Blockchain &bc, Wallet *wallet);
  friend Wallet *create_acc(Blockchain &bc);
  friend Wallet *login_wallet(Blockchain &bc);

public:
  Wallet(string secret, string enc_key, string ts = "") {
    this->privatekey = secret;
    this->encryption_key = enc_key;
    if (ts == "") {
      time_t now = time(0);
      string temp_ts = ctime(&now);
      if (!temp_ts.empty() && temp_ts.back() == '\n')
        temp_ts.pop_back();
      this->creation_time = temp_ts;
    } else {
      this->creation_time = ts;
    }
    this->publickey = public_key(this->privatekey, this->encryption_key);
  }
  string getCreationTime() { return creation_time; }
  string getPublicKey() { return publickey; }
  string getencyptionkey() { return encryption_key; }
  string getPrivateKey() { return privatekey; }
};

vector<unsigned char> aes_encrypt(string data, string key) {
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  unsigned char iv[16] = {0};
  unsigned char keyBytes[16] = {0};
  for (int i = 0; i < (int)key.size() && i < 16; i++)
    keyBytes[i] = key[i];

  vector<unsigned char> output(data.size() + 16);
  int len = 0, totalLen = 0;
  EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, keyBytes, iv);
  EVP_EncryptUpdate(ctx, output.data(), &len, (unsigned char *)data.c_str(),
                    data.size());
  totalLen = len;
  EVP_EncryptFinal_ex(ctx, output.data() + len, &len);
  totalLen += len;
  EVP_CIPHER_CTX_free(ctx);
  output.resize(totalLen);
  return output;
}

string aes_decrypt(vector<unsigned char> data, string key) {
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  unsigned char iv[16] = {0};
  unsigned char keyBytes[16] = {0};
  for (int i = 0; i < (int)key.size() && i < 16; i++)
    keyBytes[i] = key[i];

  vector<unsigned char> output(data.size() + 16);
  int len = 0, totalLen = 0;
  EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, keyBytes, iv);
  EVP_DecryptUpdate(ctx, output.data(), &len, data.data(), data.size());
  totalLen = len;
  if (EVP_DecryptFinal_ex(ctx, output.data() + len, &len) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return "DECRYPT_ERROR";
  }
  totalLen += len;
  EVP_CIPHER_CTX_free(ctx);
  return string(output.begin(), output.begin() + totalLen);
}

class Node {
  friend class Blockchain;
  friend void show_all_wallet(Blockchain &bc);
  friend Wallet *login_wallet(Blockchain &bc);
  friend int get_balance(Blockchain &bc, string pub);

private:
  int index;
  vector<unsigned char> data;
  string previoushash;
  string nexthash;
  string hash;
  Node *next;
  Node *prev;
  string timestamp;
  int nonce;

  Node(int index, vector<unsigned char> data, string previoushash) {
    time_t now = time(0);
    this->timestamp = ctime(&now);

    if (!timestamp.empty() && timestamp.back() == '\n')
      timestamp.pop_back();

    this->index = index;
    this->data = data;
    this->previoushash = previoushash;
    this->nexthash = "";
    this->next = NULL;
    this->prev = NULL;
    string dataStr(data.begin(), data.end());
    this->nonce = 0;
    this->hash = calculate_hash(index, dataStr, previoushash, timestamp, 0);
  }
};

string generate_secret_key(int length = 32) {
  const string chars =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";
  random_device rd;
  mt19937 gen(rd());
  uniform_int_distribution<> dis(0, (int)chars.size() - 1);
  string key = "";
  for (int i = 0; i < length; i++) {
    key = key + chars[dis(gen)];
  }
  return key;
}

class Blockchain {
private:
  Node *head;
  Node *tail;
  int size;

public:
  Blockchain() {
    head = NULL;
    tail = NULL;
    size = 0;
  }
  Node *getHead() { return head; }
  vector<string> getmerklehashes() {
    vector<string> hashes;
    Node *temp = head;
    while (temp != NULL) {
      hashes.push_back(temp->hash);
      temp = temp->next;
    }
    return hashes;
  }

  string merkleroot() {
    vector<string> hashes = getmerklehashes();

    while (hashes.size() > 1) {
      vector<string> newlevel;
      for (int i = 0; i < (int)hashes.size(); i += 2) {
        if (i + 1 < (int)hashes.size()) {
          newlevel.push_back(
              calculate_hash(0, hashes[i] + hashes[i + 1], "", "", 0));
        } else {
          newlevel.push_back(hashes[i]);
        }
      }
      hashes = newlevel;
    }
    return hashes[0];
  }

  void insert_tail(vector<unsigned char> encryptedData) {
    string previoshash = "0";
    if (tail != NULL) {
      previoshash = tail->hash;
    }
    Node *newnode = new Node(size, encryptedData, previoshash);

    string datastr(encryptedData.begin(), encryptedData.end());

    if (LOADING_MODE) {
      newnode->nonce = 0;
      newnode->hash =
          calculate_hash(newnode->index, datastr, newnode->previoushash,
                         newnode->timestamp, 0);
    } else {
      int DIFFICULTY = 3; // 🔥 difficulty control

      cout << "Mining block " << size << "..." << endl;

      while (true) {
        newnode->hash =
            calculate_hash(newnode->index, datastr, newnode->previoushash,
                           newnode->timestamp, newnode->nonce);

        bool ok = true;

        for (int i = 0; i < DIFFICULTY; i++) {
          if (newnode->hash[i] != '0') {
            ok = false;
            break;
          }
        }

        if (ok)
          break;

        newnode->nonce++;

        if (newnode->nonce % 100000 == 0) {
          cout << "Mining... Nonce: " << newnode->nonce << endl;
        }
      }

      cout << "Block Mined! Hash: " << newnode->hash
           << " (Nonce: " << newnode->nonce << ")" << endl;
    }

    if (head == NULL) {
      head = newnode;
      tail = newnode;
      size++;

      if (!LOADING_MODE) {
        save_blockchain(encryptedData);
      }
      return;
    }
    tail->nexthash = newnode->hash;
    tail->next = newnode;
    newnode->prev = tail;
    newnode->previoushash = tail->hash;
    tail = newnode;
    size++;
    if (!LOADING_MODE) {
      save_blockchain(encryptedData);
    }
  }

  void display(string user_key) {
    Node *temp = head;

    while (temp != NULL) {
      cout << "Block: " << temp->index << " | Hash: " << temp->hash << endl;

      // step 1: decrypt internal
      string step1 = aes_decrypt(temp->data, INTERNAL_KEY);

      if (step1 != "DECRYPT_ERROR") {
        // step 2: decrypt user
        string final = aes_decrypt(
            vector<unsigned char>(step1.begin(), step1.end()), user_key);

        if (final != "DECRYPT_ERROR") {
          cout << "Decrypted Data: " << final << endl;
        } else {
          cout << "Data: [NOT YOUR TRANSACTION]" << endl;
        }
      } else {
        cout << "Data: [CORRUPTED]" << endl;
      }

      temp = temp->next;
      cout << "-------------------------" << endl;
    }
  }

  void isvalid() {
    Node *temp = head;
    if (temp == NULL) {
      return;
    }
    while (temp != NULL) {
      string dataStr(temp->data.begin(), temp->data.end());
      string recalculate_hash =
          calculate_hash(temp->index, dataStr, temp->previoushash,
                         temp->timestamp, temp->nonce);

      if (temp->hash != recalculate_hash) {
        cout << "tampered" << endl;
        writelog("tamperted at block" + to_string(temp->index));
        return;
      }
      if (temp->prev != NULL && temp->previoushash != temp->prev->hash) {
        cout << "tampered";
        writelog("tamperted at block" + to_string(temp->index));
        return;
      }
      temp = temp->next;
    }
    cout << "chain valid after checking all possible arguments" << endl;
    writelog("Chain Valid - All OK");
  }

  void writelog(string message) {
    ofstream file("audit_log.txt", ios::app);
    time_t now = time(0);
    file << ctime(&now) << "_" << message << "\n\n";
    file.close();
  }
};

void save_to_file(string filename, string pub, string sec, string enc,
                  string ts) {
  string full_path = filename + ".txt";
  ofstream file(full_path);

  if (file.is_open()) {
    file << "--- WALLET BACKUP ---" << endl;
    file << "Public Address : " << pub << endl;
    file << "Login Secret   : " << sec << endl;
    file << "Encryption Key : " << enc << endl;
    file << "Created At     : " << ts << endl;
    file << "---------------------" << endl;
    file.close();
    cout << ">>> Backup created: " << full_path << endl;
  }
}

void give_signup_bonus(Blockchain &bc, string pub) {
  string bonus = "SYSTEM|" + pub + "|100";

  vector<unsigned char> dataVec(bonus.begin(), bonus.end());
  bc.insert_tail(dataVec);

  cout << ">>> Signup Bonus: 100 tk added!" << endl;
}

Wallet *create_acc(Blockchain &bc) {
  string secret, enc_key, user_filename;
  cout << "\n========== ACCOUNT CREATION ==========" << endl;
  cout << "Step 1: Set a Transaction pin : ";
  cin >> enc_key;
  cout << "Step 2: Filename for this Wallet backup:";
  cin >> user_filename;
  secret = generate_secret_key();
  cout << ">>> Auto Generated Secret Key: " << secret << endl;

  Wallet *new_wallet = new Wallet(secret, enc_key);
  save_to_file(user_filename, new_wallet->publickey, secret, enc_key,
               new_wallet->getCreationTime());
  string public_id_record = new_wallet->publickey;
  string wallet_record =
      "WALLET|" + public_id_record + "|" + new_wallet->getCreationTime();

  vector<unsigned char> dataVec(wallet_record.begin(), wallet_record.end());
  bc.insert_tail(dataVec);

  cout << "\n>>> [SUCCESS] Account Initialized!" << endl;
  cout << ">>> Your Public Address: " << public_id_record << endl;
  cout << ">>> Created At: " << new_wallet->getCreationTime() << endl;
  cout << ">>> Please login to access your wallet." << endl;
  cout << "======================================\n" << endl;

  // 🔥 initial bonus
  give_signup_bonus(bc, public_id_record);
  return new_wallet;
}

Wallet *create_agent(Blockchain &bc) {
  string secret, enc_key, filename;

  cout << "\n====== CREATE AGENT ACCOUNT ======" << endl;

  secret = generate_secret_key();
  cout << ">>> Agent Secret Key: " << secret << endl;

  cout << "Enter pin Key: ";
  cin >> enc_key;

  cout << "Enter Filename: ";
  cin >> filename;

  // 🔥 normal public key generate
  string base_pub = public_key(secret, enc_key);

  // 🔥 agent mark add
  string agent_pub = base_pub + "_AGENT";

  // file save
  save_to_file(filename, agent_pub, secret, enc_key, "AGENT");

  // blockchain record
  string record = "AGENT|" + agent_pub + "|CREATED";
  vector<unsigned char> dataVec(record.begin(), record.end());

  bc.insert_tail(dataVec);

  cout << ">>> [SUCCESS] Agent Account Created!" << endl;
  cout << ">>> Public Key: " << agent_pub << endl;

  // 🔥 initial bonus
  give_signup_bonus(bc, agent_pub);
  return new Wallet(secret, enc_key);
}

void transfer(Blockchain &bc, Wallet *wallet) {
  string receiver, amount, input_secret;
  cout << "\n========== SEND TRANSACTION ==========" << endl;
  cout << "Enter Receiver's Public Address: ";
  cin >> receiver;
  cout << "Enter Amount: ";
  cin >> amount;
  cout << "Enter your Secret Key to confirm transaction: ";
  cin >> input_secret;

  if (input_secret != wallet->getPrivateKey()) {
    cout << ">>> [ERROR] Unauthorized! Private Key mismatch." << endl;
    return;
  }

  if (!wallet_exist(bc, receiver)) {
    cout << ">>> [FAILED] Receiver wallet not found!" << endl;
    return;
  }

  if (amount.find_first_not_of("0123456789") != string::npos) {
    cout << ">>> [ERROR] Invalid amount!" << endl;
    return;
  }

  int amt = stoi(amount);

  int sender_balance = get_balance(bc, wallet->publickey);

  if (amt > sender_balance) {
    cout << ">>> [FAILED] Insufficient balance!" << endl;
    cout << ">>> Your Balance: " << sender_balance << " tk" << endl;
    return;
  }

  time_t now = time(0);
  string ts = ctime(&now);
  if (!ts.empty() && ts.back() == '\n')
    ts.pop_back();

  string rawdata = wallet->publickey + "|" + receiver + "|" + amount + "|" + ts;
  string gentxn = "TXN" + calculate_hash(0, rawdata, "TXN_SECURITY", ts, 0);
  string signature = calculate_hash(0, rawdata, "TXN_SECURE", ts, 0);
  string payload = rawdata + "SIG:" + signature;

  vector<unsigned char> userEncrypted =
      aes_encrypt(payload, wallet->getencyptionkey());
  vector<unsigned char> finalData = aes_encrypt(
      string(userEncrypted.begin(), userEncrypted.end()), INTERNAL_KEY);
  bc.insert_tail(finalData);

  cout << "\n>>> [NETWORK INFO] Verifying transaction integrity..." << endl;
  bc.isvalid();

  cout << "\n>>> [SUCCESS] Transaction Recorded!" << endl;
  cout << ">>> Txn ID: " << gentxn << endl;
  cout << "======================================\n" << endl;
}

Wallet *login_wallet(Blockchain &bc) {
  int login_choice;
  cout << "\n========== LOGIN MENU ==========" << endl;
  cout << "1. Login via Backup File (Drag & Drop)" << endl;
  cout << "2. Manual Login (Type Secret & Encryption Key)" << endl;
  cout << "Choice: ";
  cin >> login_choice;

  string secret = "", enc_key = "", filepath = "";

  if (login_choice == 1) {
    cout << "Drag and drop your wallet file here or enter full path: " << endl;
    cout << "Path: ";
    cin.ignore();
    getline(cin, filepath);

    if (!filepath.empty() && filepath.front() == '"') {
      filepath.erase(0, 1);
      filepath.erase(filepath.size() - 1);
    }
    ifstream file(filepath);
    if (file.is_open()) {
      string line;
      while (getline(file, line)) {
        if (line.find("Login Secret   :") != string::npos) {
          secret = line.substr(line.find(":") + 2);
        } else if (line.find("pin :") != string::npos) {
          enc_key = line.substr(line.find(":") + 2);
        }
      }
      file.close();
      if (secret == "" || enc_key == "") {
        cout << ">>> [ERROR] File format is wrong!" << endl;
        return NULL;
      }
    } else {
      cout << ">>> [ERROR] Could not open file!" << endl;
      return NULL;
    }
  } else if (login_choice == 2) {
    cout << "Enter your Secret Key: ";
    cin >> secret;
    cout << "Enter your encryption key: ";
    cin >> enc_key;
  }

  string user_pub = public_key(secret, enc_key);

  string agent_pub = user_pub + "_AGENT";

  Node *temp = bc.getHead();
  bool found = false;
  cout << "===================================" << endl;

  while (temp != NULL) {
    string dataStr(temp->data.begin(), temp->data.end());

    if (dataStr.find("AGENT|" + agent_pub) == 0) {
      cout << ">>> AGENT LOGIN SUCCESS" << endl;

      Wallet *w = new Wallet(secret, enc_key);

      // 🔥 EDIT: override public key (IMPORTANT)
      w->publickey = agent_pub;

      return w;
    }

    if (dataStr.find("WALLET|" + user_pub) == 0) {
      cout << ">>> USER LOGIN SUCCESS" << endl;

      Wallet *w = new Wallet(secret, enc_key);

      // EDIT: normal user key
      w->publickey = user_pub;

      return w;
    }
    temp = temp->next;
  }
  cout << ">>> LOGIN FAILED" << endl;
  return NULL;
}

void show_all_wallet(Blockchain &bc) {
  Node *temp = bc.getHead();
  cout << endl << "__All Accouns__" << endl;
  if (temp == NULL) {
    cout << endl << "No Wallet Found" << endl;
    return;
  }
  while (temp != NULL) {
    string dataStr(temp->data.begin(), temp->data.end());
    if (dataStr.find("WALLET|") == 0) {
      int first = dataStr.find("|");
      int second = dataStr.find("|", first + 1);
      string pub = dataStr.substr(first + 1, second - first - 1);
      cout << "Wallet Address: " << pub << endl;
    }
    temp = temp->next;
  }
  cout << "=====================" << endl;
}

void save_blockchain(vector<unsigned char> data) {
  string plain(data.begin(), data.end());
  vector<unsigned char> encrypted = aes_encrypt(plain, INTERNAL_KEY);
  ofstream file("D:/Blockchain/blockchain.dat", ios::app | ios::binary);
  int size = (int)encrypted.size();

  file.write((char *)&size, sizeof(size));
  file.write((char *)encrypted.data(), size);
  file.flush();
  file.close();
}

void load_blockchain(Blockchain &bc) {
  ifstream file("D:/Blockchain/blockchain.dat", ios::binary);

  if (!file.is_open()) {
    cout << "no file found" << endl;
    return;
  }
  LOADING_MODE = true;
  while (true) {
    int size;
    if (!file.read((char *)&size, sizeof(size))) {
      break;
    }

    vector<unsigned char> encrypted(size);
    file.read((char *)encrypted.data(), size);
    string decrypted = aes_decrypt(encrypted, INTERNAL_KEY);
    vector<unsigned char> data(decrypted.begin(), decrypted.end());
    bc.insert_tail(data);
  }
  LOADING_MODE = false;
  file.close();
}

int get_balance(Blockchain &bc, string pub) {
  Node *temp = bc.getHead();
  int balance = 0;

  while (temp != NULL) {
    string dataStr(temp->data.begin(), temp->data.end());

    int p1 = dataStr.find("|");
    int p2 = dataStr.find("|", p1 + 1);

    if (p1 != string::npos && p2 != string::npos) {
      string sender = dataStr.substr(0, p1);
      string receiver = dataStr.substr(p1 + 1, p2 - p1 - 1);
      string amountStr = dataStr.substr(p2 + 1);

      int p3 = amountStr.find("|");
      if (p3 != string::npos)
        amountStr = amountStr.substr(0, p3);

      if (!amountStr.empty() &&
          amountStr.find_first_not_of("0123456789") == string::npos) {
        int amount = stoi(amountStr);

        if (sender == "SYSTEM") {
          if (receiver == pub)
            balance += amount;
        } else {
          if (receiver == pub)
            balance += amount;

          if (sender == pub)
            balance -= amount;
        }
      }
    }

    temp = temp->next;
  }

  return balance;
}

bool wallet_exist(Blockchain &bc, string pub) {
  Node *temp = bc.getHead();
  while (temp != NULL) {
    string dataStr(temp->data.begin(), temp->data.end());
    if (dataStr.find("WALLET|" + pub) == 0 ||
        dataStr.find("AGENT|" + pub) == 0) {
      return true;
    }
    temp = temp->next;
  }
  return false;
}

TxnStack build_stack(Blockchain &bc, Wallet *wallet) {
  TxnStack st;
  Node *temp = bc.getHead();
  while (temp != NULL) {
    string step1 = aes_decrypt(temp->data, INTERNAL_KEY);
    if (step1 != "DECRYPT_ERROR") {
      string final =
          aes_decrypt(vector<unsigned char>(step1.begin(), step1.end()),
                      wallet->getencyptionkey());
      if (final != "DECRYPT_ERROR") {
        if (final.find(wallet->publickey) != string::npos) {
          st.push(final);
        }
      }
    }

    temp = temp->next;
  }
  return st;
}

// void display_my_transactions(Blockchain &bc, Wallet* wallet)
// {
//     Node* temp = bc.getHead();

//     cout << "\n===== MY TRANSACTION HISTORY =====\n";

//     while(temp != NULL)
//     {
//         string step1 = aes_decrypt(temp->data, INTERNAL_KEY);

//         if(step1 != "DECRYPT_ERROR")
//         {
//             string final = aes_decrypt(
//                 vector<unsigned char>(step1.begin(), step1.end()),
//                 wallet->getencyptionkey()
//             );

//             if(final != "DECRYPT_ERROR")
//             {
//                 // only transaction blocks
//                 if(final.find("SIG:") == string::npos)
//                 {
//                     temp = temp->next;
//                     continue;
//                 }

//                 // split data
//                 int p1 = final.find("|");
//                 int p2 = final.find("|", p1 + 1);
//                 int p3 = final.find("|", p2 + 1);
//                 int sigPos = final.find("SIG:");

//                 string sender = final.substr(0, p1);
//                 string receiver = final.substr(p1 + 1, p2 - p1 - 1);
//                 string amount = final.substr(p2 + 1, p3 - p2 - 1);
//                 string time = final.substr(p3 + 1, sigPos - p3 - 1);

//                 // show only if related to user
//                 if(sender == wallet->publickey || receiver ==
//                 wallet->publickey)
//                 {
//                     cout << "Block: " << temp->index << endl;
//                     cout << "From   : " << sender << endl;
//                     cout << "To     : " << receiver << endl;
//                     cout << "Amount : " << amount << " tk" << endl;
//                     cout << "Time   : " << time << endl;
//                     cout << "-----------------------------" << endl;
//                 }
//             }
//         }

//         temp = temp->next;
//     }
// }

int main() {
  cout << "--- Network Initialization ---" << endl;
  Blockchain bc;
  load_blockchain(bc);

  Wallet *wallet = NULL;
  int choice, choice2;
  while (true) {
    cout << "\n===== WELCOME TO BLOCKCHAIN NETWORK =====" << endl;
    cout << "1. Create New Wallet (user Register)" << endl;
    cout << "2. Create Agent Account" << endl;
    cout << "3. Login to Existing Wallet" << endl;
    cout << "4. Exit" << endl;
    cout << "5. all account" << endl;
    cout << "Choice: ";
    cin >> choice;

    if (choice == 1) {
      create_acc(bc);
      continue;
    } else if (choice == 2) {
      create_agent(bc);
      continue;
    } else if (choice == 3) {
      wallet = login_wallet(bc);
    } else if (choice == 4) {
      break;
    } else if (choice == 5) {
      show_all_wallet(bc);
      continue;
    }

    if (wallet != NULL) {
      cout << "logged in as: " << wallet->publickey << endl;
      bool loggedin = true;
      while (loggedin) {
        cout << "\n---------- USER DASHBOARD ----------" << endl;
        cout << "1. Send Transaction (Money Transfer)" << endl;
        cout << "2. View Transaction History (Ledger)" << endl;
        cout << "3. Check Balance" << endl;
        cout << "4. Logout" << endl;
        cout << "Choice: ";
        cin >> choice2;

        if (choice2 == 1) {
          transfer(bc, wallet);
        } else if (choice2 == 2) {
          TxnStack st = build_stack(bc, wallet);
          st.display();
        } else if (choice2 == 3) {
          cout << "Checking Balance..." << endl;
          int bal = get_balance(bc, wallet->publickey);
          cout << ">>> Current Balance: " << bal << " tk" << endl;
        } else if (choice2 == 4) {
          loggedin = false;
          wallet = NULL;
          cout << "Logged out successfully!" << endl;
        }
      }
    }
  }
  return 0;
}
