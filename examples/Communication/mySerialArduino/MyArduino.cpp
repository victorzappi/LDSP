#include "MyArduino.h"
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <memory>
#include "serial/serial.h"

using namespace serial;
using namespace std;

unique_ptr<Serial> MyArduino::my_serial;
bool MyArduino::shouldStop = false;
unsigned int MyArduino::_inputSize;
char MyArduino::_separator = ',';
void (*MyArduino::inputsReadyCallback)(int*, int) = nullptr;


void MyArduino::listPorts() {
    vector<PortInfo> devices_found = list_ports();
    for (const auto& device : devices_found) {
        printf("%s,\t%s,\t%s\n", device.port.c_str(), device.description.c_str(), device.hardware_id.c_str());
    }
}

MyArduino::MyArduino() {
    _inputSize = 1; // default number of inputs
    my_serial = nullptr;
}

MyArduino::~MyArduino() {
    close();
}

bool MyArduino::open(int baudrate, string port) {
    my_serial = make_unique<Serial>(port, baudrate);
    if (!my_serial->isOpen()) {
        printf("\nCould not open Arduino on port %s!\n\n", port.c_str());
        return false;
    }
    printf("Arduino open on port %s, with baudrate %d!\n", port.c_str(), baudrate);
    my_serial->flushOutput();

    return true;
}

void MyArduino::startRead(void (*callback)(int*, int), int inputSize, char separator) {
    inputsReadyCallback = callback;
    _inputSize = inputSize;
    _separator = separator;

    printf("Arduino read\n");

    shouldStop = false;
    pthread_create(&arduinoRead_thread, NULL, &MyArduino::readLoop, NULL);
}

void MyArduino::close() {
    shouldStop = true;
    if (my_serial && my_serial->isOpen()) {
        pthread_join(arduinoRead_thread, NULL);
        my_serial->close();
    }
}

void* MyArduino::readLoop(void* args) {
    string buffer; // Buffer to store incoming data across reads
    int inputs[500]; // Temporarily stores data inputs before they are sent out
    int inputNum;
    string tempBuffer="";

    while (!shouldStop && my_serial && my_serial->isOpen()) {
        size_t available = my_serial->available();
       
        if (available) {
            inputNum = 0;
            my_serial->read(buffer, available); // Read available data
            // printf("temp buff %s\n", tempBuffer.c_str());
            // printf("buff %s\n", buffer.c_str());
            // buffer = tempBuffer + buffer;
            
            // Initialize the position variable for finding separators in the buffer
            size_t pos;
            // Continue looping as long as there are separators found in the buffer
            
            while ((pos = buffer.find("\n")) != string::npos) {
                // Extract the substring from the beginning of the buffer up to the position of next new line
                string newLine = buffer.substr(0, pos);

                // Remove the processed part of the buffer, including the separator itself
                buffer.erase(0, pos + 1);
                int partialInputNum;
                size_t inputPos;
                for(partialInputNum = 0; partialInputNum<_inputSize-1; partialInputNum++){
                    inputPos = buffer.find(_separator);
                    if(inputPos == string::npos)
                        break;
                    
                    string number = newLine.substr(0,inputPos);
                    newLine.erase(0, inputPos + 1);

                    if (!number.empty()) {
                        try 
                        {
                            inputs[inputNum + partialInputNum]=stoi(number); // Convert the cleaned token to an integer and store it in the inputs vector
                        } 
                        catch (const std::exception&) // Catch any exception thrown by stoi, typically due to invalid conversion
                        { 
                            // cerr << "Invalid number received: " << number << endl; // Log the error to standard error
                            break;
                        }
                    }

                }

                // Look for the last expected input 
                if (partialInputNum == _inputSize - 1){
                    // if we don't find the last input, set inputNum back to currentInputNum
                    if(!newLine.empty()){
                        try 
                        {
                            //newLine.erase(remove(newLine.begin(),newLine.end(),','));
                            inputs[inputNum + partialInputNum]=stoi(newLine); // Convert the cleaned token to an integer and store it in the inputs vector
                            inputNum += _inputSize;
                        } 
                        catch (const std::exception&) // Catch any exception thrown by stoi, typically due to invalid conversion
                        { 
                            // cerr << "Invalid new line received:" << newLine << endl; // Log the error to standard error
                            break;
                        }
                        
                    }
                    
                }

                

 
            }

            tempBuffer = buffer;    
            // Check if we have collected inputs
            if (inputNum > 0) {
                if (inputsReadyCallback) {
                    inputsReadyCallback(inputs, inputNum); // Send data via callback
                }
            }



        }



        usleep(10000); // Sleep to prevent CPU hogging
    }
    return NULL;
}

