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


// ── Global Constants & Flags ─────────────────────────────────

// blockchain.dat ফাইল এই key দিয়ে encrypt/decrypt হবে
const string INTERNAL_KEY = "only_rahat_opens";

// true থাকলে file থেকে load হচ্ছে — তখন আবার save করবে না
bool LOADING_MODE = false;


// ── Forward Declarations ──────────────────────────────────────
// নিচে define হওয়া জিনিস আগে use করতে হলে এগুলো দরকার

class Blockchain;
class Wallet;
class TxnStack;

string calculate_hash(int index, string data, string previoushash,
                      string timestamp, int nonce);
void   save_blockchain(vector<unsigned char> data);


// ================================================================
//  HASH FUNCTION
//  প্রতিটা block-এর unique identity এই hash থেকে তৈরি হয়
// ================================================================

string calculate_hash(int index, string data, string previoushash,
                      string timestamp, int nonce)
{
    // সব input একসাথে জোড়া লাগিয়ে একটা string বানানো হচ্ছে
    string input = to_string(index) + data + previoushash + timestamp + to_string(nonce);

    long long hash = 0;

    // প্রতিটা character-এর ASCII value দিয়ে rolling hash তৈরি
    // prime number (71, 1000000009) দিয়ে mod — collision কমায়
    for (char c : input)
    {
        hash = (hash * 71 + c) % 1000000009;
    }

    string result = to_string(hash);

    // hash সবসময় 10 digit হবে — দরকার হলে আগে '0' বসাবে
    while (result.length() < 10)
    {
        result = "0" + result;
    }

    return result;
}


// ================================================================
//  PUBLIC KEY GENERATOR
//  secret + enc_key মিলিয়ে একটা unique public address বানায়
// ================================================================

string public_key(string secret, string enc_key)
{
    string combined = secret + ":" + enc_key;

    // "PUB_" prefix দেখলেই বোঝা যাবে এটা public address
    return "PUB_" + calculate_hash(0, combined, "DECENTRALIZED_ROOT", "STABLE_ID", 0);
}


// ================================================================
//  TXNSTACK CLASS
//  user-এর transaction history stack হিসেবে রাখা হয়
//  (push only — pop নেই, history delete হবে না)
// ================================================================

class TxnStack
{
    private:
        vector<string> data; // সব transaction string এখানে জমা থাকে

    public:

        // নতুন transaction যোগ করা
        void push(string txn)
        {
            data.push_back(txn);
        }

        // সবচেয়ে নতুন transaction আগে দেখাবে (reverse order)
        void display()
        {
            cout << "\n=== STACK TRANSACTION HISTORY ===\n";

            for (int i = (int)data.size() - 1; i >= 0; i--)
            {
                cout << data[i] << endl;
                cout << "----------------------\n";
            }
        }

        int size()
        {
            return (int)data.size();
        }
};


// ================================================================
//  WALLET CLASS
//  প্রতিটা user-এর account এই class দিয়ে represent করা হয়
// ================================================================

class Wallet
{
    private:
        string privatekey;      // user-এর secret — কাউকে দেওয়া যাবে না
        string creation_time;   // wallet কখন তৈরি হয়েছিল
        string encryption_key;  // transaction data encrypt করার key

    public:
        string publickey;       // public address — এটা share করা যায়

        // এই functions গুলো Wallet-এর private member access করতে পারবে
        friend void    transfer(Blockchain &bc, Wallet *wallet);
        friend Wallet* create_acc(Blockchain &bc);
        friend Wallet* login_wallet(Blockchain &bc);
        friend Wallet* create_agent(Blockchain &bc);

        // Constructor
        // secret ও enc_key দিয়ে wallet তৈরি হয়
        // ts (timestamp) না দিলে automatically এখনকার time নেবে
        Wallet(string secret, string enc_key, string ts = "")
        {
            this->privatekey      = secret;
            this->encryption_key  = enc_key;

            if (ts == "")
            {
                time_t now  = time(0);
                char*  dt   = ctime(&now);
                string temp_ts = dt ? dt : "";

                // ctime() শেষে '\n' দেয় — সেটা সরিয়ে ফেলছি
                if (!temp_ts.empty() && temp_ts.back() == '\n')
                    temp_ts.pop_back();

                this->creation_time = temp_ts;
            }
            else
            {
                this->creation_time = ts;
            }

            // public address তৈরি করা হচ্ছে secret + enc_key থেকে
            this->publickey = public_key(this->privatekey, this->encryption_key);
        }

