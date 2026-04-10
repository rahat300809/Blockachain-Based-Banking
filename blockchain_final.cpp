#include<bits/stdc++.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
using namespace std;

class Blockchain;
class Wallet;
string calculate_hash(int index, string data, string previoushash, string timestamp, int nonce);



string calculate_hash(int index,string data,string previoushash,string timestamp,int nonce)
{
   string input=to_string(index)+data+previoushash+timestamp+to_string(nonce);
   long long hash=0;
   for(char c: input)
   {
    hash=(hash*71+c)%1000000009;
   }
   return(to_string(hash));
}


string public_key(string secret,string ts)
{
   return "PUB_" + calculate_hash(0, secret, "DECENTRALIZED_ROOT", ts,0);
}

class Wallet
{
    private:
     string privatekey;
     //string publickey;
     string creation_time;

     public:
     string publickey;

     
     friend void  transfer(Blockchain &bc, Wallet* wallet);
     friend Wallet* create_acc();
     

     public:
      Wallet(string secret)
      {
        this->privatekey=secret;
        time_t now=time(0);
        string ts=ctime(&now);

        if (!ts.empty() && ts.back() == '\n') 
           ts.pop_back();
        this->creation_time = ts;
 
        this->publickey=public_key(this->privatekey,this->creation_time);

        
      }
};

vector<unsigned char> aes_encrypt(string data, string key)
{
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    unsigned char iv[16] = {0};
    unsigned char keyBytes[16] = {0};
    for(int i = 0; i < key.size() && i < 16; i++)
        keyBytes[i] = key[i];
    
    vector<unsigned char> output(data.size() + 16);
    int len = 0, totalLen = 0;
    EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, keyBytes, iv);
    EVP_EncryptUpdate(ctx, output.data(), &len, 
                     (unsigned char*)data.c_str(), data.size());
    totalLen = len;
    EVP_EncryptFinal_ex(ctx, output.data() + len, &len);
    totalLen += len;
    EVP_CIPHER_CTX_free(ctx);
    output.resize(totalLen);
    return output;
}

string aes_decrypt(vector<unsigned char> data, string key)
{
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    unsigned char iv[16] = {0};
    unsigned char keyBytes[16] = {0};
    for(int i = 0; i < key.size() && i < 16; i++)
        keyBytes[i] = key[i];
    
    vector<unsigned char> output(data.size() + 16);
    int len = 0, totalLen = 0;
    EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, keyBytes, iv);
    EVP_DecryptUpdate(ctx, output.data(), &len, data.data(), data.size()); // ← .data() use করো
    totalLen = len;
    EVP_DecryptFinal_ex(ctx, output.data() + len, &len);
    totalLen += len;
    EVP_CIPHER_CTX_free(ctx);
    return string(output.begin(), output.begin() + totalLen);
}






class Node
{
   friend class Blockchain;
   private:
     int index;
     vector<unsigned char> data; 
     string previoushash;
     string nexthash;
     string hash;
     Node* next;
     Node* prev;
     string timestamp;
     int nonce;
     

    Node(int index,vector<unsigned char> data,string previoushash)
    {
        time_t now = time(0);
        this->timestamp = ctime(&now);
        this->index=index;
        this->data=data;
        this->previoushash=previoushash;
        this->nexthash="";
        this->next=NULL;
        this->prev=NULL;
        string dataStr(data.begin(), data.end());
        this->nonce=0;
        this->hash = calculate_hash(index, dataStr, previoushash, timestamp,0);
    }


   
};

class Blockchain
{
  private:
  Node* head;
  Node* tail;
  int size;
  string key;

  public:
   Blockchain(string secretkey)
   {
    head=NULL;
    tail=NULL;
    size=0;
    key=secretkey;
   }

   vector<string> getmerklehashes()
    {
        vector<string>hashes;
        Node* temp=head;
        while(temp!=NULL)
        {
            hashes.push_back(temp->hash);
            temp=temp->next;
        }
        return hashes;
    }

    string merkleroot()
    {
        vector<string>hashes=getmerklehashes();

        while(hashes.size()>1)
        {
            vector<string>newlevel;
            for(int i=0;i<hashes.size();i+=2)
            {
                if(i+1 < hashes.size())
                {
                    newlevel.push_back(calculate_hash(0,hashes[i]+hashes[i+1],"","",0));
                }
                else
                {
                    newlevel.push_back(hashes[i]);
                }
            }
            hashes=newlevel;
        }
        return hashes[0];
    }

   void insert_tail(string data)
   {
    vector<unsigned char> encryptedData = aes_encrypt(data, key);
    string previoshash="";
    if(tail!=NULL)
    {
        previoshash=tail->hash;
    }
    Node* newnode = new Node(size, encryptedData, previoshash);
    

    int difficulty=3;
    string target(difficulty,'0');
    cout <<"Mining block"<<size<<"..."<<endl;
    string datastr(encryptedData.begin(),encryptedData.end());

    while (newnode->hash.substr(0, difficulty) != target) {
        newnode->nonce++; 
        newnode->hash = calculate_hash(newnode->index, datastr, newnode->previoushash, newnode->timestamp, newnode->nonce);
    }

    cout << "Block Mined! Hash: " << newnode->hash << " (Nonce: " << newnode->nonce << ")" << endl;

// ১. তুমি লিখলে "Hello" + ১ = (যোগফল ৫) -> হয়নি।
// ২. তুমি লিখলে "Hello" + ২ = (যোগফল ৯) -> হয়নি।
// ৩. তুমি লিখলে "Hello" + ৩ = (যোগফল ৭) -> Bingo! মাইন হয়ে গেছে।

    if(head==NULL)
    {
        head=newnode;
        tail=newnode;
        size++;
        return;
    }
    tail->nexthash=newnode->hash;
    tail->next=newnode;
    newnode->prev=tail;
    newnode->previoushash=tail->hash;
    tail=newnode;
    size++;
   }

