#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <openssl/evp.h>
#include <openssl/aes.h>
using namespace std;
const string INTERNAL_KEY = "only_rahat_opens";
void save_blockchain(vector<unsigned char>data);
bool LOADING_MODE = false;

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


string public_key(string secret,string enc_key)
{
    string combined=secret + ":" + enc_key;
   return "PUB_" + calculate_hash(0, combined, "DECENTRALIZED_ROOT", "STABLE_ID",0);
}

class Wallet
{
    private:
     string privatekey;
     string creation_time;
     string encryption_key;
 
     public:
     string publickey;

     
     friend void  transfer(Blockchain &bc, Wallet* wallet);
     friend Wallet* create_acc(Blockchain &bc);
     friend Wallet* login_wallet(Blockchain &bc);
     

     public:
      Wallet(string secret, string enc_key,string ts = "")
    {
        this->privatekey = secret;
        this->encryption_key=enc_key;
        if (ts == "") {
            time_t now = time(0);
            string temp_ts = ctime(&now);
            if (!temp_ts.empty() && temp_ts.back() == '\n') temp_ts.pop_back();
            this->creation_time = temp_ts;
        } else {
            this->creation_time = ts;
        }
        this->publickey = public_key(this->privatekey, this->encryption_key);
    }
    string getCreationTime() 
    { 
        return creation_time;
    }
    string getPublicKey() 
    { 
        return publickey; 
    }
    string getencyptionkey()
    {
        return encryption_key;
    }
    string getPrivateKey()
    {
        return privatekey;
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
    if(EVP_DecryptFinal_ex(ctx, output.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return "DECRYPT_ERROR"; // ভুল কি হলে যেন ক্র্যাশ না করে
    }
    //EVP_DecryptFinal_ex(ctx, output.data() + len, &len);
    totalLen += len;
    EVP_CIPHER_CTX_free(ctx);
    return string(output.begin(), output.begin() + totalLen);
}






class Node
{
   friend class Blockchain;
   
   friend void show_all_wallet(Blockchain &bc);
   friend Wallet* login_wallet(Blockchain &bc);

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

        if (!timestamp.empty() && timestamp.back() == '\n') 
          timestamp.pop_back();
          
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
  
  public:
   Blockchain()
   {
    head=NULL;
    tail=NULL;
    size=0;
    //key=secretkey;
   }
    Node* getHead() 
    { 
        return head; 
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

   void insert_tail(vector<unsigned char> encryptedData)
   {
    
    string previoshash="0";
    if(tail!=NULL)
    {
        previoshash=tail->hash;
    }
    Node* newnode = new Node(size, encryptedData, previoshash);
    

    // int difficulty=1;
    // string target(difficulty,'0');
    // cout <<"Mining block"<<size<<"..."<<endl;
    // string datastr(encryptedData.begin(),encryptedData.end());

    // while (stoll(newnode->hash) > 100000000) {
    //     newnode->nonce++; 
    //     if(newnode->nonce % 1000000 == 0) { // প্রতি ৫ লাখ বার ট্রাই করার পর আপডেট দিবে
    //     cout << "Still Mining... Current Nonce: " << newnode->nonce << endl;
    // }
    //     newnode->hash = calculate_hash(newnode->index, datastr, newnode->previoushash, newnode->timestamp, newnode->nonce);
    // }

    long long target_limit = 100000000; 
    
    cout << "Mining block " << size << "..." << endl;
    string datastr(encryptedData.begin(), encryptedData.end());

    // লুপ শুরু
    while (true) {
        // নতুন হাশ জেনারেট করা
        newnode->hash = calculate_hash(newnode->index, datastr, newnode->previoushash, newnode->timestamp, newnode->nonce);
        
        // হাশকে সংখ্যায় রূপান্তর করে চেক করা
        if (stoll(newnode->hash) <= target_limit) {
            break; // ধাঁধা মিলে গেছে!
        }

        newnode->nonce++; 

        if(newnode->nonce % 1000000 == 0) {
            cout << "Still Mining... Current Nonce: " << newnode->nonce << endl;
        }
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

      if(!LOADING_MODE)
      {
          save_blockchain(encryptedData);
      }

      return;
   }
    tail->nexthash=newnode->hash;
    tail->next=newnode;
    newnode->prev=tail;
    newnode->previoushash=tail->hash;
    tail=newnode;
    size++;
    if(!LOADING_MODE)
    {
      save_blockchain(encryptedData);
    }  
 }


   void display(string wallet_key = "")
   {
    Node* temp=head;
    if(head==NULL)
    {
        cout<<" NO DATA";
        return;
    }
    while(temp!=NULL)
    {
        cout << "Block: " << temp->index << " | Hash: " << temp->hash << endl;
        //string plaintext = aes_decrypt(temp->data, key); 
        // cout << "Data: " << plaintext << endl;
        // cout << "Hash: " << temp->hash << " | Index: " << temp->index << endl;
        // temp = temp->next;
        // cout << endl;


        string decrypted = aes_decrypt(temp->data, wallet_key); 
        
        if(decrypted != "FAIL") { 
            
            cout << "Decrypted Data: " << decrypted << endl; 
        } else {
             
            cout << "Data: [ENCRYPTED/LOCKED]" << endl; 
        }
        
        temp = temp->next;
        cout << "-------------------------" << endl;
    
    }
    
   }



   void isvalid()
   {
    Node* temp=head;
    if(temp==NULL)
    {
        return;
    }
    while(temp!=NULL)
    {
        string dataStr(temp->data.begin(), temp->data.end());
        
        string recalculate_hash=calculate_hash(temp->index,dataStr,temp->previoushash,temp->timestamp,temp->nonce);
        
        if(temp->hash != recalculate_hash)        
        {
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
    cout<<"chain valid after checking all possible arguments"<<endl;
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
// void save_time(string nickname,string ts,string secret_key)
// {
//     // string name_id=calculate_hash(0,nickname,"RECOVERY_ID", "", 0);
//     // string raw_data=nickname +"|"+ ts;
//     // vector<unsigned char> encrypted = aes_encrypt(raw_data, network_key);

//     ofstream file("recovery_map.txt", ios::app);
//     if(file.is_open()) 
//     {
//         // file<<name_id<<" ";
//         // for(unsigned char c : encrypted) {
//         //     file << hex << setfill('0') << setw(2) << (int)c << " ";
//         // }
//         file << dec << endl;
//         file.close();
//     }

// }

void save_to_file(string filename,string pub,string sec,string enc,string ts)
{
    string full_path=filename + ".txt";
    ofstream file(full_path);

    if(file.is_open())
    {
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

Wallet* create_acc(Blockchain &bc)
{
    string secret,enc_key,user_filename;
    cout << "\n========== ACCOUNT CREATION ==========" << endl;
    cout << "STep 1: Enter a Secret (Private Key): ";
    cin >> secret;
    cout << "Step 2: Set a Transaction Encryption Key: ";
    cin >> enc_key;
    cout << "Step 3: Filename for this Wallet backup:";
    cin >> user_filename;

    
    Wallet* new_wallet=new Wallet(secret,enc_key);
    save_to_file(user_filename, new_wallet->publickey, secret, enc_key, new_wallet->getCreationTime());
    string public_id_record = new_wallet->publickey;
    string wallet_record="WALLET|" + public_id_record + "|" + new_wallet->getCreationTime();    vector<unsigned char> dataVec(wallet_record.begin(),wallet_record.end());
    bc.insert_tail(dataVec);

    cout << "\n>>> [SUCCESS] Account Initialized!" << endl;
    
    cout << ">>> Your Public Address: " << public_id_record << endl;
    cout << ">>> Created At: " << new_wallet->getCreationTime() << endl;
    cout << "======================================\n" << endl;

    return new_wallet;
    
}


void transfer(Blockchain &bc, Wallet* wallet)
{
    string receiver,amount,input_secret;
    cout << "\n========== SEND TRANSACTION ==========" << endl;
    cout << "Enter Receiver's Public Address: ";
    cin >> receiver;
    cout << "Enter Amount: ";
    cin >> amount;
    //cout << "Enter Transaction Encryption Key: "; 
    // cin >> encryption_key;
    cout << "Enter your Secret Key to confirm transaction: ";
    cin >> input_secret;

    if(input_secret != wallet->getPrivateKey())
    {
        cout << ">>> [ERROR] Unauthorized! Private Key mismatch." << endl;
        return;
    }
    

    time_t now=time(0);
    string ts=ctime(&now);
    if(!ts.empty() && ts.back()=='\n')
       ts.pop_back();
    

       string rawdata = wallet->publickey + "|" + receiver + "|" + amount + "|" + ts;       string gentxn="TXN"+calculate_hash(0,rawdata,"TXN_SECURITY",ts,0);
       string signeture=calculate_hash(0,rawdata,"TXN_SECURE",ts,0);
       string full_payload = rawdata + "|" + gentxn;
       //string tnxid=wallet->publickey+"|"+receiver+"|"+amount+"|"+gentxn;
    //    vector<unsigned char> encryptedData = aes_encrypt(full_payload, encryption_key);
       //bc.insert_tail(encryptedData);

       string payload = rawdata + "SIG:" + signeture;

       vector<unsigned char>dataVec(payload.begin(),payload.end());


       bc.insert_tail(dataVec);

       cout << "\n>>> [NETWORK INFO] Verifying transaction integrity..." << endl;
       bc.isvalid();


       cout << "\n>>> [SUCCESS] Transaction Recorded!" << endl;
    cout << ">>> Txn ID: " << gentxn << endl;
    cout << "======================================\n" << endl;
}

void save_wallet(string pub,string sec,string ts,string filename)
{
    string full_path=filename + ".txt";
    ofstream file(full_path);

    if(file.is_open())
    {
        file << "================================" << endl;
    file<<"public key: "<<pub<<endl;
    file<<"private key(secret)"<<sec<<endl;
    file << "Created At  : " << ts << endl; 
    file << "================================" << endl << endl;
    file<<"Don't share this file";
    file.close();
    cout<<">>>wallet backed up to 'my_secret_vault.txt'. keep it safe!"<<endl;

    }
    else
    {
        cout << ">>> [ERROR] Could not create file!" << endl;
    }
    
    
}

Wallet* login_wallet(Blockchain &bc)
{
    
   int login_choice;
   cout<<"\n========== LOGIN MENU =========="<<endl;
   cout << "1. Login via Backup File (Drag & Drop)" << endl;
   cout << "2. Manual Login (Type Secret & Encryption Key)" << endl;
   cout << "Choice: "; 
   cin >> login_choice;

   string secret="",enc_key="",filepath="";

   if(login_choice == 1)
   {
    cout << "Drag and drop your wallet file here or enter full path: " << endl;
        cout << "Path: ";
        cin.ignore(); 
         
        getline(cin, filepath);

        if (!filepath.empty() && filepath.front() == '"') {
            filepath.erase(0, 1);
            filepath.erase(filepath.size() - 1);
        }
        ifstream file(filepath);
        if(file.is_open())
        {
            string line;
            while(getline(file,line))
            {
                if(line.find("Login Secret   :")!=string::npos)
                {
                    secret=line.substr(line.find(":")+2);
                }
                else if(line.find("Encryption Key :")!=string::npos)
                {
                    enc_key=line.substr(line.find(":")+2);
                }

            }   
               file.close(); 
                if(secret=="" || enc_key == "")
                {
                    cout<<">>> [ERROR] File format is wrong!" << endl;
                    return NULL;
                }
        }        
                else
                {
                    cout << ">>> [ERROR] Could not open file!" << endl;
                    return NULL;
                }
                
            
           
        
   }
    else if(login_choice==2)
        {
            cout << "Enter your Secret Key: "; 
            cin >> secret;
            cout << "Enter your encryption key: ";
            cin>>enc_key;   
        }
    
        string temp_pub=public_key(secret,enc_key);

        Node* temp=bc.getHead();
        bool found =false;
        cout<<"==================================="<<endl;


        while(temp!=NULL)
        {
            string dataStr(temp->data.begin(),temp->data.end());

            if(dataStr.find("WALLET|" + temp_pub)==0)
            {
                found=true;
                break;
            }
            temp=temp->next;
        }
            if(found)
            {
                cout<<"LOGIN SUCCESS"<<endl;
                Wallet* temp_wallet=new Wallet(secret,enc_key);
                return temp_wallet;
            }
            else
            {
                cout<<"LOGIN FAILED"<<endl;
                return NULL;
            }
            
        
        

        
}

void show_all_wallet(Blockchain &bc)
{
    Node* temp=bc.getHead();
    cout<<endl<<"__All Accouns__"<<endl;
    if(temp==NULL)
    {
        cout<<endl<<"No Wallet Found"<<endl;
        return;
    }
    while(temp!=NULL)
    {
        string dataStr(temp->data.begin(),temp->data.end());

        if(dataStr.find("WALLET|")==0)
        {
            int first = dataStr.find("|");
            int second = dataStr.find("|",first+1);
            string pub=dataStr.substr(first+1,second - first - 1);
            cout<<"Wallet Address: "<<pub<<endl;
        }
        temp=temp->next;
    }
    cout<<"====================="<<endl;
}

void save_blockchain(vector<unsigned char>data)
{
    string plain(data.begin(),data.end());
    vector<unsigned char>encrypted=aes_encrypt(plain,INTERNAL_KEY);
    ofstream file("blockchain.dat",ios::app | ios::binary);
    int size = encrypted.size();

    file.write((char*)&size,sizeof(size));
    file.write((char*)encrypted.data(),size);
    file.close();
}


void load_blockchain(Blockchain &bc)
{
    
    ifstream file("blockchain.dat",ios::binary);
    
        if(!file.is_open())
        {
            cout<<"no file found"<<endl;
            return;
        }
        LOADING_MODE = true;
        while(true)
        {
            int size;
           

            if( !file.read((char*)&size,sizeof(size)))
            {
                break;
            }

            vector<unsigned char>encrypted(size);
            file.read((char*)encrypted.data(), size);
            //part of decryption
            string decrypted = aes_decrypt(encrypted,INTERNAL_KEY);
            vector<unsigned char>data(decrypted.begin(),decrypted.end());
            bc.insert_tail(data);
            
        }
         LOADING_MODE = false;
        file.close();

       
    
}
int main() 
{


    
    cout << "--- Network Initialization ---" << endl;
    
    // cout<<"set your secretkey - ; ";
    // cin>>key;
    // key.resize(16, '0');
    Blockchain bc;
    load_blockchain(bc);

   Wallet* wallet=NULL;
    int choice,choice2;
   while(true)
   {
       cout << "\n===== WELCOME TO BLOCKCHAIN NETWORK =====" << endl;
      
        cout << "1. Create New Wallet (Register)" << endl;
        cout << "2. Login to Existing Wallet" << endl;

        cout << "3. Exit" << endl;

        cout<<"4. all account" << endl;

        cout << "Choice: "; 
        cin >> choice;
        
        if(choice==1)
        {
            create_acc(bc);
            wallet=NULL;
        }
        else if(choice==2)
        {
            wallet=login_wallet(bc);
        }
        else if(choice==3)
        {
            break;
        }
        else if(choice==4)
        {
            show_all_wallet(bc);
            continue;
        }

        if(wallet!=NULL)
        {
           cout << "logged in as: " << wallet->publickey << endl;
           bool loggedin=true;

           while(loggedin)
           {
             cout << "\n---------- USER DASHBOARD ----------" << endl;
                cout << "1. Send Transaction (Money Transfer)" << endl;
                cout << "2. View Transaction History (Ledger)" << endl;
                cout << "3. Check Balance" << endl;
                cout << "4. Logout" << endl;
                cout << "Choice: "; 
                cin >> choice2;


                if(choice2==1)
                {
                    transfer(bc,wallet);
                }
                else if(choice2==2)
                {
                    bc.display(wallet->getencyptionkey());
                }
                else if(choice2==3)
                {
                    cout << "Current Balance functionality coming soon!" << endl;
                }
                else if(choice2==4)
                {
                    loggedin=false;
                    wallet=NULL;
                    cout << "Logged out successfully!" << endl;
                }
           }
        }
      


   }

}


/**if(wallet==NULL)
            {
                cout<<"Error: Create a wallet first!" << endl;
                create_acc();
                //continue;
            }
            else
            {
                cout << "1. send amount" << endl;


                
                cout << "2. View Blockchain Ledger" << endl;

                cout << "Enter your choice: ";
        
            }
            

            // transfer(bc,wallet);**/

/**g++ blockchain_recov.cpp -o blockchain_final -I C:/msys64/mingw64/include -L C:/msys64/mingw64/lib -lssl -lcrypto
.\blockchain_final**/

// g++ blocckhain_save.cpp -o blockchain_save -I C:/msys64/mingw64/include -L C:/msys64/mingw64/lib -lssl -lcrypto
// .\blockchain_save.exe

// .\blockchain_login.exe
