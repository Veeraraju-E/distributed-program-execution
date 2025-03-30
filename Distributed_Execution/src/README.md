# Distributed Program Execution with Byzantine Fault Tolerance

## System Architecture

### Network Setup (Network_Setup.ned)
- we use dynamic network setup using a python script named `ned_file_generator.py`

## Detailed Module Workflow

### 1. Client Module (Client.cc)

#### Initialization Phase
```cpp
void Client::initialize() {
    1. Fetch available servers from server_list.txt
    2. Generate unique client ID using timestamp
    3. Save connected clients (next and previous in ring)
    4. Initialize first task
}
```

#### Task Management
1. **Task Creation**
   ```cpp
   Task create_task(bool is_first_task) {
       - Generate random array of 2n elements (n = server count)
       - Create unique task_id using hash
       - Split into n subtasks (2 elements each)
       - Select target servers:
           * First task: Random n/2 + 1 servers
           * Subsequent tasks: Top performing n/2 + 1 servers
   }
   ```

2. **Task Distribution**
   ```cpp
   void distribute_task_to_servers() {
       For each subtask:
           - Format message: "1:<client_id>:<subtask_id>:<data>"
           - Send to all selected servers
           - Log distribution details
   }
   ```

3. **Result Processing**
   ```cpp
   void update_task_result(subtask_id) {
       - Collect all server responses for subtask
       - Calculate result frequencies
       - Determine majority result
       - Update task's final result
       - Mark incorrect server responses
   }
   ```

4. **Server Score Management**
   ```cpp
   void consolidate_server_scores() {
       For each subtask result:
           - Update server scores based on correctness
           - Log updated scores
   }
   ```

5. **Score Broadcasting**
   ```cpp
   void broadcast_scores() {
       - Format: "2:<timestamp>:<client_id>:<server_scores>:<hash>"
       - Send to connected clients
       - Add to message history
   }
   ```

### 2. Server Module (Server.cc)

#### Initialization
```cpp
void Server::initialize() {
    - Generate unique server ID
    - Register in server_list.txt
    - Initialize malicious behavior tracking
}
```

#### Message Processing
```cpp
void Server::handleMessage(cMessage *msg) {
    1. Parse incoming message
    2. Extract subtask data
    3. Process task with potential malicious behavior
    4. Send response to client
}
```

#### Malicious Behavior Implementation
```cpp
int process_task(const vector<int>& arr, const string& subtask_id) {
    1. Extract task_id from subtask_id
    2. Check existing malicious decision for task
    3. If new task:
       - 25% probability of being malicious
       - Store decision for consistency
    4. Return result:
       - Normal: Maximum value
       - Malicious: Minimum value
}
```

### 3. Helper Module (helper.h, helper.cc)

#### Core Structures
```cpp
struct Message {
    MessageType type;
    string client_id;
    string subtask_id;
    vector<int> data;
    string msg_hash;
    SubtaskResult subtaskresult;
    bool isError;
    map<string,float> sever_scores;
}

struct SubtaskResult {
    string subtask_id;
    string server_id;
    int result;
    simtime_t timestamp;
    bool isWrongResult;
}
```

#### Utility Functions
- `generate_hash()`: Creates unique identifiers
- `fetch_servers()`: Reads server registry
- `fetch_clients()`: Reads client registry
- `create_server()`: Registers new server
- `create_client()`: Registers new client

## Complete Workflow

1. **System Initialization**
   - Servers initialize first, register in server_list.txt
   - Clients initialize, establish ring connections
   - Each client generates unique ID and connects to all servers

2. **Task Execution (Per Client)**
   - Client generates first task with random numbers
   - Selects random n/2 + 1 servers
   - Distributes subtasks to selected servers
   - Servers process subtasks (some maliciously)
   - Client collects responses and determines majority
   - Updates server reputation scores
   - Broadcasts scores to other clients

3. **Score Propagation**
   - Clients receive score broadcasts
   - Update local server reputation data
   - Forward broadcasts to maintain network consistency
   - Use updated scores for next task's server selection

4. **Second Task Execution**
   - Client creates second task
   - Selects top-performing servers based on scores
   - Repeats distribution and collection process
   - Updates and broadcasts final server scores

## Byzantine Fault Tolerance Implementation

1. **Server-side**
   - 25% probability of malicious behavior per task
   - Consistent malicious behavior throughout task
   - Returns minimum instead of maximum value

2. **Client-side**
   - Sends each subtask to n/2 + 1 servers
   - Implements majority voting for result verification
   - Maintains server reputation system
   - Shares reputation data across network

## Problem Requirements and Solutions

### 1. Byzantine Fault Tolerance Requirements

