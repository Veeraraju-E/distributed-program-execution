#include <string>
#include <omnetpp.h>
#include <vector>
#include <iostream>
#include <algorithm>
#include <random>
#include <map>
#include <fstream>
#include "helper.h"

using namespace omnetpp;

class Server : public cSimpleModule {
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;

private:
    int my_server_number = 0;
    bool is_malicious = false;
    string my_id = "";
    vector<vector<string>> clients_available = {};


    vector<SubtaskResult> subtask_history;

    // get info from message; GPT-genereated
    Message parse_message(const string& msg_content) {
        // Message Format needed: <Message Type>:<client_id>:<sub_task_id>:<elements of array>(csv)
        Message msg;
        stringstream ss(msg_content);
        string token;
        getline(ss,token,':');
        getline(ss, token, ':');    // at first ':', we stop and retrieve client_id
        msg.client_id = token;

        getline(ss, token, ':');    // from first to second ':', we retrieve task_id
        msg.subtask_id = token;

        // fetch the vector data, starts after third ':'
        if(getline(ss, token, ':')) {
            stringstream arr_ss(token);
            string num;
            while(getline(arr_ss, num, ',')) {
                if(!num.empty()) {
                    msg.data.push_back(stoi(num));
                }
            }
        }
        return msg;
    }

    int find_max(const vector<int>& arr) {
        if (arr.empty()) return -1;
        return *max_element(arr.begin(), arr.end());    // value
    }

    // Return wrong results: TODO: How? Random/Explicitly wrong
    int get_malicious_result(const vector<int>& arr) {
        if (arr.empty()) return -1;
        return arr[0]; // rn, simply return the first element
    }

    pair<string,int> create_response(const Message& original_msg, int result) {
        // Response Format: <MessageType>:<server_id>:<client_id>:<sub_task_id>:result
        stringstream ss;
        ss << "0:"<< my_id << ":" << original_msg.client_id << ":" << original_msg.subtask_id << ":" << result;
        if(clients_available.empty()){
            clients_available = fetch_clients();
        }
        for(auto client: clients_available){
            if(client[0] == original_msg.client_id){
                return {ss.str(),stoi(client[1])};
            }
        }
    }

    // GPT-genereated
    void log_result(const SubtaskResult& result) {
        // To console
        EV << "Server " << my_server_number << " computed max: " << result.result << " for task: " << result.subtask_id << endl;

        // To file
        string filename = "output.txt";
        ofstream outfile(filename);
        if (outfile.is_open()) {
            outfile << "Task: " << result.subtask_id << ", Max: " << result.result << ", Time: " << result.timestamp.str() << endl;
            outfile.close();
        }
    }

    // Doubtful of this code!!
    void decide_malicious() {
        int total_servers = gateSize("outClientGates");
        // Ensure that max of n/4 are malicious
        if (dblrand() < (0.25 * total_servers)) {
            is_malicious = true;
        }
    }
};

Define_Module(Server);

void Server::initialize() {
    vector<vector<string>> servers = fetch_servers();
    string temp = create_server(servers.size());
    if(temp != "None") {
        my_id = temp;
    }
    my_server_number = servers.size();
    decide_malicious();

    EV << "Server " << my_server_number << " with ID: " << my_id << " has malicious behavior: " << (is_malicious ? "Yes" : "No") << endl;
}

void Server::handleMessage(cMessage *msg) {
    string msg_content = msg->getName();
    EV << "Server " << my_server_number << " received message: " << msg_content << endl;

    Message parsed_msg = parse_message(msg_content);

    // Process subtask
    int result;
    result = is_malicious ? get_malicious_result(parsed_msg.data) : find_max(parsed_msg.data);

    SubtaskResult subtask_result;

    subtask_result.subtask_id = parsed_msg.subtask_id;
    subtask_result.result = result;
    subtask_result.timestamp = simTime();

    subtask_history.push_back(subtask_result);
    log_result(subtask_result);  // log

    // response to client
    pair<string,int> response = create_response(parsed_msg, result);
    cMessage *response_msg = new cMessage(response.first.c_str());


    // GPT recommended, but can remove this random processing delay between 0.1 and 0.5 seconds
    // issue, where should the message go?? I mean which gate?
    simtime_t processing_delay = uniform(0.1, 0.5);
    send(response_msg,"outClientGates",response.second);

    delete msg;
}
