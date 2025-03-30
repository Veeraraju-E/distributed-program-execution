/*
 * © B22CS080 - Veeraraju Elluru, B22CS052 - Sumeet S Patil
 * This file is has original work. All rights are reserved under the jurisdiction of IIT Jodhpur
 */

#ifndef HELPER_H
#define HELPER_H

#include <vector>
#include <string>
#include <omnetpp.h>

using namespace std;

// the same global malicious_count from helper.cc
extern int global_malicious_count;

string generate_hash(const string &input);
string create_server(int server_number);
string create_client(int client_number);
vector<vector<string>> fetch_servers();
vector<vector<string>> fetch_clients();

enum MessageType {
    SUBTASK_REQUEST,
    SUBTASK_RESPONSE,
    SERVER_SCORES_UPDATE
};

struct SubtaskResult {
       string subtask_id;
       string server_id;
       int result;
       bool isWrongResult = false;
       omnetpp::simtime_t timestamp;
   };


// Overall message
struct Message {
    MessageType type;
    string client_id;
    string subtask_id;
    vector<int> data;
    string msg_hash;
    SubtaskResult subtaskresult;
    bool isError = false;
    map<string,float> sever_scores;
};

#endif