#### Requirement: Handle Malicious Servers
**Solution Implemented:**
- System tolerates f Byzantine faults where n ≥ 3f + 1 (n = total servers)
- With 8 servers, system handles up to 2 malicious servers
- Each subtask sent to 5 servers (n/2 + 1) ensuring majority consensus
```cpp
// In Client.cc
void create_task(bool is_first_task) {
    // Ensures n/2 + 1 servers are selected
    while(chosen_servers.size() != n/2 + 1) {
        int server_index = rand() % n;
        chosen_servers.insert(server_index);
    }
}
```

#### Requirement: Consistent Malicious Behavior
**Solution Implemented:**
- Servers maintain consistent malicious/honest behavior throughout a task
- 25% probability of being malicious for each new task
```cpp
// In Server.cc
map<string, bool> is_malicious_for_task;
const double MALICIOUS_PROBABILITY = 0.25;

bool will_be_malicious = (dis(gen) < MALICIOUS_PROBABILITY);
is_malicious_for_task[task_id] = will_be_malicious;
```

### 2. Network Communication Requirements

#### Requirement: Ring Topology for Clients
**Solution Implemented:**
- Clients arranged in ring topology
- Each client connected to two other clients
- Efficient score broadcasting mechanism
```cpp
// In Client.cc
void save_my_clients(vector<vector<string>> clients_available) {
    for(int i = 0; i < clients_available.size(); i++) {
        if(client_number == (my_client_number+1)%4 || 
           client_number == (my_client_number+3)%4) {
            clients_connected.push_back(clients_available[i]);
        }
    }
}
```

#### Requirement: Full Mesh Server Connectivity
**Solution Implemented:**
- Each client connected to all servers
- Enables flexible server selection
- Maintains system reliability even with failed servers

### 3. Task Processing Requirements

#### Requirement: Distributed Task Execution
**Solution Implemented:**
- Tasks split into subtasks
- Each subtask processed independently
- Results aggregated using majority voting
```cpp
// In Client.cc
void update_task_result(const string& subtask_id) {
    map<int,int> freq_results;
    // Count frequency of each result
    for(auto result: results) {
        freq_results[result.result]++;
    }
    // Select majority result
    pair<int,int> correct_result = {0,-1};
    for(auto result: freq_results) {
        if(result.second > correct_result.second) {
            correct_result = result;
        }
    }
}
```

### 4. Reputation System Requirements

#### Requirement: Server Score Management
**Solution Implemented:**
- Dynamic server reputation tracking
- Scores based on result correctness
- Network-wide score propagation
```cpp
// In Client.cc
void consolidate_server_scores() {
    for(auto subtask_result: results) {
        if(subtask_result.isWrongResult) {
            // Decrease score for wrong results
            server_scores[subtask_result.server_id] -= penalty;
        } else {
            // Increase score for correct results
            server_scores[subtask_result.server_id] += reward;
        }
    }
}
```

### 5. Fault Detection Requirements

#### Requirement: Identify Malicious Behavior
**Solution Implemented:**
- Result comparison across multiple servers
- Majority-based correctness verification
- Score adjustment for detected malicious behavior
```cpp
// In Client.cc
void update_task_result(const string& subtask_id) {
    // Mark wrong results as malicious
    for(auto subtaskresult: results) {
        if(subtaskresult.result != correct_result.first) {
            subtaskresult.isWrongResult = true;
        }
    }
}
```

### 6. System Reliability Requirements

#### Requirement: Ensure Result Correctness
**Solution Implemented:**
- Multiple server execution
- Majority voting mechanism
- Consistent result verification
- Server reputation tracking

#### Requirement: Handle Network Delays
**Solution Implemented:**
- Asynchronous message processing
- 1000ms network delay simulation
- Message queuing and ordering
```cpp
// In Network_Setup.ned
channel Channel extends ned.DelayChannel {
    delay = 1000ms;
}
```

### 7. Logging Requirements

#### Requirement: Comprehensive System Monitoring
**Solution Implemented:**
- Detailed client and server logs
- Task creation and distribution tracking
- Result processing logs
- Score broadcasting logs
```cpp
// In Client.cc
void log_info(string to_log) {
    ofstream log_file("./src/client_log.txt", ios::app);
    log_file << "Client " << my_client_number << " " 
             << my_id << " : " << simTime() << " " 
             << to_log << endl;
}
```

## Logging and Verification

- Client logs:
  * Task creation and distribution
  * Server responses and majority decisions
  * Score updates and broadcasts
  * Network message propagation

- Server logs:
  * Task processing decisions
  * Malicious behavior tracking
  * Response transmission

This implementation ensures reliable distributed computation with Byzantine fault tolerance through:
- Redundant task distribution
- Majority-based result verification
- Network-wide server reputation tracking
- Consistent malicious behavior detection