        string getCreationTime()  { return creation_time; }
        string getPublicKey()     { return publickey; }
        string getencyptionkey()  { return encryption_key; }
        string getPrivateKey()    { return privatekey; }
};


// ================================================================
//  AES ENCRYPT
//  string data কে AES-128-CBC দিয়ে encrypt করে
//  return করে vector<unsigned char> (binary data)
// ================================================================

vector<unsigned char> aes_encrypt(string data, string key)
{
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    // IV (Initialization Vector) — সব zero দিয়ে fix করা আছে
    unsigned char iv[16]       = {0};
    unsigned char keyBytes[16] = {0};

    // key string থেকে প্রথম 16 byte নেওয়া হচ্ছে
    for (int i = 0; i < (int)key.size() && i < 16; i++)
        keyBytes[i] = key[i];

    vector<unsigned char> output(data.size() + 16); // extra 16 byte padding এর জন্য
    int len = 0, totalLen = 0;

    EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, keyBytes, iv);
    EVP_EncryptUpdate(ctx, output.data(), &len,
                      (unsigned char*)data.c_str(), (int)data.size());
    totalLen = len;

    EVP_EncryptFinal_ex(ctx, output.data() + len, &len); // final padding block
    totalLen += len;

    EVP_CIPHER_CTX_free(ctx);
    output.resize(totalLen); // actual size অনুযায়ী resize
    return output;
}


// ================================================================
//  AES DECRYPT
//  encrypted binary data কে আবার string-এ ফিরিয়ে আনে
//  fail হলে "DECRYPT_ERROR" return করে — crash করে না
// ================================================================

string aes_decrypt(vector<unsigned char> data, string key)
{
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    unsigned char iv[16]       = {0};
    unsigned char keyBytes[16] = {0};

    for (int i = 0; i < (int)key.size() && i < 16; i++)
        keyBytes[i] = key[i];

    vector<unsigned char> output(data.size() + 16);
    int len = 0, totalLen = 0;

    EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, keyBytes, iv);
    EVP_DecryptUpdate(ctx, output.data(), &len, data.data(), (int)data.size());
    totalLen = len;

    // Final step fail হলে মানে key ভুল ছিল
    if (EVP_DecryptFinal_ex(ctx, output.data() + len, &len) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        return "DECRYPT_ERROR"; // ভুল key হলে crash না করে এটা return করবে
    }

    totalLen += len;
    EVP_CIPHER_CTX_free(ctx);
    return string(output.begin(), output.begin() + totalLen);
}


// ================================================================
//  NODE CLASS
//  Blockchain-এর প্রতিটা block এই class দিয়ে represent করা হয়
//  Doubly Linked List — next ও prev দুই দিকেই pointer আছে
// ================================================================

class Node
{
    // এই class গুলো Node-এর private member সরাসরি access করতে পারবে
    friend class Blockchain;
    friend void    show_all_wallet(Blockchain &bc);
    friend Wallet* login_wallet(Blockchain &bc);
    friend int     get_balance(Blockchain &bc, string pub);
    friend bool    wallet_exist(Blockchain &bc, string pub);
    friend TxnStack build_stack(Blockchain &bc, Wallet *wallet);

    private:
        int                    index;        // block number (0, 1, 2...)
        vector<unsigned char>  data;         // encrypted block data
        string                 previoushash; // আগের block-এর hash
        string                 nexthash;     // পরের block-এর hash
        string                 hash;         // এই block-এর নিজের hash
        Node*                  next;         // পরের block-এর pointer
        Node*                  prev;         // আগের block-এর pointer
        string                 timestamp;    // কখন তৈরি হয়েছে
        int                    nonce;        // mining-এ use হওয়া counter

        // Constructor — নতুন block তৈরির সময় call হয়
        Node(int index, vector<unsigned char> data, string previoushash)
        {
            time_t now = time(0);
            char*  dt  = ctime(&now);
            this->timestamp = dt ? dt : "";

            if (!timestamp.empty() && timestamp.back() == '\n')
                timestamp.pop_back();

            this->index        = index;
            this->data         = data;
            this->previoushash = previoushash;
            this->nexthash     = "";
            this->next         = NULL;
            this->prev         = NULL;
            this->nonce        = 0;

            // প্রাথমিক hash তৈরি (mining এর আগে)
            string dataStr(data.begin(), data.end());
            this->hash = calculate_hash(index, dataStr, previoushash, timestamp, 0);
        }
};


// ================================================================
//  SECRET KEY GENERATOR
//  random alphanumeric + symbol দিয়ে একটা strong key তৈরি করে
// ================================================================

