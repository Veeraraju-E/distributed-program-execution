simple Client
{
    gates:
        input inServerGates[4];
        input inClientGates[2];
        output outClientGates[2];
        output outServerGates[4];
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
        client[4]: Client;
        server[4]: Server;

    connections allowunconnected:
        for i = 0..3 {
            client[i].outClientGates[0] --> { delay = 1000ms; } --> client[(i + 1) % 4].inClientGates[0];
            client[i].outClientGates[1] --> { delay = 1000ms; } --> client[(i + 3) % 4].inClientGates[1];
        }

        for i = 0..3, for j = 0..3 {
            client[i].outServerGates[j] --> { delay = 1000ms; } --> server[j].inClientGates[i];
            server[j].outClientGates[i] --> { delay = 1000ms; } --> client[i].inServerGates[j];
        }
}