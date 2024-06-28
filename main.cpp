#include <iostream>
#include "terminal.hpp"
#include <thread>
#include <SFML/Graphics.hpp>

int main() {
    if(terminalOperations() == 1){
        printf("Terminal op error\n");
    }

    std::thread terminalThread(terminalOperations);
    terminalThread.join();
    return 0;
}
