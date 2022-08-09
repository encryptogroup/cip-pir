#include "../CIP_PIR/Client/CIP_pirClient.h"
#include "../CIP_PIR/Computation/helper.h"
#include <chrono>

#define COUNT 1000
#define TIME2

using namespace std;

void request(){
    string filename = "database/raw.db.manifest";
    string username = "dan";
    string password = "gun";

    #ifdef TIME2
        std::chrono::high_resolution_clock::duration duration = std::chrono::high_resolution_clock::duration::zero();
    #endif
    for(int i = 0; i < COUNT; ++i){
        auto party = RAID_pirClient (filename);
        party.addServer ("127.0.0.1", 7123);
        party.addServer("127.0.0.1", 7124);
        cout << i << "/" << COUNT << endl;

        #ifdef TIME2
            auto start = std::chrono::high_resolution_clock::now();
        #endif
        //party.request (i+1);
        party.checkPassword(username, password);
        #ifdef TIME2
            auto end = std::chrono::high_resolution_clock::now();
            duration += (end - start);
        #endif
        party.close();
    }

    auto d = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

    std::cout << "#Q=" << std::dec << COUNT << " TIME2: " << std::dec << d.count() << " ms" << std::endl;
}

int main () {


#ifdef TIME2
    auto startTIME2 = getMilliSecs();
#endif
    // auto response = party.checkPassword(username, password);
#ifdef TIME2
    auto finalTIME2 = getMilliSecs();
  cout << "C3 TIME2: " << finalTIME2 - startTIME2 << " ms" << endl;
#endif
    request();


    // printBytes (data.begin(), BLOCKSIZE);

}

