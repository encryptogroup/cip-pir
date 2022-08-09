
#include <iostream>
#include <iomanip>
#include "helper.h"

using std::byte;
using std::cout;
using std::endl;

void printBytes(const byte *msg, uint32_t bytes) {
  for(uint32_t i = 0; i < bytes; i++) {
    auto hexa = static_cast<int> (msg[i]);
    cout << std::setw(2) << std::setfill('0') << std::hex << hexa;
  }
  cout << endl;
}
