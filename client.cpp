#include <iostream>
#include "efp/ElasticFrameProtocol.h"
#include "efpbond/EFPBonding.h"
#include "srtwrap/SRTNet.h"

#define MTU 1456 //SRT-max
#define WORKER_VIDEO_FRAMES 3000

#define NO_GROUP_INTERFACES 3
#define NO_GROUPS 1
EFPBonding::EFPBondingInterfaceID groupInterfacesID[NO_GROUP_INTERFACES];
EFPBonding::EFPBondingGroupID groupID[NO_GROUPS];

SRTNet mySRTNetIf1; //The SRT interface1
SRTNet mySRTNetIf2; //The SRT interface2
ElasticFrameProtocolSender myEFPSender(MTU); //EFP sender
EFPBonding myEFPBonding; //The EFP bonding plug in

//This will act as our encoder and just provide us with a H264 AnnexB frame when we want one.
std::vector<uint8_t> getNALUnit(int i) {
  std::string fileName = "../media/" + std::to_string(i) + ".h264";
  FILE *f = fopen(fileName.c_str(), "rb");
  if (!f) {
    std::cout << "Failed opening file" << std::endl;
    std::vector<uint8_t> empty;
    return empty;
  }
  fseek(f, 0, SEEK_END);
  size_t fSize = ftell(f);
  fseek(f, 0, SEEK_SET);
  uint8_t *pictureBuffer;
  int result = posix_memalign((void **) &pictureBuffer, 32, fSize);
  if (result) {
    std::cout << "Failed reserving memory" << std::endl;
    std::vector<uint8_t> empty;
    return empty;
  }
  size_t fResult = fread(pictureBuffer, 1, fSize, f);
  if (fResult != fSize) {
    std::cout << "Failed reading data" << std::endl;
    std::vector<uint8_t> empty;
    return empty;
  }
  std::vector<uint8_t> my_vector(&pictureBuffer[0], &pictureBuffer[fSize]);
  free(pictureBuffer);
  fclose(f);
  return my_vector;
}

void networkInterface1(const std::vector<uint8_t> &rSubPacket) {
  SRT_MSGCTRL thisMSGCTRL1 = srt_msgctrl_default;
  bool result = mySRTNetIf1.sendData((uint8_t *) rSubPacket.data(), rSubPacket.size(), &thisMSGCTRL1);
  if (!result) {
    std::cout << "Failed sending if1. Deal with that." << std::endl;
    //mySRTNetClient.stop(); ?? then reconnect?? try again for x times?? Notify the user?? Use a alternative socket??
  }
}

void networkInterface2(const std::vector<uint8_t> &rSubPacket) {
  SRT_MSGCTRL thisMSGCTRL1 = srt_msgctrl_default;
  bool result = mySRTNetIf2.sendData((uint8_t *) rSubPacket.data(), rSubPacket.size(), &thisMSGCTRL1);
  if (!result) {
    std::cout << "Failed sending if2. Deal with that." << std::endl;
    //mySRTNetClient.stop(); ?? then reconnect?? try again for x times?? Notify the user?? Use a alternative socket??
  }
}

//Just for debug
int packSize;

uint64_t packetCounter;
void sendData(const std::vector<uint8_t> &rSubPacket, uint8_t streamID) {

  //for debugging actual bytes sent (payload bytes)
  if (rSubPacket[0] == 1 || rSubPacket[0] == 3) {
    packSize += rSubPacket.size() - myEFPSender.geType1Size();
  }

  //for debugging actual bytes sent (payload bytes)
  if (rSubPacket[0] == 2) {
    packSize += rSubPacket.size() - myEFPSender.geType2Size();
    //std::cout << "Sent-> " << packSize << std::endl;
    packSize = 0;
  }


  //Here is the magic. And my approach is super simple
  //A real world implementation should look at whan retransmissions start and many other parameters.
  //EFPBonding is changing it's parameters every 100th fragment so that's why the if below. Higher frequency
  //modifications does not make any sense
  if (packetCounter == 0) {

    double allIfTotalEstimatedBW = 0;
    double If1EstimatedBW = 0;
    double If2EstimatedBW = 0;

    SRT_TRACEBSTATS currentClientStats = {0};
    mySRTNetIf1.getStatistics(&currentClientStats, SRTNetClearStats::yes, SRTNetInstant::yes);
    allIfTotalEstimatedBW += currentClientStats.mbpsBandwidth;
    If1EstimatedBW = currentClientStats.mbpsBandwidth;
    mySRTNetIf1.getStatistics(&currentClientStats, SRTNetClearStats::yes, SRTNetInstant::yes);
    allIfTotalEstimatedBW += currentClientStats.mbpsBandwidth;
    If2EstimatedBW = currentClientStats.mbpsBandwidth;

    //Should probably also check if allIfTotalEstimatedBW is lower than what you try to send.
    //Then ask the source to lower tha BW for example

    if (allIfTotalEstimatedBW) {
      double if1Commit = (If1EstimatedBW / allIfTotalEstimatedBW) * 100.0;
      double if2Commit = (If2EstimatedBW / allIfTotalEstimatedBW) * 100.0;
      if (if1Commit < 1.0) {
        if1Commit += 1.0;
        if2Commit -= 1.0;
      }
      if (if2Commit < 1.0) {
        if2Commit += 1.0;
        if1Commit -= 1.0;
      }


      std::cout << "if1BW: " << If1EstimatedBW <<
      " if2BW: " << If2EstimatedBW <<
      " if1commit: " << if1Commit << "%" <<
      " if2commit: " << if2Commit << "%" <<
      std::endl;

      std::vector<EFPBonding::EFPInterfaceCommit> myInterfaceCommits;
      myInterfaceCommits.push_back(EFPBonding::EFPInterfaceCommit(if1Commit, groupInterfacesID[0]));
      myInterfaceCommits.push_back(EFPBonding::EFPInterfaceCommit(if2Commit, groupInterfacesID[1]));
      EFPBondingMessages result = myEFPBonding.modifyInterfaceCommits(myInterfaceCommits,groupID[0]);
      if (result != EFPBondingMessages::noError) {
        std::cout << "Error modifyTotalGroupCommit -> " << unsigned(result) << std::endl;
      }
    } else {
      std::cout << "SRT reports no available BW.. I guess that's kind of fatal." << std::endl;
    }
  }

  myEFPBonding.distributeData(rSubPacket,0);

  packetCounter++;
  if (packetCounter == 100) {
    packetCounter = 0;
  }

}

