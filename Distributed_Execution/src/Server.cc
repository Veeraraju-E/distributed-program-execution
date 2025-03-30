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
    string my_id = "";
    vector<vector<string>> clients_available = {};
    
    map<string, bool> is_malicious_for_task;
    
    int total_servers = 0;
    const double probability = 0.25; 

    vector<SubtaskResult> subtask_history;

    // get info from message
    Message parse_message(const string& msg_content) {
        // Message Format needed: <Message Type>:<client_id>:<sub_task_id>:<elements of array>(csv)
        Message msg;
        stringstream ss(msg_content);
        string token;
        getline(ss,token,':');
        getline(ss, token, ':');    // at first ':', we stop and retrieve client_id
        msg.client_id = token;

        getline(ss, token, ':');    // from first to second ':', we retrieve subtask_id
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

    // process the task given to me
    int process_task(const vector<int>& arr, const string& subtask_id) {
        // Extract task_id from subtask_id (first part before last underscore)
        string task_id = subtask_id.substr(0, subtask_id.find_last_of('_'));
        
        // if we've already decided for this task, maintain consistency
        if (is_malicious_for_task.find(task_id) != is_malicious_for_task.end()) {
            if (is_malicious_for_task[task_id]) {
                EV << "Server " << my_server_number << " remaining malicious for task " << task_id << endl;
                return find_malicious_result(arr);
            }
            return find_max(arr);
        }

        // else, make new decision for this task
        random_device rd;
        mt19937 gen(rd());
        uniform_real_distribution<> dis(0.0, 1.0);
        
        bool will_be_malicious = (dis(gen) < probability);
        is_malicious_for_task[task_id] = will_be_malicious;
        
        if (will_be_malicious) {
            EV << "Server " << my_server_number << " decided to be malicious for task " << task_id << endl;
            return find_malicious_result(arr);
        }

        return find_max(arr);
    }

    // malicious result generation
    int find_malicious_result(const vector<int>& arr) {
        if (arr.empty()) return 0;
        
        int actual_max = find_max(arr);
        
        // Always return  wrong (minimum, it'll be wrong since each server gets subtask with subarray size of >=2)
        if (arr.size() > 1) {

            int min_val = *min_element(arr.begin(), arr.end());
            return min_val;
        }
        
        // if by chance, server gets only one element and still it has to act malicious, it'll return half
        return actual_max / 2;
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
};

Define_Module(Server);

void Server::initialize() {
    vector<vector<string>> servers = fetch_servers();
    string temp = create_server(servers.size());
    if(temp != "None") {
        my_id = temp;
    }
    my_server_number = servers.size();
    total_servers = getParentModule()->getSubmoduleVectorSize("server");
    
    EV << "Server " << my_server_number << " with ID: " << my_id << " initialized" << endl;
}

void Server::handleMessage(cMessage *msg) {
    string msg_content = msg->getName();
    EV << "Server " << my_server_number << " received message: " << msg_content << endl;

    Message parsed_msg = parse_message(msg_content);
    int result = process_task(parsed_msg.data, parsed_msg.subtask_id);

    SubtaskResult subtask_result;
    subtask_result.subtask_id = parsed_msg.subtask_id;
    subtask_result.result = result;
    subtask_result.timestamp = simTime();

    subtask_history.push_back(subtask_result);
    log_result(subtask_result);

    pair<string,int> response = create_response(parsed_msg, result);
    cMessage *response_msg = new cMessage(response.first.c_str());
    send(response_msg, "outClientGates", response.second);

    delete msg;