   void display()
   {
    Node* temp=head;
    if(head==NULL)
    {
        cout<<" NO DATA";
        return;
    }
    while(temp!=NULL)
    {
        
        string plaintext = aes_decrypt(temp->data, key); 
        cout << "Data: " << plaintext << endl;
        cout << "Hash: " << temp->hash << " | Index: " << temp->index << endl;
        temp = temp->next;
        cout << endl;
    }
    
   }

   void isvalid()
   {
    Node* temp=head;
    if(head==NULL)
    {
        return;
    }
    while(temp!=NULL)
    {
        string dataStr(temp->data.begin(), temp->data.end());
        
        if(temp->hash != calculate_hash(temp->index, dataStr, temp->previoushash, temp->timestamp,0))        {
            cout<<"tampered"<<endl;
            writelog("tamperted at block"+to_string(temp->index));
            return;
        }
        if(temp->prev!=NULL && temp->previoushash!=temp->prev->hash)
        {
            cout<<"tampered";
            writelog("tamperted at block"+to_string(temp->index));
            return;
        }
        temp=temp->next;
    }
    cout<<"chain valid"<<endl;
    writelog("Chain Valid - All OK");
   }

   void writelog(string message)
   {
    ofstream file("audit_log.txt",ios::app);
    time_t now=time(0);
    file<<ctime(&now)<<"_"<<message<<"\n\n";
    file.close();
   }
   
};

Wallet* create_acc()
{
    string secret;
    cout << "\n========== ACCOUNT CREATION ==========" << endl;
    cout << "Enter a Secret (Private Key): ";
    cin >> secret;

    Wallet* new_wallet=new Wallet(secret);

    cout << "\n>>> [SUCCESS] Account Initialized!" << endl;
    cout << ">>> Your Public Address: " << new_wallet->publickey << endl;
    cout << ">>> Created At: " << new_wallet->creation_time << endl;
    cout << "======================================\n" << endl;

    return new_wallet;
    
}


void transfer(Blockchain &bc, Wallet* wallet)
{
    string receiver,amount,txn;
    cout << "\n========== SEND TRANSACTION ==========" << endl;
    cout << "Enter Receiver's Public Address: ";
    cin >> receiver;
    cout << "Enter Amount: ";
    cin >> amount;
    // cout << "Enter Transaction ID (e.g. TXN001): ";
    // cin >> txn;

    time_t now=time(0);
    string ts=ctime(&now);
    if(!ts.empty() && ts.back()=='\n')
       ts.pop_back();
    

       string rawdata = wallet->publickey+receiver+amount+ts;
       string gentxn="TXN"+calculate_hash(0,rawdata,"TXN_SECURITY",ts,0);
       string tnxid=wallet->publickey+"|"+receiver+"|"+amount+"|"+gentxn;
       bc.insert_tail(tnxid);

       cout << "\n>>> [SUCCESS] Transaction Recorded!" << endl;
    cout << ">>> Txn ID: " << tnxid << endl;
    cout << "======================================\n" << endl;
}


int main() 
{


    string key;
    cout << "--- Network Initialization ---" << endl;
    
    cout<<"set your secretkey - ; ";
    cin>>key;
    key.resize(16, '0');
    Blockchain bc(key);

   Wallet* wallet=NULL;
    int choice;
   while(true)
   {
        cout << "\n========== BLOCKCHAIN MENU ==========" << endl;
        cout << "1. Create New Account (Wallet)" << endl;
        cout << "2. Send Transaction" << endl;
        cout << "3. View Blockchain Ledger" << endl;
        cout << "4. Exit" << endl;
        cout << "Enter your choice: ";
        
        cin>>choice;


        if(choice==1)
        {
            wallet = create_acc();
        }
        else if(choice==2)
        {
            if(wallet==NULL)
            {
                cout<<"Error: Create a wallet first!" << endl;
                wallet = create_acc();
            }
            transfer(bc,wallet);
        }
        else if(choice==3)
        {
            bc.display();
            bc.isvalid();
        }
        else if(choice==4)
        {
            break;
        }
        else
        {
            cout << "Invalid choice!" << endl;
        }


   }

    // bc.insert_tail("Rahat|karim|500|TXN001");
    // bc.insert_tail("Karim|Sajid|200|TXN002");
    // bc.insert_tail("Sajid|Rahat|100|TXN003");
    // bc.display();
    // bc.isvalid();
    
    // cout << "Merkle Root: " << bc.merkleroot() << endl;
    // bc.insert_tail("Karim|Sajid|9999|TXN002");  // 200 → 9999
    // cout << "Merkle Root: " << bc.merkleroot() << endl;
    // return 0;
}


// g++   b lockchain_encrypted.cpp -o blockchain_encrypted -IC:/msys64/mingw64/include -LC:/msys64/mingw64/lib -lssl -lcrypto
// .\blockchain_encrypted  invalid now



// g++ blockchain_final.cpp -o blockchain_final -IC:/msys64/mingw64/include -LC:/msys64/mingw64/lib -lssl -lcrypto

// .\blockchain_final