string generate_secret_key(int length = 32)
{
    const string chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";

    random_device rd;                               // hardware random seed
    mt19937 gen(rd());                              // Mersenne Twister engine
    uniform_int_distribution<> dis(0, (int)chars.size() - 1);

    string key = "";
    for (int i = 0; i < length; i++)
    {
        key = key + chars[dis(gen)]; // random character বাছাই করা
    }

    return key;
}


// ================================================================
//  BLOCKCHAIN CLASS
//  মূল chain — সব block এখানে doubly linked list হিসেবে থাকে
// ================================================================

class Blockchain
{
    // এই functions গুলো Blockchain-এর private member access করতে পারবে
    friend void    show_all_wallet(Blockchain &bc);
    friend int     get_balance(Blockchain &bc, string pub);
    friend TxnStack build_stack(Blockchain &bc, Wallet *wallet);
    friend void    load_blockchain(Blockchain &bc);
    friend Wallet* login_wallet(Blockchain &bc);
    friend void    transfer(Blockchain &bc, Wallet *wallet);
    friend Wallet* create_acc(Blockchain &bc);
    friend Wallet* create_agent(Blockchain &bc);
    friend bool    wallet_exist(Blockchain &bc, string pub);

    private:
        Node* head; // chain-এর প্রথম block
        Node* tail; // chain-এর শেষ block
        int   size; // মোট block সংখ্যা

    public:

        Blockchain()
        {
            head = NULL;
            tail = NULL;
            size = 0;
        }

        Node* getHead() { return head; }

        // সব block-এর hash একটা vector-এ দেয় — Merkle Tree-এর জন্য দরকার
        vector<string> getmerklehashes()
        {
            vector<string> hashes;
            Node* temp = head;

            while (temp != NULL)
            {
                hashes.push_back(temp->hash);
                temp = temp->next;
            }

            return hashes;
        }

        // Merkle Root বের করে
        // সব hash জোড়ায় জোড়ায় মিলিয়ে একটা single root hash বানায়
        string merkleroot()
        {
            vector<string> hashes = getmerklehashes();

            // যতক্ষণ একটার বেশি hash আছে, জোড়া মিলিয়ে উপরে উঠতে থাকবে
            while (hashes.size() > 1)
            {
                vector<string> newlevel;

                for (int i = 0; i < (int)hashes.size(); i += 2)
                {
                    if (i + 1 < (int)hashes.size())
                    {
                        // দুটো hash জোড়া লাগিয়ে নতুন hash
                        newlevel.push_back(
                            calculate_hash(0, hashes[i] + hashes[i + 1], "", "", 0));
                    }
                    else
                    {
                        // বেজোড় হলে শেষেরটা এমনিই উপরে যাবে
                        newlevel.push_back(hashes[i]);
                    }
                }

                hashes = newlevel;
            }

            return hashes[0]; // এটাই Merkle Root
        }

        // নতুন block chain-এর শেষে যোগ করে
        void insert_tail(vector<unsigned char> encryptedData)
        {
            string previoshash = "0"; // genesis block-এর আগে কিছু নেই

            if (tail != NULL)
                previoshash = tail->hash; // আগের block-এর hash নিচ্ছি

            Node* newnode = new Node(size, encryptedData, previoshash);
            string datastr(encryptedData.begin(), encryptedData.end());

            if (LOADING_MODE)
            {
                // file থেকে load হলে mining লাগবে না — সরাসরি hash সেট
                newnode->nonce = 0;
                newnode->hash  = calculate_hash(newnode->index, datastr,
                                                newnode->previoushash,
                                                newnode->timestamp, 0);
            }
            else
            {
                // ── Proof of Work Mining ──────────────────────────────
                // hash-এর প্রথম DIFFICULTY টা character '0' হতে হবে
                int DIFFICULTY = 3;
                cout << "Mining block " << size << "..." << endl;

                while (true)
                {
                    newnode->hash = calculate_hash(newnode->index, datastr,
                                                   newnode->previoushash,
                                                   newnode->timestamp,
                                                   newnode->nonce);

                    // প্রথম DIFFICULTY টা character check করা
                    bool ok = true;
                    for (int i = 0; i < DIFFICULTY && i < (int)newnode->hash.length(); i++)
                    {
                        if (newnode->hash[i] != '0')
                        {
                            ok = false;
                            break;
                        }
                    }

                    if (ok) break; // ধাঁধা মিলেছে — mining শেষ!

                    newnode->nonce++; // না মিললে nonce বাড়িয়ে আবার try

                    if (newnode->nonce % 100000 == 0)
                        cout << "Mining... Nonce: " << newnode->nonce << endl;
                }

                cout << "Block Mined! Hash: " << newnode->hash
                     << " (Nonce: " << newnode->nonce << ")" << endl;
            }

            // ── Chain-এ যোগ করা ──────────────────────────────────────
            if (head == NULL)
            {
                // প্রথম block (Genesis Block)
                head = newnode;
                tail = newnode;
                size++;

                if (!LOADING_MODE)
                    save_blockchain(encryptedData);

                return;
            }

            // পুরনো tail-এর সাথে নতুন node জোড়া লাগানো
            tail->nexthash     = newnode->hash;
            tail->next         = newnode;
            newnode->prev      = tail;
            newnode->previoushash = tail->hash; // backward link
            tail               = newnode;        // tail আপডেট
            size++;

            if (!LOADING_MODE)
                save_blockchain(encryptedData);
        }

