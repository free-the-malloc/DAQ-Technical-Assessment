
#include <iostream>
#include <fstream>
#include <string.h>
#include <bitset>
#include <dbcppp/Network.h>

#define BUFLEN (128)

class dbc_parser {
public:
    std::unique_ptr<dbcppp::INetwork> net;
    std::unordered_map<uint64_t, const dbcppp::IMessage*> dbc_map;
    
    dbc_parser(char *dbc_path)
    {
        {
            std::ifstream idbc(dbc_path);
            net = dbcppp::INetwork::LoadDBCFromIs(idbc);
        }
        for (const dbcppp::IMessage& msg : net->Messages())
        {
            dbc_map.insert(std::make_pair(msg.Id(), &msg));
        }
    }
    
    void write_sigs_to_file(uint32_t id, const void *message, char *timestamp, std::ofstream& out)
    {
        auto iter = dbc_map.find(id);

        if (iter != dbc_map.end()){
            const dbcppp::IMessage* msg = iter->second;
            for (const dbcppp::ISignal& sig : msg->Signals())
            {
                const dbcppp::ISignal* mux_sig = msg->MuxSignal();
                if (sig.MultiplexerIndicator() != dbcppp::ISignal::EMultiplexer::MuxValue ||
                    (mux_sig && mux_sig->Decode(message) == sig.MultiplexerSwitchValue()))
                {
                    out << timestamp << ": " << sig.Name() << ": " << sig.RawToPhys(sig.Decode(message)) << "\n";
                }
            }
        }
    }
};

int main()
{   
    char buffer[BUFLEN];
    uint32_t id;
    uint64_t message;
    unsigned char bytes[8];
    char timestamp[32], interface[4];

    /* initialise dbc parser objects for each dbc file */
    dbc_parser can0("dbc-files/ControlBus.dbc");
    dbc_parser can1("dbc-files/SensorBus.dbc");
    dbc_parser can2("dbc-files/TractiveBus.dbc");

    /* open dump and outfile */
    std::FILE *dump = std::fopen("dump.log","r");
    std::ofstream outfile("output.txt");

    while(std::fgets(buffer,BUFLEN,dump) != NULL){
        std::sscanf(buffer,"%s %s %x#%lx",timestamp,interface,&id,&message);

        for (int i = 0 ; i < 8 ; i++){
            bytes[i] = (message >> (8 * (7 - i))) & 0xFF;
        }

        if (strncmp(interface,"can0",4) == 0){
            can0.write_sigs_to_file(id,bytes,timestamp,outfile);
        } else if (strncmp(interface,"can1",4) == 0){
            can1.write_sigs_to_file(id,bytes,timestamp,outfile);
        } else if (strncmp(interface,"can2",4) == 0){
            can2.write_sigs_to_file(id,bytes,timestamp,outfile);
        }
    }

    std::fclose(dump);
    outfile.close();
    
    return 0;
}
