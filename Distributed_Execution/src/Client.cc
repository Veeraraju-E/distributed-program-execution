#include <string>
#include <omnetpp.h>
#include <vector>
#include <iostream>
#include <algorithm>
#include "helper.h"
#include <unordered_set>
#include <map>
#include <chrono>
#include <fstream>

#define MAX_ELEMENT_OF_ARRAY 10000000007

using namespace omnetpp;



// Client Tasks
// TODO: handle logging more elegantly
// TODO: Optimize the code.
// TODO: Dynamic network setup

/* things to be logged:
 * 1. Client Creation
 * 2. The first task array
 * 3. The first task servers chosen
 * 4. The first task subtasks distributed, one log per subtask
 * 5. The subtask completion, log the majority result
 * 6. The task completion, its final answer and scores of servers
 * 7. Broadcast log, one final log after all broadcast sent
 * 8. If broadcast received, then log the message
 * 9. Repeat 2-8 for task 2
*/

class Client : public cSimpleModule
{
  protected:
    // The following redefined virtual function holds the algorithm.
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;

  private:

    // Needed to efficiently handle subtasks
    struct Subtask {
        string subtask_id;
        vector<int> input;
    };

    // needed to efficiently handle task
    struct Task{
        string task_id;
        map<string,Subtask> subtasks;
        unordered_set<int> servers_targeted;               // -> the servers to whom we'll send the subtasks
        map<string,vector<SubtaskResult>> subtask_results; // -> results of subtasks are a dictionary
        int final_result = 0;
        int subtasks_done = 0;
    };

    vector<vector<string>> servers_connected = {};         // list of servers
    map<string,float> server_scores;                       // server scores
    vector<vector<string>> clients_connected = {};         // list of connected clients
    string my_id = "";
    int tasks_done = 0;
    int my_client_number = 0;
    Task current_task;
    vector<Task> task_history = {};
    vector<Message> message_history = {};

    // method to save clients that the current client is connected to
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

    // a typecast method to type cast subtask to a message that can be transferred
    string to_message(Subtask subtask){
        // Message Format needed: <Message_Type>:<client_id>:<sub_task_id>:<elements of array>(csv)
        string msg = "1:"+my_id+":"+subtask.subtask_id+":"+to_string(subtask.input[0])+","+to_string(subtask.input[1]);
        return msg;
    }

    // used to parse a message received by client
    Message parse_message(const string& msg){

        Message received_message;
        stringstream ss(msg);
        string token;
        // token = Message_Type : 0 -> Subtask response, 1-> subtask request, 2-> server score broadcast
        getline(ss,token,':');
        if(token == "0"){

            // Response Format: <Message_Type>:<server_id>:<client_id>:<sub_task_id>:<result> for subtask result

            received_message.type = SUBTASK_RESPONSE;
            SubtaskResult result;
            // token = server id
            getline(ss,token,':');
            result.server_id = token;

            // token = client id now.
            getline(ss,token,':');
            if(token != my_id){
                // if client id doesn't match => this isn't my message, so we need to discard this message
                received_message.isError = true;
                received_message.subtaskresult = result;
                return received_message;
            }
            // token = subtask id
            getline(ss,token,':');
            result.subtask_id = token;

            // token = subtask result by the current server
            getline(ss,token,':');
            result.result = stoi(token);
            received_message.subtaskresult = result;

            // for logging purpose
            cout << received_message.subtaskresult.result << " " << received_message.subtask_id << " " <<  received_message.subtaskresult.server_id << endl;
        }
        else if(token == "2"){
            // Response format: <Message_Type>:<timestamp>:<client_id>:<server_scores>:<msg_hash>
            received_message.type = SERVER_SCORES_UPDATE;

            // token = timestamp -> need to handle its usage
            getline(ss,token,':');

            // token = client id of the sender client
            getline(ss,token,':');
            received_message.client_id = token;

            // token = server scores which are of the form <serverid>_<serverscore>
            getline(ss,token,':');

            string server_score_pair; // -> to store the server_score_pair

            stringstream server_scores_received(token);

            while(getline(server_scores_received,server_score_pair,',')){

                // updating server scores in the message struct as of now!!
                string server_id = server_score_pair.substr(0,server_score_pair.find('_'));
                int server_score = stoi(server_score_pair.substr(server_score_pair.find('_')+1,server_score_pair.size()));
                received_message.sever_scores[server_id] = server_score;
            }
        }
        return received_message;
    }

