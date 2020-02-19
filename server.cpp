#include <iostream>
#include "efp/ElasticFrameProtocol.h"
#include "srtwrap/SRTNet.h"

#define MTU 1456 //SRT-max

SRTNet mySRTNetServer; //SRT

void gotData(ElasticFrameProtocolReceiver::pFramePtr &rPacket);

//**********************************
//Server part
//**********************************

//This server is super simple and not dynamic. It assumes all connections are from the same bonding interface
//A real implementation need to signal som kind of token binding the connections together

ElasticFrameProtocolReceiver myEFPReceiver(10, 2);

// Return a connection object. (Return nullptr if you don't want to connect to that client)
std::shared_ptr<NetworkConnection> validateConnection(struct sockaddr_in *sin) {
  auto *ip = (unsigned char *) &sin->sin_addr.s_addr;
  std::cout << "Connecting IP: " << unsigned(ip[0]) << "." << unsigned(ip[1]) << "." << unsigned(ip[2]) << "."
            << unsigned(ip[3]) << std::endl;
  // Validate connection.. Do we have resources to accept it? Is it from someone we want to talk to?

  auto a1 = std::make_shared<NetworkConnection>(); // Create a connection
  return a1; // Now hand over the ownership to SRTNet
}

//Network data recieved callback.
bool handleData(std::unique_ptr<std::vector<uint8_t>> &content,
                SRT_MSGCTRL &msgCtrl,
                std::shared_ptr<NetworkConnection> ctx,
                SRTSOCKET clientHandle) {
  //We got data from SRTNet
  ElasticFrameMessages result = myEFPReceiver.receiveFragment(*content,0);
  if (result != ElasticFrameMessages::noError) {
    std::cout << "Error" << std::endl;
  }
  return true;
}

//ElasticFrameProtocol got som data from some efpSource.. Everything you need to know is in the rPacket
//meaning EFP stream number EFP id and content type. if it's broken the PTS value
//code with additional information of payload variant and if there is embedded data to extract and so on.
void gotData(ElasticFrameProtocolReceiver::pFramePtr &rPacket) {
  std::cout << "BAM... Got some NAL-units of size " << unsigned(rPacket->mFrameSize) <<
            " pts " << unsigned(rPacket->mPts) <<
            " is broken? " << rPacket->mBroken <<
            " from EFP connection " << unsigned(rPacket->mSource) <<
            std::endl;
}

int main() {

  //Set-up EFP
  myEFPReceiver.receiveCallback = std::bind(&gotData, std::placeholders::_1);

  //Setup and start the SRT server
  mySRTNetServer.clientConnected = std::bind(&validateConnection, std::placeholders::_1);
  mySRTNetServer.recievedData = std::bind(&handleData,
                                          std::placeholders::_1,
                                          std::placeholders::_2,
                                          std::placeholders::_3,
                                          std::placeholders::_4);
  if (!mySRTNetServer.startServer("0.0.0.0", 8000, 16, 1000, 100, MTU)) {
    std::cout << "SRT Server failed to start." << std::endl;
    return EXIT_FAILURE;
  }

  //Run this server until ........
  while (true) {
    sleep(1);
  }

  //When you decide to quit garbage collect and stop threads....
  mySRTNetServer.stop();

  std::cout << "Done serving. Will exit." << std::endl;
  return 0;
}

