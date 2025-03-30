simple Server
{   
    parameters:
        @display("i=device/server");
    gates:
        input inClientGates[10];
        output outClientGates[10];
}

simple Client
{
    parameters:
        @display("i=device/laptop"); 
    gates:
        input inServerGates[5];
        input inClientGates[2];
        output outClientGates[2];
        output outServerGates[5];
}

network Network_Setup
{
    submodules:
        server[5]: Server;
        client[10]: Client;

    connections allowunconnected:
        for i = 0..9 {
            client[i].outClientGates[0] --> { delay = 1000ms; } --> client[(i + 1) % 10].inClientGates[0];
            client[i].outClientGates[1] --> { delay = 1000ms; } --> client[(i + 3) % 10].inClientGates[1];
        }

        for i = 0..9, for j = 0..4 {
            client[i].outServerGates[j] --> { delay = 1000ms; } --> server[j].inClientGates[i];
            server[j].outClientGates[i] --> { delay = 1000ms; } --> client[i].inServerGates[j];
        }
}