   Task create_task(bool is_first_task){
        // initialize an array first.
        int n = servers_connected.size();
        vector<int> task_array;

        string task_digest = "";

        for(int i = 0; i < 2*n;i++){
            int num = rand() % MAX_ELEMENT_OF_ARRAY;
            task_digest+=to_string(num)+",";
            task_array.push_back(num);
        }

        log_info("Array generated for task "+to_string(tasks_done+1)+" "+task_digest);
        EV << "Client " << my_client_number << " Array generated for task "+to_string(tasks_done+1)+" "+task_digest;

        // creating the task
        string task_id = generate_hash(task_digest);
        Task task;
        task.task_id = task_id;

        // creating the subtasks
        for(int i = 0; i < n;i++){

            int num1 = task_array[i],num2 = task_array[n+i];

            string subtask_digest = task_id+to_string(num1)+to_string(num2);
            string subtask_id = generate_hash(subtask_digest);
            Subtask subtask;

            subtask.input = {num1,num2};
            subtask.subtask_id = subtask_id;
            task.subtasks[subtask_id] = subtask;

        }

        if(is_first_task){
        // choosing the target servers randomly
            unordered_set<int> chosen_servers;
            while(chosen_servers.size() != n/2 + 1){
                int server_index = rand() % n;
                chosen_servers.insert(server_index);
            }
            string servers_chosen = "";
            for(auto server: chosen_servers){
                servers_chosen+="Server "+to_string(server)+",";
            }
            log_info("Servers chosen for task "+to_string(tasks_done+1)+" " + servers_chosen);

            task.servers_targeted = chosen_servers;

      } else {

          unordered_set<int> chosen_servers;
          // converting to vector to sort
          vector<pair<string,float>> current_server_scores(server_scores.begin(),server_scores.end());
          sort(current_server_scores.begin(),current_server_scores.end(),[](const auto& a, const auto& b) {
              return a.second < b.second;  // Sort by value (lexicographically)
          });

          // choosing top n//2 + 1 servers for this subtask
          for(int i = 0; i < n/2 + 1;i++){
              // fetching server gate to send the message
              int gate = get_server_gate(current_server_scores[i].first);
              chosen_servers.insert(gate);
          }
          string servers_chosen = "";
          for(auto server: chosen_servers){
              servers_chosen+="Server "+to_string(server)+",";
          }
          log_info("Servers chosen for task "+to_string(tasks_done+1)+" " + servers_chosen);
          // target servers attached to the task
          task.servers_targeted = chosen_servers;

      }

     return task;
    }

   int get_server_gate(const string& server_id){
       // overhead function to fetch server gate based on id
       for(auto server: servers_connected){
           if(server[0] == server_id){
               return stoi(server[1]);
           }
       }
    }

   void distribute_task_to_servers(){
       // important function to distribute tasks among chosen servers
       for(auto subtask: current_task.subtasks){
           for(auto server: current_task.servers_targeted){
               // send the message
               string msg = to_message(subtask.second);
               cMessage* msg_to_send = new cMessage(msg.c_str());
               send(msg_to_send,"outServerGates",server);
//               delete msg_to_send;
           }
           log_info("Subtask "+subtask.second.subtask_id+" of "+to_string(tasks_done+1)+" distributed to servers");
       }
   }

