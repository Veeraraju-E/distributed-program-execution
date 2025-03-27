#include <string>
#include <omnetpp.h>
#include <vector>
#include <iostream>
#include "helper.h"
#include <unordered_set>
#include <map>
#include <chrono>

#define MAX_ELEMENT_OF_ARRAY 10000000007

using namespace omnetpp;



// Client Tasks

/* Parser message
 * TODO: Task 03
 * Then, broadcast to all clients my score (Format must be gossip format) <self.timestamp>:<self.ID>:<self.Score#>
 * Ensure that we proceed for the second task only after broadcasting my score
 */

/*
 * TODO: Task 04
 * Selection of servers based on scores (Need to decide the algorithm)
 */



class Client : public cSimpleModule
{
  protected:
    // The following redefined virtual function holds the algorithm.
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;

  private:

    struct Subtask {
        string subtask_id;
        vector<int> input;
        int right_answer;
    };


    struct Task{
        string task_id;
        map<string,Subtask> subtasks;
        unordered_set<int> servers_targeted;
        map<string,vector<SubtaskResult>> subtask_results;
        int final_result = 0;
        int subtasks_done = 0;
    };

    vector<vector<string>> servers_connected = {};
    map<string,float> server_scores;
    vector<vector<string>> clients_connected = {};
    string my_id = "";
    int tasks_done = 0;
    int my_client_number = 0;
    Task current_task;
    vector<Task> task_history = {};
    vector<Message> message_history = {};


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


    string to_message(Subtask subtask){
        // Message Format needed: <Message_Type>:<client_id>:<sub_task_id>:<elements of array>(csv)
        string msg = "1:"+my_id+":"+subtask.subtask_id+":"+to_string(subtask.input[0])+","+to_string(subtask.input[1]);
        return msg;
    }

