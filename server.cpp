#include <iostream>
#include "efp/ElasticFrameProtocol.h"
#include "srtwrap/SRTNet.h"

#define MTU 1456 //SRT-max

SRTNet mySRTNetServer; //SRT

void gotData(ElasticFrameProtocol::pFramePtr &rPacket);

//**********************************
//Server part
//**********************************

// This is the class and everything you want to associate with a SRT connection
// You can see this as a classic c-'void* context' on steroids since SRTNet will own it and handle
// it's lifecycle. The destructor is called when the SRT connection is terminated. Magic!

class MyClass {
public:
  virtual ~MyClass() {
    *efpActiveElement = false; //Release active marker
    myEFPReceiver.stopReceiver();
  };
  uint8_t efpId = 0;
  std::atomic_bool *efpActiveElement;
  ElasticFrameProtocol myEFPReceiver;
};

// Array of 256 possible EFP receivers, could be millions but I just decided 256 change to your needs.
// You could make it much simpler just giving a new connection a uint64_t number++
std::atomic_bool efpActiveList[UINT8_MAX] = {false};
uint8_t getEFPId() {
  for (int i = 1; i < UINT8_MAX - 1; i++) {
    if (!efpActiveList[i]) {
      efpActiveList[i] = true; //Set active
      return i;
    }
  }
  return UINT8_MAX;
}

// Return a connection object. (Return nullptr if you don't want to connect to that client)
std::shared_ptr<NetworkConnection> validateConnection(struct sockaddr_in *sin) {
  auto *ip = (unsigned char *) &sin->sin_addr.s_addr;
  std::cout << "Connecting IP: " << unsigned(ip[0]) << "." << unsigned(ip[1]) << "." << unsigned(ip[2]) << "."
            << unsigned(ip[3]) << std::endl;
  // Validate connection.. Do we have resources to accept it? Is it from someone we want to talk to?

  //Get EFP ID.
  uint8_t efpId = getEFPId();
  if (efpId == UINT8_MAX) {
    std::cout << "Unable to accept more EFP connections " << std::endl;
    return nullptr;
  }

  // Here we can put whatever into the connection. The object we embed is maintained by SRTNet
  // In this case we put MyClass in containing the EFP ID we got from getEFPId() and a EFP-receiver
  auto a1 = std::make_shared<NetworkConnection>(); // Create a connection
  a1->object = std::make_shared<MyClass>(); // And my object containing my stuff
  auto v = std::any_cast<std::shared_ptr<MyClass> &>(a1->object); //Then get a pointer to my stuff
  v->efpId = efpId; // Populate it with the efpId
  v->efpActiveElement =
      &efpActiveList[efpId]; // And a pointer to the list so that we invalidate the id when SRT drops the connection
  v->myEFPReceiver.receiveCallback =
      std::bind(&gotData, std::placeholders::_1); //In this example we aggregate all callbacks..
  v->myEFPReceiver.startReceiver(10, 2);
  return a1; // Now hand over the ownership to SRTNet
}

//Network data recieved callback.
bool handleData(std::unique_ptr<std::vector<uint8_t>> &content,
                SRT_MSGCTRL &msgCtrl,
                std::shared_ptr<NetworkConnection> ctx,
                SRTSOCKET clientHandle) {
  //We got data from SRTNet
  auto v = std::any_cast<std::shared_ptr<MyClass> &>(ctx->object); //Get my object I gave SRTNet
  v->myEFPReceiver.receiveFragment(*content,
                                   v->efpId); //unpack the fragment I got using the efpId created at connection time.
  return true;
}

//ElasticFrameProtocol got som data from some efpSource.. Everything you need to know is in the rPacket
//meaning EFP stream number EFP id and content type. if it's broken the PTS value
//code with additional information of payload variant and if there is embedded data to extract and so on.
void gotData(ElasticFrameProtocol::pFramePtr &rPacket) {
  std::cout << "BAM... Got some NAL-units of size " << unsigned(rPacket->mFrameSize) <<
            " pts " << unsigned(rPacket->mPts) <<
            " is broken? " << rPacket->mBroken <<
            " from EFP connection " << unsigned(rPacket->mSource) <<
            std::endl;
}

int main() {

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