   void update_task_result(const string& subtask_id){
       // to update server task result
       if(current_task.subtasks.find(subtask_id) == current_task.subtasks.end()){
           EV << "Received wrong subtask result " << subtask_id <<  " for the current task" << endl;
           return;
       }

       // subtask fetched
       Subtask subtask = current_task.subtasks[subtask_id];
       // subtask results from all server fetched
       vector<SubtaskResult> results = current_task.subtask_results[subtask_id];

       // computing the frequencies of each result
       map<int,int> freq_results;
       for(auto result: results){
           if(freq_results.find(result.result) == freq_results.end()){
               freq_results[result.result] = 1;
           } else {
               freq_results[result.result]+=1;
           }
       }

       // storing the correct result based on majority
       pair<int,int> correct_result = {0,-1};
       for(auto result: freq_results){
           if(result.second > correct_result.second){
               correct_result = result;
           }
       }

       log_info("Subtask "+subtask.subtask_id+" result is: "+to_string(correct_result.first)+" with vote of "+to_string(correct_result.second));

       // updating the task's final result also based on this
       if(current_task.final_result < correct_result.first){
           current_task.final_result = correct_result.first;
       }

       log_info("Task" + to_string(tasks_done+1) + " result updated to "+to_string(current_task.final_result));

       // updating whether the server's response was wrong (malicious) or not
       for(auto subtaskresult: results){
           if(subtaskresult.result != correct_result.first){
               subtaskresult.isWrongResult = true;
           }
       }
   }

   void consolidate_server_scores(){
       // a method that calculates server scores and updates them
       for(auto subtask: current_task.subtask_results){
           for(auto subtaskresult: subtask.second){
               string server_id = subtaskresult.server_id;
               if(server_scores.find(server_id) == server_scores.end()){
                   server_scores[server_id] = (subtaskresult.isWrongResult == false);
               } else {
                   server_scores[server_id]+= (subtaskresult.isWrongResult == false);
               }
           }
          string str_server_scores = "";
          for(auto server: server_scores){
              str_server_scores+=server.first+" "+to_string(server.second)+"\n";
          }
          EV << "Server Scores after task " << to_string(tasks_done+1) << " " << str_server_scores << endl;
          log_info("Server Scores after task " + to_string(tasks_done+1) + " " + str_server_scores);
       }
   }

   pair<Message,string> get_server_scores(){
       // Broadcast format: <Message_Type>:<timestamp>:<client_id>:<server_scores>:<msg_hash>
       // server_scores -> <server_id>_<server_score>[,<server_id>_<server_score>]

       Message message;
       message.type = SERVER_SCORES_UPDATE;
       message.client_id = my_id;
       message.sever_scores = server_scores;

       string msg = "";
       msg+="2:";

       auto now = chrono::system_clock::now();
       time_t now_ = chrono::system_clock::to_time_t(now);
       msg+=to_string(now_)+":";
       msg+=my_id+":";


       for(auto server: server_scores){
           msg+=server.first+"_"+to_string(server.second);
           msg+=",";
       }
       msg.pop_back();

       string msg_hash = generate_hash(msg);
       message.msg_hash = msg_hash;
       msg+=":"+msg_hash;
       return {message,msg};
   }


   void broadcast_scores(){

       // need to have log here
       pair<Message,string> msg_to_broadcast = get_server_scores();
       send(new cMessage(msg_to_broadcast.second.c_str()),"outClientGates",0);
       send(new cMessage(msg_to_broadcast.second.c_str()),"outClientGates",1);
       log_info("Scores broadcasted for task "+to_string(tasks_done+1));
       message_history.push_back(msg_to_broadcast.first);
   }

   bool is_message_in_history(Message msg){
       // again an overhead function to check if the message received was in history or not
       for(auto msg_in_history: message_history){
           if(msg.msg_hash == msg_in_history.msg_hash){
               return true;
           }
       }
       return false;
   }

   void forward_server_scores(Message msg,string msg_to_forward){
       // currently a separate function, will be merged soon with broadcast_scores
       if(!is_message_in_history(msg)){
          send(new cMessage(msg_to_forward.c_str()),"outClientGates",0);
          send(new cMessage(msg_to_forward.c_str()),"outClientGates",1);
      } else {
          return;
      }
       message_history.push_back(msg);
   }


