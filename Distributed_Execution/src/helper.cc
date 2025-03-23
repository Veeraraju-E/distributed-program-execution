#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <chrono>
#include <ctime>

#ifndef HELPER_H
#define HELPER_H

#define HASH_LENGTH 16

using namespace std;

string generate_hash(const string &input) {
    hash<string> hash_fn;
    size_t hash_value = hash_fn(input);

    stringstream ss;
    ss << hex << setw(HASH_LENGTH) << setfill('0') << hash_value;

    string hex_string = ss.str();
    if (hex_string.length() > HASH_LENGTH) {
        hex_string = hex_string.substr(0, HASH_LENGTH);
    }

    return hex_string;
}

string create_server(int server_number) {

    /*
     * An auto generated server_list.txt will be formed, which will be helpful to connect and identify a particular server
     * The format of the server_list.txt would be:
     * -------------------------------------
     * Server_List generated at: <timestamp>
     * -------------------------------------
     * Then each line would represent a <server id> server_<server_number>
     */

    auto now = chrono::system_clock::now();
    time_t now_ = chrono::system_clock::to_time_t(now);

    stringstream ss;
    ss << put_time(localtime(&now_), "%Y-%m-%d %H:%M:%S");
    string timestamp = ss.str();

    ofstream file("./src/server_list.txt", ios::app);
    if (!file) {
        cerr << "Error opening server_list.txt!" << endl;
        return "None";
    }

    string server_id = to_string(now_) + "server" + to_string(server_number);
    string s_id = generate_hash(server_id);
    file << s_id << " server_" << server_number << "\n";


    file.close();
    return s_id;
}

string create_client(int client_number) {

    /*
     * An auto generated client_list.txt will be formed, which will be helpful to connect and identify a particular server
     * The format of the client_list.txt would be:
     * Then each line would represent a <client id> client_<client_number>
     */

    auto now = chrono::system_clock::now();
    time_t now_ = chrono::system_clock::to_time_t(now);

    stringstream ss;
    ss << put_time(localtime(&now_), "%Y-%m-%d %H:%M:%S");
    string timestamp = ss.str();

    ofstream file("./src/client_list.txt", ios::app);
    if (!file) {
        cerr << "Error opening client_list.txt!" << endl;
        return "None";
    }

    string client_id = to_string(now_) + "client_" + to_string(client_number);
    string c_id = generate_hash(client_id);
    file << c_id << " client_" << client_number <<  "\n";

    file.close();
    return c_id;
}

vector<vector<string>> fetch_servers() {
    ifstream file("./src/server_list.txt");
    vector<vector<string>> servers;
    string line;

    if (!file.is_open()) {
        cout << "Error opening server_list.txt!" << endl;
        return servers;
    }


    while (getline(file, line)) {

            if(line.find("-") != string::npos) continue;
            stringstream ss(line);
            vector<string> result;
            string word;
            while(ss >> word){
                 if(word.find("_") != string::npos){
                    word = word.substr(word.size()-1,word.size());
                }
                result.push_back(word);
            }
            servers.push_back(result);

    }

    file.close();
    return servers;
}

vector<vector<string>> fetch_clients() {
    ifstream file("./src/client_list.txt");
    vector<vector<string>> clients;
    string line;

    if (!file.is_open()) {
        cout << "Error opening server_list.txt!" << endl;
        return clients;
    }

    bool is_reading = false;
    while (getline(file, line)) {

            stringstream ss(line);
            vector<string> result;
            string word;
            while(ss >> word){
                 if(word.find("_") != string::npos){
                    word = word.substr(word.size()-1,word.size());
                }
                result.push_back(word);
            }
            clients.push_back(result);

    }

    file.close();
    return clients;
}

#endif

// int main(){
//     vector<vector<string>> servers = fetch_servers();
//     for(auto server: servers){
//         cout << server[0] << " " << server[1] << endl;
//     }

//     vector<vector<string>> clients = fetch_clients();
//     for(auto server: clients){
//         cout << server[0] << " " << server[1] << endl;
//     }
// }

