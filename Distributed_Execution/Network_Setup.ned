simple Client
{
    gates:
        input inServerGates[8];
        input inClientGates[2];
        output outClientGates[2];
        output outServerGates[8];
}

simple Server
{
    gates:
        input inClientGates[4];
        output outClientGates[4];
}

network Network_Setup
{
    submodules:
        server[8]: Server;
        client[4]: Client;

    connections allowunconnected:
        for i = 0..3 {
            client[i].outClientGates[0] --> { delay = 1000ms; } --> client[(i + 1) % 4].inClientGates[0];
            client[i].outClientGates[1] --> { delay = 1000ms; } --> client[(i + 3) % 4].inClientGates[1];
        }

        for i = 0..3, for j = 0..7 {
            client[i].outServerGates[j] --> { delay = 1000ms; } --> server[j].inClientGates[i];
            server[j].outClientGates[i] --> { delay = 1000ms; } --> client[i].inServerGates[j];
        }
}