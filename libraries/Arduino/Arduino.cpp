#include "Arduino.h"
#include <unistd.h>
#include <sstream>
#include <iostream>

using std::string;
using std::to_string;
using std::vector;
using std::unique_ptr;
using std::make_unique;
using std::atomic;
using std::stringstream;
using std::getline;
using namespace serial;

constexpr unsigned int ArduinoReadSleepUs = 10;
constexpr unsigned int ArduinoWriteSleepUs = 10;

unique_ptr<Serial> Arduino::my_serial = nullptr;
bool Arduino::shouldStop = false;

vector< unique_ptr< atomic<unsigned int> > > Arduino::inputs;
atomic<unsigned int> Arduino::readBufferIdx;
unsigned int Arduino::_numOfInputs = 1;
unsigned int Arduino::maxInPacketSize = 0;

atomic<float> Arduino::outputs[output_buffer_size];
int Arduino::outputs_readPtr = -1;
atomic<int> Arduino::outputs_writePtr;


void Arduino::listPorts()
{
    printf("-----SERIAL PORTS-----\n");
	vector<PortInfo> devices_found = list_ports();

	vector<PortInfo>::iterator iter = devices_found.begin();

	while( iter != devices_found.end() )
	{
		PortInfo device = *iter++;
		printf( "%s,\t%s,\t%s\n", device.port.c_str(), device.description.c_str(), device.hardware_id.c_str() );
	}
	printf("----------------------\n\n");
}


Arduino::Arduino()
{
    Arduino(1);
}

Arduino::Arduino(int numOfInputs)
{
    _numOfInputs = numOfInputs;
    my_serial = nullptr;
}

Arduino::~Arduino()
{
    close();
}

bool Arduino::open(int baudrate, string port) 
{
    my_serial = make_unique<Serial>(port, baudrate);

    if (!my_serial->isOpen())
	{
		printf("\nCould not open Arduino on port %s!\n\n", port.c_str());
		return false;
	}

    printf("Arduino open on port %s, with baudrate %d!\n", port.c_str(), baudrate);
    // always flush once at the beginning of connection
    my_serial->flushOutput();


    // prepare read loop vars
    maxInPacketSize = _numOfInputs*4 + 2; // each input can be up to 4 bytes [string "1023"], then we have a sapce separator and a '\n' delimiter
    
    // make space for all inputs we expect to receive at each transmission
    inputs.resize(_numOfInputs);
    for(auto &v : inputs)
    {
        v = make_unique< atomic<unsigned int> >(); // we allocate the atomic container
        v->store(0); // init all inputs to 0
    }
    readBufferIdx.store(0);

    // prepare write loop var
    outputs_writePtr.store(-1);

    // start loops
    shouldStop = false;
    pthread_create(&arduinoRead_thread, NULL, readLoop, NULL);
    pthread_create(&arduinoWrite_thread, NULL, writeLoop, NULL);
    
    return true;
}

void Arduino::close()
{
	if(my_serial != nullptr)
	{
		if(my_serial->isOpen())
        {
            // ask read loop to stop
            shouldStop = true;
            // wait for it to complete
            pthread_join(arduinoRead_thread, NULL);
            pthread_join(arduinoWrite_thread, NULL);
			my_serial->close();
        }
        // no need to delete/reset the ptr, it is done automatically on destruction or when a new ptr is created via open()
	}        

}

// this is designed to read continuous streams and it may miss one-time events!
int Arduino::read(int* values)
{
    if(my_serial != nullptr)
	{
		if(my_serial->isOpen())
        {
            // read the first input
            int retval = inputs[0]->load();

            // if we are set to read more than one input, copy all the inputs into the container
            if(values !=  nullptr)
            {
                values[0] = retval;
                for(int i=1; i<_numOfInputs; i++)
                    values[i] = inputs[i]->load();
            }       

            // no matter what, return the first one
            return retval;
        }
    }
    
    return -1;
}

void Arduino::write(float value)
{
    if(my_serial != nullptr)
	{
		if(my_serial->isOpen())
        {
            // read the index of the last value we wrote
            unsigned int loadIndex = outputs_writePtr.load();
            loadIndex = (loadIndex + 1) % output_buffer_size;
            outputs[loadIndex].store(value); // write latest value
            outputs_writePtr.store(loadIndex);  // Update index of the last value we wrote
        }    
    }
}