void handleDataClient(std::unique_ptr<std::vector<uint8_t>> &content,
                      SRT_MSGCTRL &msgCtrl,
                      std::shared_ptr<NetworkConnection> ctx,
                      SRTSOCKET serverHandle) {
  std::cout << "Got data from server" << std::endl;
}

int main() {
  packSize = 0;
  packetCounter = 0;

  //Set-up framing protocol
  myEFPSender.sendCallback = std::bind(&sendData, std::placeholders::_1, std::placeholders::_2);

  //Set-up SRT
  auto client1Connection = std::make_shared<NetworkConnection>();
  mySRTNetIf1.recievedData = std::bind(&handleDataClient,
                                          std::placeholders::_1,
                                          std::placeholders::_2,
                                          std::placeholders::_3,
                                          std::placeholders::_4);
  if (!mySRTNetIf1.startClient("127.0.0.1", 8000, 16, 1000, 100, client1Connection, MTU)) {
    std::cout << "SRT if1 failed starting." << std::endl;
    return EXIT_FAILURE;
  }

  auto client2Connection = std::make_shared<NetworkConnection>();
  mySRTNetIf2.recievedData = std::bind(&handleDataClient,
                                       std::placeholders::_1,
                                       std::placeholders::_2,
                                       std::placeholders::_3,
                                       std::placeholders::_4);
  if (!mySRTNetIf2.startClient("127.0.0.1", 8000, 16, 1000, 100, client2Connection, MTU)) {
    std::cout << "SRT if2 failed starting." << std::endl;
    return EXIT_FAILURE;
  }

  //Create bonding group

  std::vector<EFPBonding::EFPInterface> lInterfaces;
  groupInterfacesID[0] = myEFPBonding.generateInterfaceID();
  lInterfaces.push_back(EFPBonding::EFPInterface(std::bind(&networkInterface1, std::placeholders::_1), groupInterfacesID[0], EFP_MASTER_INTERFACE));
  groupInterfacesID[1] = myEFPBonding.generateInterfaceID();
  lInterfaces.push_back(EFPBonding::EFPInterface(std::bind(&networkInterface2, std::placeholders::_1), groupInterfacesID[1], EFP_NORMAL_INTERFACE));
  groupID[0] = myEFPBonding.addInterfaceGroup(lInterfaces);
  if (!groupID[0]) {
    std::cout << "Failed bonding the interfaces" << std::endl;
    return EXIT_FAILURE;
  }

  //Generate some data to send
  uint64_t pts = 0;
  uint64_t dts = 0;
  for (int i = 0; i < WORKER_VIDEO_FRAMES; ++i) {
    std::vector<uint8_t> thisNalData = getNALUnit(i + 1);
    //std::cout << "SendNAL > " << thisNalData.size() << " pts " << pts << std::endl;
    //We got no DTS in this example set to PTS if no DTS is available (In this version of EFP DTS MUST be set to PTS if not available)
    dts = pts;
    myEFPSender.packAndSend(thisNalData,
                            ElasticFrameContent::h264,
                            pts,
                            dts,
                            EFP_CODE('A', 'N', 'X', 'B'),
                            1,
                            NO_FLAGS);
    //you could also send other data from other threads for example audio when the audio encoder spits out something.
    //myEFPSender.packAndSend(thisADTSData,ElasticFrameContent::adts,pts,'ADTS',1,NO_FLAGS);
    pts += 90000 / 60; //fake a pts of 60Hz. FYI.. the codestream is 23.98 (I and P only)
    usleep(1000 * 16); //sleep for 16ms ~60Hz
  }
  mySRTNetIf1.stop();
  mySRTNetIf2.stop();
  std::cout << "Done sending will exit" << std::endl;
  return 0;
}