        // blockchain-এর সব block দেখায়
        // user_key দিয়ে নিজের transaction decrypt করার চেষ্টা করে
        void display(string user_key)
        {
            Node* temp = head;

            while (temp != NULL)
            {
                cout << "Block: " << temp->index
                     << " | Hash: " << temp->hash << endl;

                // Step 1: INTERNAL_KEY দিয়ে outer layer decrypt
                string step1 = aes_decrypt(temp->data, INTERNAL_KEY);

                if (step1 != "DECRYPT_ERROR")
                {
                    // Step 2: user-এর enc_key দিয়ে inner layer decrypt
                    string final = aes_decrypt(
                        vector<unsigned char>(step1.begin(), step1.end()),
                        user_key);

                    if (final != "DECRYPT_ERROR")
                        cout << "Decrypted Data: " << final << endl;
                    else
                        cout << "Data: [NOT YOUR TRANSACTION]" << endl;
                }
                else
                {
                    cout << "Data: [CORRUPTED]" << endl;
                }

                temp = temp->next;
                cout << "-------------------------" << endl;
            }
        }

        // chain-এর integrity check করে
        // প্রতিটা block-এর hash আবার calculate করে মেলায়
        void isvalid()
        {
            Node* temp = head;
            if (temp == NULL) return;

            while (temp != NULL)
            {
                string dataStr(temp->data.begin(), temp->data.end());

                // stored hash vs নতুন calculate করা hash
                string recalculate_hash = calculate_hash(temp->index, dataStr,
                                                         temp->previoushash,
                                                         temp->timestamp,
                                                         temp->nonce);

                if (temp->hash != recalculate_hash)
                {
                    cout << "tampered" << endl;
                    writelog("tamperted at block" + to_string(temp->index));
                    return;
                }

                // backward link check — আগের block-এর hash মেলে কিনা
                if (temp->prev != NULL && temp->previoushash != temp->prev->hash)
                {
                    cout << "tampered" << endl;
                    writelog("tamperted at block" + to_string(temp->index));
                    return;
                }

                temp = temp->next;
            }

            cout << "chain valid after checking all possible arguments" << endl;
            writelog("Chain Valid - All OK");
        }

        // audit_log.txt ফাইলে timestamp সহ message লেখে
        void writelog(string message)
        {
            ofstream file("audit_log.txt", ios::app);
            time_t now = time(0);
            char*  dt  = ctime(&now);

            if (dt)
                file << dt << "_" << message << "\n\n";

            file.close();
        }
};


// ================================================================
//  HELPER FUNCTIONS (Global)
// ================================================================

// wallet info .txt ফাইলে backup করে রাখে
void save_to_file(string filename, string pub, string sec, string enc, string ts)
{
    string   full_path = filename + ".txt";
    ofstream file(full_path);

    if (file.is_open())
    {
        file << "--- WALLET BACKUP ---" << endl;
        file << "Public Address : " << pub << endl;
        file << "Login Secret   : " << sec << endl;
        file << "Encryption Key : " << enc << endl;
        file << "Created At     : " << ts  << endl;
        file << "---------------------" << endl;
        file.close();
        cout << ">>> Backup created: " << full_path << endl;
    }
}

// নতুন wallet তৈরি হলে system থেকে 100 tk bonus দেওয়া হয়
void give_signup_bonus(Blockchain &bc, string pub)
{
    // format: SYSTEM|<receiver_pub>|<amount>
    string bonus = "SYSTEM|" + pub + "|100";
    vector<unsigned char> dataVec(bonus.begin(), bonus.end());
    bc.insert_tail(dataVec);
    cout << ">>> Signup Bonus: 100 tk added!" << endl;
}

