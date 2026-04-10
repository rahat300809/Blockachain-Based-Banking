#include<bits/stdc++.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
using namespace std;

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



string claculate_hash(int index,string data,string previoushash,string timestamp)
{
   string input=to_string(index)+data+previoushash+timestamp;
   long long hash=0;
   for(char c: input)
   {
    hash=(hash*31+c)%1000000007;
   }
   return(to_string(hash));
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
        this->hash = claculate_hash(index, dataStr, previoushash, timestamp);
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
                    newlevel.push_back(claculate_hash(0,hashes[i]+hashes[i+1],"",""));
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
        
        if(temp->hash != claculate_hash(temp->index, dataStr, temp->previoushash, temp->timestamp))        {
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




int main() 
{
    string key;
    cout<<"set your secretkey - ; ";
    cin>>key;
    key.resize(16, '0');
    Blockchain bc(key);
    bc.insert_tail("Rahat|karim|500|TXN001");
    bc.insert_tail("Karim|Sajid|200|TXN002");
    bc.insert_tail("Sajid|Rahat|100|TXN003");
    bc.display();
    bc.isvalid();
    
    cout << "Merkle Root: " << bc.merkleroot() << endl;
    bc.insert_tail("Karim|Sajid|9999|TXN002");  // 200 → 9999
    cout << "Merkle Root: " << bc.merkleroot() << endl;
    return 0;
}


// g++ blockchain_encrypted.cpp -o blockchain_encrypted -IC:/msys64/mingw64/include -LC:/msys64/mingw64/lib -lssl -lcrypto
// .\blockchain_encrypted