   void update_server_scores(Message msg){
       // helper function to update the received server scores from other clients
       for(auto server: msg.sever_scores){
           if(server_scores.find(server.first) == server_scores.end()){
               server_scores[server.first] = server.second;
           } else {
               server_scores[server.first]+=server.second;
           }
       }
   }

   void log_info(string to_log){
       std::ofstream log_file("./src/client_log.txt",ios::app);
       if(!log_file){
           EV << "Log file can't be opened";
           return;
       }
       stringstream ss;
       log_file << "Client " << my_client_number << " " << my_id << " : " << simTime() << " " << to_log << endl;
       log_file.close();
       return;
   }


   void start_task(){
       // initial task function -> no relevance as of now
       send(new cMessage("Hello"),"outClientGates",0);
   }

};


Define_Module(Client);

void Client::initialize() {
    // a check to ensure that servers are initialized first and then client
    while(servers_connected.size() == 0){
        servers_connected = fetch_servers();
    }

    // fetching all clients that are available
    vector<vector<string>> clients_available = fetch_clients();
    // getting client id by registering in the network
    string temp = create_client(clients_available.size());
    if(temp != "None"){
        my_id = temp;
    }
    // the client number is for finding client gates efficiently
    my_client_number = clients_available.size();

    // logging purpose
    log_info("Client id generated");
    EV << "Client " << my_client_number << " " << my_id << " : " << simTime() << " " << "Client id generated" << endl;
    // saving connected clients for further use
    save_my_clients(clients_available);

    // create the first task
    Task first_task = create_task(true);
    current_task = first_task;
    distribute_task_to_servers();
}

void Client::handleMessage(cMessage *cmsg)
{
    Message msg = parse_message(cmsg->getName());


    if(msg.type == SUBTASK_RESPONSE){

        if(msg.isError){
                return;
            }

        EV << "Hello from client " << my_client_number << endl;

        // if this is the first server to send subtask result
        if(current_task.subtask_results.find(msg.subtaskresult.subtask_id) == current_task.subtask_results.end()){
            current_task.subtask_results[msg.subtaskresult.subtask_id] = {msg.subtaskresult};

        } else {
//            EV << "Client " << my_client_number << "received message " << cmsg->getName() << endl;
            current_task.subtask_results[msg.subtaskresult.subtask_id].push_back(msg.subtaskresult);
        }

//        EV << "Client " << my_client_number << " received message " << cmsg->getName() << " " <<  current_task.subtask_results.size() << " " << current_task.subtask_results[msg.subtaskresult.subtask_id].size() << endl;

        // Implies all subtask responses are received
        if(current_task.subtask_results[msg.subtaskresult.subtask_id].size() == current_task.servers_targeted.size()){
            cout << "Hello in subtask completion" << endl;
//            EV << "Client " << my_client_number << "received message " << cmsg->getName() << endl;

            // update the task result based on the completed subtask
            update_task_result(msg.subtaskresult.subtask_id);
            current_task.subtasks_done+=1;

            // logging purpose
            EV << "Client " << my_client_number << " " << current_task.final_result << endl;
        }

        // if all subtasks of current task are done
        if(current_task.subtasks_done == current_task.subtasks.size()){
            consolidate_server_scores();
//            EV << "Myself Client " << my_client_number << ". I have received all subtask results " << current_task.final_result << endl;
            broadcast_scores();
            tasks_done+=1;
            if(tasks_done != 2){
                Task task = create_task(false);
                task_history.push_back(current_task);
                current_task = task;
                distribute_task_to_servers();
            }
        }
    }

    else if(msg.type == SERVER_SCORES_UPDATE){
        if(msg.isError){
            return;
        }
        log_info(cmsg->getName());
        update_server_scores(msg);
        forward_server_scores(msg,cmsg->getName());
    }

}