    Message parse_message(const string& msg){

        Message received_message;
        stringstream ss(msg);
        string token;
        getline(ss,token,':');
        if(token == "0"){
            // Response Format: <Message_Type>:<server_id>:<client_id>:<sub_task_id>:<result> for subtask result

            received_message.type = SUBTASK_RESPONSE;
            SubtaskResult result;
            getline(ss,token,':');

            result.server_id = token;
            getline(ss,token,':');
            cout << msg << " " << token << " " << my_id << endl;
            if(token != my_id){
                received_message.isError = true;
                received_message.subtaskresult = result;
                return received_message;
            }
            getline(ss,token,':');
            result.subtask_id = token;
            getline(ss,token,':');
            result.result = stoi(token);
            received_message.subtaskresult = result;
            cout << received_message.subtaskresult.result << " " << received_message.subtask_id << " " <<  received_message.subtaskresult.server_id << endl;
        }
        else if(token == "2"){
            // Response format: <Message_Type>:<timestamp>:<client_id>:<server_scores>:<msg_hash>

            // verification with hash pending
            received_message.type = SERVER_SCORES_UPDATE;
            getline(ss,token,':');
            getline(ss,token,':');
            received_message.client_id = token;
            getline(ss,token,':');

            string server_score_pair;
            stringstream server_scores_received(token);
            while(getline(server_scores_received,server_score_pair,',')){
                string server_id = server_score_pair.substr(0,server_score_pair.find('_'));
                cout << server_score_pair << endl;
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

        EV << "Client " << my_client_number << ' ' << task_digest << endl;

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
        // choosing the target servers
            unordered_set<int> chosen_servers;
            while(chosen_servers.size() != n/2 + 1){
                int server_index = rand() % n;
                chosen_servers.insert(server_index);
            }

            task.servers_targeted = chosen_servers;

      }

     return task;
    }

   void distribute_task_to_servers(){

       for(auto subtask: current_task.subtasks){
           for(auto server: current_task.servers_targeted){
               string msg = to_message(subtask.second);
               EV << "Client " << my_client_number << " " << msg << " to server" << server <<  endl;
               cMessage* msg_to_send = new cMessage(msg.c_str());
               send(msg_to_send,"outServerGates",server);

//               delete msg_to_send;
           }
       }
   }

   void update_task_result(const string& subtask_id){
       if(current_task.subtasks.find(subtask_id) == current_task.subtasks.end()){
           EV << "Received wrong subtask result " << subtask_id <<  " for the current task" << endl;
           return;
       }

       Subtask subtask = current_task.subtasks[subtask_id];
       vector<SubtaskResult> results = current_task.subtask_results[subtask_id];
       map<int,int> freq_results;
       for(auto result: results){
           if(freq_results.find(result.result) == freq_results.end()){
               freq_results[result.result] = 1;
           } else {
               freq_results[result.result]+=1;
           }
       }

       pair<int,int> correct_result = {0,-1};
       for(auto result: freq_results){
           if(result.second > correct_result.second){
               correct_result = result;
           }
       }

       if(current_task.final_result < correct_result.first){
           current_task.final_result = correct_result.first;
       }

       for(auto subtaskresult: results){
           if(subtaskresult.result != correct_result.first){
               subtaskresult.isWrongResult = true;
           }
       }
   }

   void consolidate_server_scores(){
       for(auto subtask: current_task.subtask_results){
           for(auto subtaskresult: subtask.second){
               string server_id = subtaskresult.server_id;
               if(server_scores.find(server_id) == server_scores.end()){
                   server_scores[server_id] = (subtaskresult.isWrongResult == false);
               } else {
                   server_scores[server_id]+= (subtaskresult.isWrongResult == false);
               }
           }
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
       pair<Message,string> msg_to_broadcast = get_server_scores();


       send(new cMessage(msg_to_broadcast.second.c_str()),"outClientGates",0);
       send(new cMessage(msg_to_broadcast.second.c_str()),"outClientGates",1);

       message_history.push_back(msg_to_broadcast.first);
   }

   bool is_message_in_history(Message msg){
       for(auto msg_in_history: message_history){
           if(msg.msg_hash == msg_in_history.msg_hash){
               return true;
           }
       }
       return false;
   }

   void forward_server_scores(Message msg,string msg_to_forward){

       if(!is_message_in_history(msg)){
          send(new cMessage(msg_to_forward.c_str()),"outClientGates",0);
          send(new cMessage(msg_to_forward.c_str()),"outClientGates",1);
      } else {
          return;
      }
       message_history.push_back(msg);
   }


   void update_server_scores(Message msg){
       for(auto server: msg.sever_scores){
           if(server_scores.find(server.first) == server_scores.end()){
               server_scores[server.first] = server.second;
           } else {
               server_scores[server.first]+=server.second;
           }
       }
   }


   void start_task(){
       send(new cMessage("Hello"),"outClientGates",0);
   }

};


Define_Module(Client);

void Client::initialize() {
    // a check to ensure that servers are initialized first and then client
    while(servers_connected.size() == 0){
        servers_connected = fetch_servers();
    }
    cout << servers_connected.size() << endl;
    vector<vector<string>> clients_available = fetch_clients();
    string temp = create_client(clients_available.size());
    if(temp != "None"){
        my_id = temp;
    }
    my_client_number = clients_available.size();
    EV << my_id << " " << my_client_number << endl;
    save_my_clients(clients_available);
    Task first_task = create_task(true);

    current_task = first_task;
    distribute_task_to_servers();
//    start_task();
}




void Client::handleMessage(cMessage *cmsg)
{
    Message msg = parse_message(cmsg->getName());


    if(msg.type == SUBTASK_RESPONSE){

        if(msg.isError){
                return;
            }

        EV << "Hello from client " << my_client_number << endl;

        if(current_task.subtask_results.find(msg.subtaskresult.subtask_id) == current_task.subtask_results.end()){
            current_task.subtask_results[msg.subtaskresult.subtask_id] = {msg.subtaskresult};

        } else {
//            EV << "Client " << my_client_number << "received message " << cmsg->getName() << endl;
            current_task.subtask_results[msg.subtaskresult.subtask_id].push_back(msg.subtaskresult);
        }

        EV << "Client " << my_client_number << " received message " << cmsg->getName() << " " <<  current_task.subtask_results.size() << " " << current_task.subtask_results[msg.subtaskresult.subtask_id].size() << endl;

        // Implies all sent subtask instances are received
        if(current_task.subtask_results[msg.subtaskresult.subtask_id].size() == current_task.servers_targeted.size()){
            cout << "Hello in subtask completion" << endl;
//            EV << "Client " << my_client_number << "received message " << cmsg->getName() << endl;
            update_task_result(msg.subtaskresult.subtask_id);
            current_task.subtasks_done+=1;
            EV << "Client " << my_client_number << " " << current_task.final_result << endl;
        }

        if(current_task.subtasks_done == current_task.subtasks.size() && tasks_done != 2){
            consolidate_server_scores();
            EV << "Myself Client " << my_client_number << ". I have received all subtask results " << current_task.final_result << endl;
            broadcast_scores();
            tasks_done+=1;
            Task task = create_task(false);
            task_history.push_back(current_task);
            current_task = task;
            distribute_task_to_servers();
        }
    }

    else if(msg.type == SERVER_SCORES_UPDATE){
        if(msg.isError){
            return;
        }
        update_server_scores(msg);
        forward_server_scores(msg,cmsg->getName());
    }

}
