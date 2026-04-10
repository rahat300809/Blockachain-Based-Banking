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
            return;
        }
        if(temp->prev!=NULL && temp->previoushash!=temp->prev->hash)
        {
            cout<<"tampered";
            return;
        }
        temp=temp->next;
    }
    cout<<"chain valid";
   }
   
};




int main() 
{
    Blockchain bc;
    bc.insert_tail("Rahat|karim|TXN001");
    bc.insert_tail("Karim|Sajid|200|TXN002");
    bc.insert_tail("Sajid|Rahat|100|TXN003");
    bc.display();
    bc.isvalid();
    return 0;
}