void Arduino::writeNow(float value)
{
    if(my_serial != nullptr)
	{
		if(my_serial->isOpen())
        {
            const string val = to_string(value) + '\n';
            my_serial->write(val);
		    my_serial->flushOutput(); // make sure the data is sent... but be careful, this is blocking
        }    
    }
}


//-----------------------------------------------------------------------------------------------------


void* Arduino::readLoop(void*)
{
    // working vectors wehere to store incoming strings for parsing
    vector<string> items;
    items.resize(_numOfInputs);

    vector<unsigned int> values;
    values.resize(_numOfInputs);

    // set minimum thread niceness
 	set_niceness(-20, false);

    // set thread priority
    set_priority(LDSPprioOrder_arduinoRead, false);

    while(!shouldStop)
    {
        // wait to buffer at least enough bytes to represent the maximum packet, i.e., all inputs set to 1023
        if(my_serial->available() >= maxInPacketSize)
        {
            //printf("________________%zu\n", my_serial->available());
            // Read the entire buffer
            string buffer;
            size_t bytes_read = my_serial->read(buffer, my_serial->available());  
            //printf("buffer: %s\n", buffer.c_str());

            // Split the buffer into strings based on the delimiter '\n'
            string latest_packet;
            size_t last_newline_pos = buffer.rfind('\n');  // Find the last newline
            if (last_newline_pos != string::npos) 
            {  
                // Check if a newline was found
                size_t prev_newline_pos = buffer.rfind('\n', last_newline_pos - 1);  // Find the previous newline
                if (prev_newline_pos == string::npos)
                    prev_newline_pos = 0; // The packet extends to the start of the buffer
                else 
                    prev_newline_pos += 1;  // The packet starts after the previous newline
                // Extract the packet
                latest_packet = buffer.substr(prev_newline_pos, last_newline_pos - prev_newline_pos);
            }
            
            // Check if any data was read
            if(latest_packet!="") 
            {            
                //printf("latest_packet: %s\n", latest_packet.c_str());
                // Split the line into space-separated items
                stringstream ss(latest_packet);
                string item;
                int index = 0;
                while(getline(ss, item, ' '))  
                    items[index++] = item;   
                
                // Check if the correct number of inputs was received in the latest packet
                if(index == _numOfInputs) 
                {  
                    bool wellFormatted = true;
                    // extract the received values as integer inputs
                    for(int i=0; i<_numOfInputs; i++) 
                    {
                        try 
                        {
                            values[i] = stoi(items[i]);  // Convert string to integer
                        } 
                        catch (const std::invalid_argument& e)
                        {
                            wellFormatted = false;
                            //std::cerr << "Arduino input - Invalid argument: " << e.what() << std::endl;
                        }
                        catch (const std::out_of_range& e)
                        {
                            wellFormatted = false;
                            //std::cerr << "Arduino input - Out of range: " << e.what() << std::endl;
                        }
                    }
                    // only if the whole packet is well formatted
                    if(wellFormatted)
                    {
                        for (int i = 0; i < _numOfInputs; i++)
                        {
                            inputs[i]->store(values[i]);
                            //printf("\tinputs %d: %d\n", i, values[i]);
                        }
                    }
                    // Now we have the latest packet of values in inputs
                }
            }
        }
        usleep(ArduinoReadSleepUs);
    }

    return (void *)0;
}


void* Arduino::writeLoop(void*)
{
    // set minimum thread niceness
 	set_niceness(-20, false);

    // set thread priority
    set_priority(LDSPprioOrder_arduinoRead, false);

    while(!shouldStop)
    {
        // here we use a non-blocking circular buffer to make sure that all output values are sent
        // this is guaranteed by the large size of the buffer, i.e., the store index [write pointer] will never overtake by a lap the load index [read pointer]

        // read the index of the latest value to write
        unsigned int loadIndex = outputs_writePtr.load();
        // until the index of the last write value is equal to the index of the last value we sent...
        while(outputs_readPtr != loadIndex) 
        { 
            // send as a string!
            const string val = to_string(outputs[outputs_readPtr].load()) + '\n';
            my_serial->write(val);
		    my_serial->flushOutput(); // make sure the data is sent... but be careful, this is blocking

            outputs_readPtr = (outputs_readPtr + 1) % output_buffer_size;;  // and update the index of the last value we sent
        }

        usleep(ArduinoWriteSleepUs);
    }
    return (void *)0;
}