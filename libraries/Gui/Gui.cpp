// This code is based on the code credited below, but it has been modified
// further by Victor Zappi

 /*
 ___  _____ _        _
| __ )| ____| |      / \
|  _ \|  _| | |     / _ \
| |_) | |___| |___ / ___ \
|____/|_____|_____/_/   \_\

The platform for ultra-low latency audio and sensor processing

http://bela.io

A project of the Augmented Instruments Laboratory within the Centre for Digital Music at Queen Mary University of London. http://instrumentslab.org

(c) 2016-2020 Augmented Instruments Laboratory: Andrew McPherson, Astrid Bin, Liam Donovan, Christian Heinrichs, Robert Jack, Giulio Moro, Laurel Pardue, Victor Zappi. All rights reserved.

The Bela software is distributed under the GNU Lesser General Public License (LGPL 3.0), available here: https://www.gnu.org/licenses/lgpl-3.0.txt */


#include "Gui.h"
#include "WebServer.h"
#include <seasocks/PageHandler.h>
#include <seasocks/Request.h>
#include <seasocks/ResponseBuilder.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <filesystem>
namespace fs = std::__fs::filesystem;


class GuiPageHandler : public seasocks::PageHandler 
{
public:
    GuiPageHandler(std::string projectName) : _projectName(projectName) {}

    std::shared_ptr<seasocks::Response> handle(const seasocks::Request& request) override 
    {
        const std::string uri = request.getRequestUri();
        const std::string basePath = "/data/ldsp/";

        // printf("uri: %s\n", uri.c_str());

        // this is needed to pass web socket requests to the web socket handler
        if(request.verb() == seasocks::Request::Verb::WebSocket) 
            return seasocks::Response::unhandled();

        // printf("uri filtered: %s\n", uri.c_str());

        // Function to check if a string ends with another string
        auto endsWith = [](const std::string &fullString, const std::string &ending) 
        {
            if (fullString.length() >= ending.length()) 
                return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
            else 
                return false;
        };

        //TODO move some of these to default serve func?

        // Handling for css files in the /gui/js/ directory
        if (uri.find("/gui/css/") == 0) 
        {
            std::string filePath = basePath + "resources" + uri;
            return serveFile(filePath, "text/css");
        }

        // Handling for js files in the /gui/js/ directory
        if (uri.find("/gui/js/") == 0) 
        {
            std::string filePath = basePath + "resources" + uri; 
            return serveFile(filePath, "application/javascript");
        }

        // Handling for js files in the /js/ directory
        if (uri.find("/js/") == 0) {
            std::string filePath = basePath + "resources" + uri;
            return serveFile(filePath, "application/javascript");
        }
        
        // Handling for /gui/gui-template.html
        if (uri == "/gui/gui-template.html") 
            return serveFile( basePath + "resources/gui/gui-template.html", "text/html");
        
        // Handling for /gui/p5-sketches/sketch.js
        if (uri == "/gui/p5-sketches/sketch.js")
            return serveFile( basePath + "resources/gui/p5-sketches/sketch.js", "application/javascript");

        // Handling for font files in the /fonts/ directory
        if (uri.find("/fonts/") == 0) 
        {
            std::string filePath = basePath + "resources" + uri;

            // Extract the file extension
            std::string extension = uri.substr(uri.find_last_of(".") + 1);

            // Determine the MIME type based on the file extension
            std::string mimeType;
            if (extension == "woff")
                mimeType = "font/woff";
            else if (extension == "woff2")
                mimeType = "font/woff2";
            else if (extension == "ttf")
                mimeType = "font/ttf";

            return serveFile(filePath, mimeType);
        }

        
        // Dynamic handling for project-specific files
        if (uri.find("/projects/") == 0) 
        {
            std::string filePath;
            if (endsWith(uri, "/main.html"))
                filePath = basePath + "projects/" + _projectName + "/main.html";
            else if (endsWith(uri, ".js"))
                filePath = basePath + "projects/" + _projectName + "/sketch.js";


            // Check if file exists
            if (fs::exists(filePath))
                return serveFile(filePath, endsWith(uri, ".js") ? "application/javascript" : "text/html");
            else 
            {
                // File not found handling
                seasocks::ResponseBuilder builder(seasocks::ResponseCode::NotFound);
                builder.withContentType("text/plain");
                builder << "File not found";
                return builder.build();
            }
        }

        // Default case for serving the main HTML file
        if (uri == "/" || uri == "/gui/index.html" || uri == "/gui/") 
        {
            std::string filePath = basePath + "resources/gui/index.html";
            return serveFile( filePath, "text/html");
        }


        // Default case for URIs that don't match any known patterns
        seasocks::ResponseBuilder builder(seasocks::ResponseCode::NotFound);
        builder.withContentType("text/plain");
        builder << "Resource not found for URI: " << uri;
        return builder.build();
    }

private:
    std::shared_ptr<seasocks::Response> serveFile(const std::string& path, const std::string& mimeType) 
    {
        std::ifstream file(path, std::ios::binary);
        // printf("\ttrying to serve: %s\n", path.c_str());
        if(file) 
        {
            // printf("\t\tserved: %s\n", path.c_str());
            seasocks::ResponseBuilder builder(seasocks::ResponseCode::Ok);
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            builder.withContentType(mimeType);
            builder << content;
            return builder.build();
        } 
        else 
        {
            //printf("\t\tcannot serve: %s\n", path.c_str());
            seasocks::ResponseBuilder builder(seasocks::ResponseCode::NotFound);
            builder.withContentType("text/plain");
            builder << "File not found";
            return builder.build();
        }
    }

