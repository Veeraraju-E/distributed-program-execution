#include <string>
#include <omnetpp.h>
#include <vector>
#include <iostream>
#include "helper.h"

using namespace omnetpp;

class Server : public cSimpleModule
{
  protected:
    // The following redefined virtual function holds the algorithm.
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;

  private:
    string my_id = "";
    int my_server_number = 0;

};


Define_Module(Server);

void Server::initialize() {

    vector<vector<string>> servers = fetch_servers();
    my_server_number = servers.size();
    string temp = create_server(servers.size());
    if(temp!= "None"){
        my_id = temp;
    }

}


void Server::handleMessage(cMessage *msg)
{

    send(msg, "out");
}
