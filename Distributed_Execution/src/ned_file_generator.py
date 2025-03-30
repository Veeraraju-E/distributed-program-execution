"""
© B22CS080 - Veeraraju Elluru, B22CS052 - Sumeet S Patil
This file is has original work. All rights are reserved under the jurisdiction of IIT Jodhpur
 """

with open("topo.txt", "r") as topo_file:
    network_parameters = topo_file.readlines()

# Extract the number of servers and clients
_, _, no_servers = network_parameters[0].split(" ")
_, _, no_clients = network_parameters[1].split(" ")

no_servers, no_clients = int(no_servers), int(no_clients)

copy_rights = """
 // © B22CS080 - Veeraraju Elluru, B22CS052 - Sumeet S Patil
 // This file is has original work. All rights are reserved under the jurisdiction of IIT Jodhpur
 """

Client_Module = f"""simple Client
{{
    parameters:
        @display("i=device/laptop"); 
    gates:
        input inServerGates[{no_servers}];
        input inClientGates[2];
        output outClientGates[2];
        output outServerGates[{no_servers}];
}}"""

Server_Module = f"""simple Server
{{   
    parameters:
        @display("i=device/server");
    gates:
        input inClientGates[{no_clients}];
        output outClientGates[{no_clients}];
}}
"""

Network_Module = f"""
network Network_Setup
{{
    submodules:
        server[{no_servers}]: Server;
        client[{no_clients}]: Client;

    connections allowunconnected:
        for i = 0..{no_clients-1} {{
            client[i].outClientGates[0] --> {{ delay = 1000ms; }} --> client[(i + 1) % {no_clients}].inClientGates[0];
            client[i].outClientGates[1] --> {{ delay = 1000ms; }} --> client[(i + 3) % {no_clients}].inClientGates[1];
        }}

        for i = 0..{no_clients-1}, for j = 0..{no_servers-1} {{
            client[i].outServerGates[j] --> {{ delay = 1000ms; }} --> server[j].inClientGates[i];
            server[j].outClientGates[i] --> {{ delay = 1000ms; }} --> client[i].inServerGates[j];
        }}
}}
"""

ned_content = copy_rights + "\n" + Server_Module + "\n" + Client_Module + "\n" + Network_Module
with open("../Network_Setup.ned", "w") as ned_file:
    ned_file.write(ned_content)

server_connections = ["\nServers:\n{"]
for j in range(no_servers):
    clients_connected = [f"Client {i}" for i in range(no_clients)]
    server_connections.append(f"    Server {j} : [{', '.join(clients_connected)}],")
server_connections.append("}\n")

client_connections = ["Clients:\n{"]
for i in range(no_clients):
    servers_connected = [f"Server {j}" for j in range(no_servers)]
    client_neighbors = [f"Client {(i + 1) % no_clients}", f"Client {(i + 3) % no_clients}"]
    client_connections.append(f"    Client {i} : [{', '.join(servers_connected + client_neighbors)}],")
client_connections.append("}\n")


with open("topo.txt", "a") as topo_file:
    topo_file.write("\n".join(server_connections) + "\n" + "\n".join(client_connections) + "\n")
