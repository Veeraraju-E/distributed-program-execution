#include <fstream>
#include <vector>
#include <algorithm>
#include <iostream>

using namespace std;

Define_Module(Client);

void Client::initialize() {
    // get topology from external .txt file and create servers and clients
    std::ifstream file(par("topology.txt").stringValue());
    std::vector<int> servers, clients;
    std::string line;

    while(std::getline(file, line)) {
        if (line.find('server:') == 0) {
            int serverid = std::stoi(line.substr(7));
            servers.push_back(serverid);
        }
        else if (line.find('client:') == 0) {
            int clientid = std::stoi(line.substr(7));
            clients.push_back(clientid);
        }
    }

    // Set up connections
    for (int server: servers) {
        cModuleType *server_type = cModuleType::get("project.simulations.Server");
        cModule *server = server_type -> create("server", getParentModule(), server);
        server->par("serverID") = server;
        server->finalizeParameters();
        server->buildInside();
        // 2-way connection via gates, after build
        cGate *client_gate = gate("serverGate$o", server);  // $output
        cGate *server_gate = server->gate("gate$i");
        cpnnect(client_gate, server_gate);
    }

    // Connect to ALL different clients
    int curr_client = par("clientId");
    for (int client: clients) {
        if (client != curr_client) {
            cModule *diff_client = getParentModule()->getSubModule("client", client);
            if (diff_client) {
                cGate *transmit_gate = gate("clientGate$o", client);
                cGate *receive_gate = diff_client->gate("clientGate$i", curr_client);
                connect(transmit_gate, receive_gate);
            }
        }
    }
    scheduleAt(simTime() + 1.0, new cMessage("Processing Started...\n"))
}

void Client::distribute_msg(cMessage *msg) {
    // splitting msg, selecting n/2 + 1 servers, sending sub tasks
}

void Client::handle_message(cMessage *msg) {

}











