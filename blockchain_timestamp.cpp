#include<bits/stdc++.h>
using namespace std;

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
     string data;
     string previoushash;
     string nexthash;
     string hash;
     Node* next;
     Node* prev;
     string timestamp;
     

    Node(int index,string data,string previoushash)
    {
        time_t now = time(0);
        this->timestamp = ctime(&now);
        this->index=index;
        this->data=data;
        this->previoushash=previoushash;
        this->nexthash="";
        this->next=NULL;
        this->prev=NULL;
        this->hash=claculate_hash(index,data,previoushash,timestamp);
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
    string previoshash="";
    if(tail!=NULL)
    {
        previoshash=tail->hash;
    }
    Node* newnode=new Node(size,data,previoshash);

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
       
        cout<<temp->data<<" "<<temp->hash<<" "<<temp->index<<" "<<temp->nexthash<<" "<<temp->timestamp;
        temp=temp->next;
        cout<<endl;
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
        if(temp->hash!=claculate_hash(temp->index, temp->data, temp->previoushash,temp->timestamp))
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
    Blockchain bc;
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