// এই দুটো function নিচে define হবে — তার আগে declare করা দরকার
int  get_balance(Blockchain &bc, string pub);
bool wallet_exist(Blockchain &bc, string pub);


// ================================================================
//  CREATE ACCOUNT (User Registration)
// ================================================================

Wallet* create_acc(Blockchain &bc)
{
    string secret, enc_key, user_filename;

    cout << "\n========== ACCOUNT CREATION ==========" << endl;
    cout << "Step 1: Set a Transaction pin : ";
    cin  >> enc_key;
    cout << "Step 2: Filename for this Wallet backup: ";
    cin  >> user_filename;

    // secret key auto generate হবে — user কে type করতে হবে না
    secret = generate_secret_key();
    cout << ">>> Auto Generated Secret Key: " << secret << endl;

    Wallet* new_wallet      = new Wallet(secret, enc_key);
    string  public_id_record = new_wallet->publickey;

    // backup file তৈরি করা
    save_to_file(user_filename, public_id_record, secret, enc_key,
                 new_wallet->getCreationTime());

    // blockchain-এ wallet record যোগ করা
    // format: WALLET|<pub>|<creation_time>
    string wallet_record = "WALLET|" + public_id_record + "|" + new_wallet->getCreationTime();
    vector<unsigned char> dataVec(wallet_record.begin(), wallet_record.end());
    bc.insert_tail(dataVec);

    cout << "\n>>> [SUCCESS] Account Initialized!" << endl;
    cout << ">>> Your Public Address: " << public_id_record << endl;
    cout << ">>> Created At: " << new_wallet->getCreationTime() << endl;
    cout << ">>> Please login to access your wallet." << endl;
    cout << "======================================\n" << endl;

    give_signup_bonus(bc, public_id_record); // signup bonus দেওয়া
    return new_wallet;
}


// ================================================================
//  CREATE AGENT ACCOUNT
//  Agent wallet-এর public key-এ "_AGENT" suffix থাকে
// ================================================================

Wallet* create_agent(Blockchain &bc)
{
    string secret, enc_key, filename;

    cout << "\n====== CREATE AGENT ACCOUNT ======" << endl;

    secret = generate_secret_key();
    cout << ">>> Agent Secret Key: " << secret << endl;

    cout << "Enter pin Key: ";
    cin  >> enc_key;
    cout << "Enter Filename: ";
    cin  >> filename;

    // agent-এর public key-এ "_AGENT" লাগানো হচ্ছে
    string base_pub  = public_key(secret, enc_key);
    string agent_pub = base_pub + "_AGENT";

    save_to_file(filename, agent_pub, secret, enc_key, "AGENT");

    // blockchain-এ agent record যোগ করা
    // format: AGENT|<agent_pub>|CREATED
    string record = "AGENT|" + agent_pub + "|CREATED";
    vector<unsigned char> dataVec(record.begin(), record.end());
    bc.insert_tail(dataVec);

    cout << ">>> [SUCCESS] Agent Account Created!" << endl;
    cout << ">>> Public Key: " << agent_pub << endl;

    give_signup_bonus(bc, agent_pub);
    return new Wallet(secret, enc_key);
}


// ================================================================
//  TRANSFER FUNCTION
//  sender থেকে receiver-এ amount পাঠায়
//  double encryption — user key + INTERNAL_KEY
// ================================================================

