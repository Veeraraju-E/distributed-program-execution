#ifndef HELPER_H
#define HELPER_H

#include <vector>
#include <string>

using namespace std;

string generate_hash(const string &input);
string create_server(int server_number);
string create_client(int client_number);
vector<vector<string>> fetch_servers();
vector<vector<string>> fetch_clients();

#endif