    std::string _projectName;
};


Gui::Gui()
{
}

Gui::Gui(unsigned int port, std::string address)
{
	setup(port, address);
}

int Gui::setup(unsigned int port, std::string address)
{
	_port = port;
	_addressData = address+"_data";
	_addressControl = address+"_control";

	// Set up the webserver
    if(web_server)
        web_server.reset();
    web_server = std::make_unique<WebServer>();
	web_server->setup(_projectName, "GUI", port);

	web_server->addPageHandler(std::make_shared<GuiPageHandler>(_projectName));

	web_server->addAddress(_addressData,
		[this](std::string address, void* buf, int size)
		{
			ws_onData((const char*) buf, size);
		},
	 nullptr, nullptr, true);

	web_server->addAddress(_addressControl,
		// onData()
		[this](std::string address, void* buf, int size)
		{
			ws_onControlData((const char*) buf, size);
		},
		// onConnect()
		[this](std::string address)
		{
			ws_connect();
		},
		// onDisconnect()
		[this](std::string address)
		{
			ws_disconnect();
		}
	);

	web_server->run();

	return 0;
}

int Gui::setup(std::string projectName, unsigned int port, std::string address)
{
	_projectName = projectName;
	setup(port, address);
	return 0;
}
/*
 * Called when websocket is connected.
 * Communication is started here with the server sending a 'connection' JSON object
 * with initial settings.
 * The client replies with 'connection-ack', which should be parsed accordingly.
 */
void Gui::ws_connect()
{
	// printf("++++++connect %ls\n", _projectName.c_str());
	// send connection JSON
	nlohmann::json root;
	root["event"] = "connection";
	if(!_projectName.empty())
		root["projectName"] = _projectName;  

	sendControl(root);
}

/*
 * Called when websocket is disconnected.
 *
 */
void Gui::ws_disconnect()
{
	wsIsConnected = false;
}

/*
 *  on_data callback for scope_control websocket
 *  runs on the (linux priority) seasocks thread
 */
void Gui::ws_onControlData(const char* data, unsigned int size)
{
	try {
		// parse data
		nlohmann::json value = nlohmann::json::parse(data);
		
		// look for the "event" key
		if(customOnControlData && !customOnControlData(value, controlCallbackArg))
		{
			return;
		}
		if (value.contains("event") && value["event"].is_string()){
			std::string event = value["event"];
			if (event.compare("connection-reply") == 0){
				wsIsConnected = true;
			}
    }
	} catch (const nlohmann::json::parse_error& e) {
		fprintf(stderr, "Could not parse JSON:\n%s\n", data);
	}

	return;
}

//TODO make this thread-safe via atomics
void Gui::ws_onData(const char* data, unsigned int size)
{
	// printf("++++++data %s\n", data);

	if(customOnData && !customOnData(data, size, binaryCallbackArg))
	{
		return;
	}
	else
	{
		uint32_t bufferId = *(uint32_t*) data;
		data += sizeof(uint32_t);
		char bufferType = *data;
		data += sizeof(uint32_t);
		uint32_t bufferLength = *(uint32_t*) data;
		uint32_t numBytes = (bufferType == 'c' ? bufferLength : bufferLength * sizeof(float));
		data += 2*sizeof(uint32_t);
		if(bufferId < _buffers.size())
		{
			if(bufferType != _buffers[bufferId].getType())
			{
				fprintf(stderr, "Buffer %d: received buffer type (%c) doesn't match original buffer type (%c).\n", bufferId, bufferType, _buffers[bufferId].getType());
			}
			else
			{
				if(numBytes > _buffers[bufferId].getCapacity())
				{
					fprintf(stderr, "Buffer %d: size of received buffer (%d bytes) exceeds that of the original buffer (%d bytes). The received data will be trimmed.\n", bufferId, numBytes, _buffers[bufferId].getCapacity());
					numBytes = _buffers[bufferId].getCapacity();
				}
				// Copy data to buffers
				getDataBuffer(bufferId).getBuffer()->assign(data, data + numBytes);
			}
		}
		else
		{
			fprintf(stderr, "Received buffer ID %d is out of range.\n", bufferId);
		}
	}
	return;
}

// BUFFERS
unsigned int Gui::setBuffer(char bufferType, unsigned int size)
{
	unsigned int buffId = _buffers.size();
	_buffers.emplace_back(bufferType, size);
	return buffId;
}

DataBuffer& Gui::getDataBuffer( unsigned int bufferId )
{
	if(bufferId >= _buffers.size())
		throw std::runtime_error((std::string("Buffer ID ")+std::to_string((int)bufferId)+std::string(" is out of range.\n")).c_str());

	return _buffers[bufferId];
}

Gui::~Gui()
{
	cleanup();
}
void Gui::cleanup()
{
}

int Gui::sendControl(nlohmann::json root) {
	std::string str = root.dump();
	//printf("************************send %s\n", str.c_str());
    return web_server->send(_addressControl.c_str(), str.c_str());
}

int Gui::doSendBuffer(const char* type, unsigned int bufferId, const void* data, size_t size)
{
	std::string idTypeStr = std::to_string(bufferId) + "/" + std::string(type);
	int ret;
	if(0 == (ret = web_server->send(_addressData.c_str(), idTypeStr.c_str())))
                    if(0 == (ret = web_server->send(_addressData.c_str(), (void*)data, size)))
                            return 0;
	fprintf(stderr, "You are sending messages to the GUI too fast. Please slow down\n");
	return ret;
}