void transfer(Blockchain &bc, Wallet* wallet)
{
    string receiver, amount, input_secret;

    cout << "\n========== SEND TRANSACTION ==========" << endl;
    cout << "Enter Receiver's Public Address: ";
    cin  >> receiver;
    cout << "Enter Amount: ";
    cin  >> amount;
    cout << "Enter your Secret Key to confirm transaction: ";
    cin  >> input_secret;

    // private key verify — ভুল হলে transaction হবে না
    if (input_secret != wallet->getPrivateKey())
    {
        cout << ">>> [ERROR] Unauthorized! Private Key mismatch." << endl;
        return;
    }

    // receiver wallet blockchain-এ আছে কিনা check
    if (!wallet_exist(bc, receiver))
    {
        cout << ">>> [FAILED] Receiver wallet not found!" << endl;
        return;
    }

    // amount শুধু number হতে হবে — letter থাকলে reject
    if (amount.find_first_not_of("0123456789") != string::npos)
    {
        cout << ">>> [ERROR] Invalid amount!" << endl;
        return;
    }

    int amt            = stoi(amount);
    int sender_balance = get_balance(bc, wallet->publickey);

    // balance কম থাকলে transaction হবে না
    if (amt > sender_balance)
    {
        cout << ">>> [FAILED] Insufficient balance!" << endl;
        cout << ">>> Your Balance: " << sender_balance << " tk" << endl;
        return;
    }

    // timestamp নেওয়া
    time_t now = time(0);
    char*  dt  = ctime(&now);
    string ts  = dt ? dt : "";
    if (!ts.empty() && ts.back() == '\n')
        ts.pop_back();

    // transaction payload তৈরি
    // format: sender|receiver|amount|timestamp
    string rawdata    = wallet->publickey + "|" + receiver + "|" + amount + "|" + ts;
    string gentxn     = "TXN" + calculate_hash(0, rawdata, "TXN_SECURITY", ts, 0);
    string signature  = calculate_hash(0, rawdata, "TXN_SECURE", ts, 0);
    string payload    = rawdata + "SIG:" + signature;

    // Double Encryption:
    // Layer 1 → user-এর enc_key দিয়ে encrypt
    vector<unsigned char> userEncrypted = aes_encrypt(payload, wallet->getencyptionkey());

    // Layer 2 → INTERNAL_KEY দিয়ে আবার encrypt
    vector<unsigned char> finalData = aes_encrypt(
        string(userEncrypted.begin(), userEncrypted.end()), INTERNAL_KEY);

    bc.insert_tail(finalData);

    cout << "\n>>> [NETWORK INFO] Verifying transaction integrity..." << endl;
    bc.isvalid();
    cout << "\n>>> [SUCCESS] Transaction Recorded!" << endl;
    cout << ">>> Txn ID: " << gentxn << endl;
    cout << "======================================\n" << endl;
}


// ================================================================
//  LOGIN WALLET
//  file drag-drop অথবা manual secret+key দিয়ে login করা যায়
// ================================================================

Wallet* login_wallet(Blockchain &bc)
{
    int login_choice;

    cout << "\n========== LOGIN MENU ==========" << endl;
    cout << "1. Login via Backup File (Drag & Drop)" << endl;
    cout << "2. Manual Login (Type Secret & Encryption Key)" << endl;
    cout << "Choice: ";
    cin  >> login_choice;

    string secret = "", enc_key = "", filepath = "";

    if (login_choice == 1)
    {
        cout << "Drag and drop your wallet file here or enter full path: " << endl;
        cout << "Path: ";
        cin.ignore(); // আগের '\n' clear করা
        getline(cin, filepath);

        // Windows drag-drop করলে path-এ quotes আসে — সেটা সরানো
        if (!filepath.empty() && filepath.front() == '"')
        {
            filepath.erase(0, 1);
            filepath.erase(filepath.size() - 1);
        }

        ifstream file(filepath);

        if (file.is_open())
        {
            string line;

            while (getline(file, line))
            {
                // backup file থেকে secret ও enc_key পড়া
                if (line.find("Login Secret   :") != string::npos)
                    secret  = line.substr(line.find(":") + 2);
                else if (line.find("Encryption Key :") != string::npos)
                    enc_key = line.substr(line.find(":") + 2);
            }

            // leading space থাকলে সরিয়ে ফেলা
            secret.erase(0,  secret.find_first_not_of(" "));
            enc_key.erase(0, enc_key.find_first_not_of(" "));

            file.close();

            if (secret == "" || enc_key == "")
            {
                cout << ">>> [ERROR] File format is wrong!" << endl;
                return NULL;
            }
        }
        else
        {
            cout << ">>> [ERROR] Could not open file!" << endl;
            return NULL;
        }
    }
    else if (login_choice == 2)
    {
        cout << "Enter your Secret Key: ";
        cin  >> secret;
        cout << "Enter your encryption key: ";
        cin  >> enc_key;
    }

    // user-এর public key তৈরি করে blockchain-এ খোঁজা
    string user_pub  = public_key(secret, enc_key);
    string agent_pub = user_pub + "_AGENT";

    Node* temp = bc.getHead();

    while (temp != NULL)
    {
        string dataStr(temp->data.begin(), temp->data.end());

        // আগে agent check, তারপর regular wallet check
        if (dataStr.find("AGENT|" + agent_pub) == 0)
        {
            cout << ">>> AGENT LOGIN SUCCESS" << endl;
            Wallet* w = new Wallet(secret, enc_key);
            w->publickey = agent_pub; // agent-এর public key আলাদা
            return w;
        }

        if (dataStr.find("WALLET|" + user_pub) == 0)
        {
            cout << ">>> USER LOGIN SUCCESS" << endl;
            Wallet* w = new Wallet(secret, enc_key);
            w->publickey = user_pub;
            return w;
        }

        temp = temp->next;
    }

    cout << ">>> LOGIN FAILED" << endl;
    return NULL;
}


