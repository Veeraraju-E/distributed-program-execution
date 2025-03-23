#include <string>
#include <omnetpp.h>
#include <vector>
#include <iostream>
#include "helper.h"

using namespace omnetpp;

// Client Tasks

/* TODO: Task 01
 * Select a random subset of clients_available and send a connect request (Format to be decided)
 * Wait for each connection request's ack and then append that client to my clients_connected list and append the connection in topo.txt (hopefully set up a thread for it)
 * Upon receiving a connection request, we will add the client to our client_connected list and append the new client in topo.txt (hopefully set up a thread for it)
 */

/* TODO: Task 02
 * Once, all my connection requests have acknowledged (keep a counter):
 * Create an array of integers (random integers) and split it into n parts.
 * Then, each part will be sent to randomly selected randint(n//2 + 1,n) servers (for the first time) else the smartly chosen servers (Task 04)
 * Wait till you receive response from all servers (again another counter)
 * Once all responses for a subtask are obtained ,Verify the response from each server and score (need to think what does verification mean)
 * Continue for next subtask
 */

/*
 * TODO: Task 03
 * Once, all subtasks for a task are over, the scoring algorithm must run to score a server
 * Then, broadcast to all clients my score (Format must be gossip format) <self.timestamp>:<self.ID>:<self.Score#>
 * Ensure that we proceed for the second task only after broadcasting my score
 */

/*
 * TODO: Task 04
 * Selection of servers based on scores (Need to decide the algorithm)
 */

/* TODO: Task 05
 * Verification Algorithm (of the response)
 *
 */



class Client : public cSimpleModule
{
  protected:
    // The following redefined virtual function holds the algorithm.
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;

  private:
    vector<vector<string>> servers_connected = {};
    vector<vector<string>> clients_connected = {};
    string my_id = "";
    int my_client_number = 0;

    void save_my_clients(vector<vector<string>> clients_available){
        if(clients_available.empty()){
            return;
        }

        for(int i = 0; i < clients_available.size();i++){
            int client_number = std::stoi(clients_available[i][1].c_str());
            if(client_number == (my_client_number+1)%4 || client_number == (my_client_number+3)%4){
                clients_connected.push_back(clients_available[i]);
            }
        }

    }

    void start_task(){
        string msg = my_id + " " + to_string(my_client_number);
        cMessage* msg1 = new cMessage(msg.c_str());
        send(msg1, "outClientGates", 0);

        cMessage* msg2 = new cMessage(msg.c_str());
        send(msg2,"outClientGates",1);
    }
};


Define_Module(Client);

void Client::initialize() {

    servers_connected = fetch_servers();
    vector<vector<string>> clients_available = fetch_clients();
    string temp = create_client(clients_available.size());
    if(temp != "None"){
        my_id = temp;
    }
    my_client_number = clients_available.size();
    EV << my_id << " " << my_client_number << endl;
    save_my_clients(clients_available);
    start_task();

}


void Client::handleMessage(cMessage *msg)
{
    EV << "Got Message" << msg <<  " " << my_client_number << endl;
}
