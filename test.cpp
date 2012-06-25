#include "CInteger.h"

#include <iostream>
#include <stdlib.h>
#include <cstdio>

int main(int argc, char ** argv) {
    if (argc < 4) {
        printf("Wrong parameters\n");
        return -1;
    };

    CInteger x(atoi(argv[1])), y(atoi(argv[3]));
    CInteger z;

    switch (*argv[2]) {
      case '*' : {
        z = x * y;
      } break;
      case '+' : {
        z = x + y;
      } break;
      case '-' : {
        z = x - y;
      } break;
      default : {
        return -2;
      };
    };

    std::cout << z.str() << "\n";
    std::cout << (z * CInteger(0x100000) * CInteger(0x100000) * CInteger(0x100000)).str() << "\n";
    std::cout << (z * CInteger(0x346346) * CInteger(0x657567) * CInteger(0x234234)).str() << "\n";
    std::cout << (z + CInteger(0x100000) + CInteger(0x100000) + CInteger(0x100000)).str() << "\n";

    for (unsigned i = 0; i < 7; i++) {
      z = z*z;
      std::cout << z.str() << "\n";
    }
    
    return 0;
}
