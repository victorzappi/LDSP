#ifndef MYARDUINO_H
#define MYARDUINO_H

#include <string>
#include <memory> // for unique_ptr
#include "thread_utils.h"
#include "serial/serial.h"

// arduino boards often connect to this port
#define ARDUINO_PORT "/dev/ttyACM0"

class MyArduino {
public:
    static void listPorts();

    MyArduino();
    ~MyArduino();

    bool open(int baudrate=9600, std::string port=ARDUINO_PORT);

    void startRead(void (*callback)(int*,int), int inputSize, char separator);
    void close();

private:
    static std::unique_ptr<serial::Serial> my_serial;
    static bool shouldStop;

    pthread_t arduinoRead_thread;
    static unsigned int _inputSize;

    static char _separator;
    static void* readLoop(void*);
    static void (*inputsReadyCallback)(int*,int);
};

#endif // MYARDUINO_H