// ================================================================
//  SHOW ALL WALLETS
//  chain scan করে সব wallet address print করে
// ================================================================

void show_all_wallet(Blockchain &bc)
{
    Node* temp = bc.getHead();

    cout << endl << "__All Accounts__" << endl;

    if (temp == NULL)
    {
        cout << endl << "No Wallet Found" << endl;
        return;
    }

    while (temp != NULL)
    {
        // INTERNAL_KEY দিয়ে decrypt করে WALLET| দিয়ে শুরু কিনা দেখা
        string step1 = aes_decrypt(temp->data, INTERNAL_KEY);

        if (step1 != "DECRYPT_ERROR" && step1.find("WALLET|") == 0)
        {
            int first  = step1.find("|");
            int second = step1.find("|", first + 1);
            string pub = step1.substr(first + 1, second - first - 1);
            cout << "Wallet Address: " << pub << endl;
        }

        temp = temp->next;
    }

    cout << "=====================" << endl;
}


// ================================================================
//  SAVE BLOCKCHAIN TO FILE
//  নতুন block-এর data INTERNAL_KEY দিয়ে encrypt করে
//  blockchain.dat ফাইলে binary হিসেবে append করে
// ================================================================

void save_blockchain(vector<unsigned char> data)
{
    string plain(data.begin(), data.end());
    vector<unsigned char> encrypted = aes_encrypt(plain, INTERNAL_KEY);

    ofstream file("blockchain.dat", ios::app | ios::binary);

    // প্রথমে data-র size লেখা, তারপর actual data
    int size = (int)encrypted.size();
    file.write((char*)&size,             sizeof(size));
    file.write((char*)encrypted.data(),  size);
    file.flush(); // disk-এ নিশ্চিত করা
    file.close();
}


// ================================================================
//  LOAD BLOCKCHAIN FROM FILE
//  program start-এ blockchain.dat পড়ে chain rebuild করে
// ================================================================

void load_blockchain(Blockchain &bc)
{
    ifstream file("blockchain.dat", ios::binary);

    if (!file.is_open())
    {
        cout << "no file found" << endl;
        return;
    }

    LOADING_MODE = true; // save হবে না এই mode-এ

    while (true)
    {
        int size;

        // file শেষ হলে loop break
        if (!file.read((char*)&size, sizeof(size)))
            break;

        // encrypted data পড়া
        vector<unsigned char> encrypted(size);
        file.read((char*)encrypted.data(), size);

        // decrypt করে chain-এ যোগ করা
        string decrypted = aes_decrypt(encrypted, INTERNAL_KEY);
        vector<unsigned char> data(decrypted.begin(), decrypted.end());
        bc.insert_tail(data);
    }

    LOADING_MODE = false;
    file.close();
}


// ================================================================
//  GET BALANCE
//  chain scan করে একটা wallet-এর current balance বের করে
//  received amount যোগ করে, sent amount বাদ দেয়
// ================================================================

int get_balance(Blockchain &bc, string pub)
{
    Node* temp    = bc.getHead();
    int   balance = 0;

    while (temp != NULL)
    {
        // INTERNAL_KEY দিয়ে decrypt করা
        string dStr = aes_decrypt(temp->data, INTERNAL_KEY);

        if (dStr == "DECRYPT_ERROR")
        {
            temp = temp->next;
            continue;
        }

        if (dStr.find("SYSTEM|") == 0)
        {
            // System bonus transaction
            // format: SYSTEM|<receiver>|<amount>
            int p1       = (int)dStr.find("|");
            int p2       = (int)dStr.find("|", p1 + 1);
            string recv  = dStr.substr(p1 + 1, p2 - p1 - 1);
            int    amount = stoi(dStr.substr(p2 + 1));

            if (recv == pub)
                balance += amount; // bonus পেয়েছে
        }
        else if (dStr.find("|") != string::npos)
        {
            // Regular transaction
            // format: sender|receiver|amount|timestamp
            int p1 = (int)dStr.find("|");
            int p2 = (int)dStr.find("|", p1 + 1);
            int p3 = (int)dStr.find("|", p2 + 1);

            if (p1 != (int)string::npos &&
                p2 != (int)string::npos &&
                p3 != (int)string::npos)
            {
                string sender   = dStr.substr(0, p1);
                string receiver = dStr.substr(p1 + 1, p2 - p1 - 1);
                int    amount   = stoi(dStr.substr(p2 + 1, p3 - p2 - 1));

                if (receiver == pub) balance += amount; // টাকা পেয়েছে
                if (sender   == pub) balance -= amount; // টাকা পাঠিয়েছে
            }
        }

        temp = temp->next;
    }

    return balance;
}


// ================================================================
//  WALLET EXIST CHECK
//  blockchain-এ কোনো wallet/agent address আছে কিনা দেখে
// ================================================================

bool wallet_exist(Blockchain &bc, string pub)
{
    Node* temp = bc.getHead();

    while (temp != NULL)
    {
        string step1 = aes_decrypt(temp->data, INTERNAL_KEY);

        if (step1 != "DECRYPT_ERROR")
        {
            // WALLET বা AGENT যেকোনো একটা match করলেই true
            if (step1.find("WALLET|" + pub) == 0 ||
                step1.find("AGENT|"  + pub) == 0)
            {
                return true;
            }
        }

        temp = temp->next;
    }

    return false;
}


// ================================================================
//  BUILD TXNSTACK
//  wallet-এর সাথে সম্পর্কিত সব transaction TxnStack-এ ভরে দেয়
// ================================================================

TxnStack build_stack(Blockchain &bc, Wallet* wallet)
{
    TxnStack st;
    Node*    temp = bc.getHead();

    while (temp != NULL)
    {
        // Step 1: INTERNAL_KEY দিয়ে outer layer decrypt
        string step1 = aes_decrypt(temp->data, INTERNAL_KEY);

        if (step1 != "DECRYPT_ERROR")
        {
            // Step 2: user-এর enc_key দিয়ে inner layer decrypt
            string final = aes_decrypt(
                vector<unsigned char>(step1.begin(), step1.end()),
                wallet->getencyptionkey());

            // এই wallet-এর publickey থাকলেই stack-এ push
            if (final != "DECRYPT_ERROR" &&
                final.find(wallet->publickey) != string::npos)
            {
                st.push(final);
            }
        }

        temp = temp->next;
    }

    return st;
}


// ================================================================
//  MAIN FUNCTION
// ================================================================

int main()
{
    cout << "--- Network Initialization ---" << endl;

    Blockchain bc;
    load_blockchain(bc); // আগের data load করা

    Wallet* wallet = NULL;
    int     choice, choice2;

    while (true)
    {
        // ── Main Menu ─────────────────────────────────────────
        cout << "\n===== WELCOME TO BLOCKCHAIN NETWORK =====" << endl;
        cout << "1. Create New Wallet (user Register)" << endl;
        cout << "2. Create Agent Account" << endl;
        cout << "3. Login to Existing Wallet" << endl;
        cout << "4. Exit" << endl;
        cout << "5. Show All Accounts" << endl;
        cout << "Choice: ";
        cin  >> choice;

        if (choice == 1)
        {
            create_acc(bc);
            continue;
        }
        else if (choice == 2)
        {
            create_agent(bc);
            continue;
        }
        else if (choice == 3)
        {
            wallet = login_wallet(bc);
        }
        else if (choice == 4)
        {
            break; // program বন্ধ
        }
        else if (choice == 5)
        {
            show_all_wallet(bc);
            continue;
        }

        // ── User Dashboard (login-এর পরে) ────────────────────
        if (wallet != NULL)
        {
            cout << "logged in as: " << wallet->publickey << endl;
            bool loggedin = true;

            while (loggedin)
            {
                cout << "\n---------- USER DASHBOARD ----------" << endl;
                cout << "1. Send Transaction (Money Transfer)" << endl;
                cout << "2. View Transaction History (Ledger)" << endl;
                cout << "3. Check Balance" << endl;
                cout << "4. Logout" << endl;
                cout << "Choice: ";
                cin  >> choice2;

                if (choice2 == 1)
                {
                    transfer(bc, wallet);
                }
                else if (choice2 == 2)
                {
                    // TxnStack বানিয়ে display করা
                    TxnStack st = build_stack(bc, wallet);
                    st.display();
                }
                else if (choice2 == 3)
                {
                    cout << "Checking Balance..." << endl;
                    int bal = get_balance(bc, wallet->publickey);
                    cout << ">>> Current Balance: " << bal << " tk" << endl;
                }
                else if (choice2 == 4)
                {
                    loggedin = false;
                    wallet   = NULL;
                    cout << "Logged out successfully!" << endl;
                }
            }
        }
    }

    return 0;
}