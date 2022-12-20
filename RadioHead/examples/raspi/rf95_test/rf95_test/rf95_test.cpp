#include <pigpio.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <map>
#include <iostream>
#include <fstream>
#include <array>
#include <string.h>
#include <time.h>
#include <cstring>
#include <sstream>
#include "structures.h"

// Function Definitions
void sig_handler(int sig);

// Driver for module used
#include <RH_RF95.h>

// Driver for mesh capability
#include <RHMesh.h>

// Max message length
#define RH_MESH_MAX_MESSAGE_LEN 50

// Flag for join request message
#define RH_FLAGS_JOIN_REQUEST 0x1f

// Pins used
#define RFM95_CS_PIN 8
#define RFM95_IRQ_PIN 4

// Network of 6 nodes
std::map<int, bool> node_status_map;
#define NODE1_ADDRESS 11
#define NODE2_ADDRESS 22
#define THIS_NODE_ADDRESS 33
#define NODE4_ADDRESS 44
#define NODE5_ADDRESS 55
#define NODE6_ADDRESS 66

// RFM95 Configuration
#define RFM95_FREQUENCY 915.00
#define RFM95_TXPOWER 14

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS_PIN, RFM95_IRQ_PIN);

RHMesh manager(rf95, THIS_NODE_ADDRESS);

// Flag for Ctrl-C to end the program.
int flag = 0;

// File writer global variables
std::string path = "/media/node3/node3ssd/Node Data/";
std::string fileName = "";
std::string packetTimeStamp = "packetTimeStamp";
std::string logTimeStamp = "logFileTimeStamp";
bool file_exists = false;

// Indicates the start state of the node.
int state = 7; // All nodes start in join request state

// DNP3 packet struct
struct DNP3Packet
{
  std::string sync;
  std::string length;
  std::string link_control;
  std::string destination_address;
  std::string source_address;
  std::string crc;
  std::string phase_angle;
  std::string phase_on_each_bus;
  std::string power_flow_on_each_transmission_line;
  std::string substation_load;
  std::string substation_component_status;
  std::string time_stamp;
};

/** Create a timestamp using the computer's date and time and returns it as a sting.
Received a string that would tell the fuction which format to use for the timestamp.*/
std::string getCurrentDateTime(std::string s)
{
  time_t now = time(0);
  struct tm timeStruct;
  char timeStamp[20];
  timeStruct = *localtime(&now);

  // Log file timestamp
  if (s == "logFileTimeStamp")
  {
    strftime(timeStamp, sizeof(timeStamp), "%Y-%m-%d %H:%M:%S", &timeStruct);
  }
  // Packet timestamp
  else if (s == "packetTimeStamp")
  {
    strftime(timeStamp, sizeof(timeStamp), "%Y-%m-%d_%H-%M-%S", &timeStruct);
  }

  return timeStamp;
}

/* Received an array containing the data from the LoRa packet.
It stores the data in a struct simulating the DNP3Packet and returns it.*/
DNP3Packet DNP3PacketGenerator(std::array<std::string, 10> packetContent)
{
  DNP3Packet packet;

  // DNP3Packet header
  packet.sync = "Sync";
  packet.length = "Length";
  packet.link_control = "Link Contol";
  packet.destination_address = "Destination Adress";
  packet.source_address = "Source Adress";
  packet.crc = "CRC";

  // DNP3Packet data
  packet.phase_angle = packetContent[0];
  packet.phase_on_each_bus = packetContent[1];
  packet.power_flow_on_each_transmission_line = packetContent[2];
  packet.substation_load = packetContent[3];
  packet.substation_component_status = packetContent[4];

  // DNP3Packet timestamp
  packet.time_stamp = getCurrentDateTime(packetTimeStamp);

  return packet;
}

/* Received the array where the data of the LoRa packet is stored when the message is sent and the timestamp of when the packet was created.
 Extracts the data from the packet and returns a string array with the data and the timestamp of the packet*/
std::array<std::string, 10> packetReader(uint8_t data[], std::string timeStamp)
{
  std::array<std::string, 10> packetContent;

  packetContent[0] = std::to_string(int(data[2]));
  packetContent[1] = std::to_string(int(data[3]));
  packetContent[2] = std::to_string(int(data[4]));
  packetContent[3] = std::to_string(int(data[5]));
  packetContent[4] = std::to_string(int(data[6]));
  packetContent[5] = timeStamp;

  return packetContent;
}

/* This function receives the path to the where the file is gonna be saved, the name of the file and a array containing the data of the packetContent.
 It creates a file of type csv and writes a file in csv format of the data from the packet.
 It creates a log file of type txt that contains the date the file was created, the name of the file and its directory path .*/
void fileWriter(std::string path, std::string fileName, std::array<std::string, 10> packetContent)
{
  std::string timeStamp = "";
  bool fileWasCreated = false;
  timeStamp = packetContent[5];

  std::ofstream file;

  // Concatenates the the path, fileName, timestamp and the type together into the variable fileDirectory.
  std::string fileDirectory = path + fileName + timeStamp + ".csv";

  // Creates the nodes data file. If the files doesn't exist it will be created and if the file exist it will just open it.
  file.open(fileDirectory);

  // If the file was able to be open it will write into the file else it will not write into the file.
  // After the file was written it will close the file and chage the variable fileWasCreated to true.
  if (file.is_open())
  {
    file << "Substation Parameters,Values,Measurtments Units \n";
    file << "Phase angle," + packetContent[0] + "," + "degrees \n";
    file << "Phase on each bus," + packetContent[1] + "," + "kW \n";
    file << "Power flow on each transmission line," + packetContent[2] + "," + "MW \n";
    file << "Substation load," + packetContent[3] + "," + "kW \n";
    file << "Substation component status," + packetContent[4] + "," + "boolean \n";
    file.close();
    fileWasCreated = true;
  }

  // If fileWasCreated is true it will create and update the log file.
  if (fileWasCreated)
  {
    // Creates the log file. If the files doesn't exist it will be created in append mode and if the file exist it will open it and update the contebts of the file.
    file.open(path + "Node Data.log", std::ofstream ::app);

    // If the file was able to be open it will write into the file else it will not write into the file.
    // After the file was written it will close the file and chage the variable fileWasCreated to false.
    if (file.is_open())
    {
      file << "[" + getCurrentDateTime(logTimeStamp) + "]" + " File created:[" << fileName + timeStamp + "]" + " File directory path:[" << fileDirectory + "]"
                                                                                                                                                           "\n";
      file.close();
      fileWasCreated = false;
    }
  }
}

// Encrypt fuctions

/* Serves as the initial round during encryption
 * AddRoundKey is simply an XOR of a 128-bit block with the 128-bit key.
 */
void AddRoundKey(unsigned char *state, unsigned char *roundKey)
{
  for (int i = 0; i < 16; i++)
  {
    state[i] ^= roundKey[i];
  }
}

/* Perform substitution to each of the 16 bytes
 * Uses S-box as lookup table
 */
void SubBytesEncryp(unsigned char *state)
{
  for (int i = 0; i < 16; i++)
  {
    state[i] = s[state[i]];
  }
}

// Shift left, adds diffusion
void ShiftRowsEncryp(unsigned char *state)
{
  unsigned char tmp[16];

  /* Column 1 */
  tmp[0] = state[0];
  tmp[1] = state[5];
  tmp[2] = state[10];
  tmp[3] = state[15];

  /* Column 2 */
  tmp[4] = state[4];
  tmp[5] = state[9];
  tmp[6] = state[14];
  tmp[7] = state[3];

  /* Column 3 */
  tmp[8] = state[8];
  tmp[9] = state[13];
  tmp[10] = state[2];
  tmp[11] = state[7];

  /* Column 4 */
  tmp[12] = state[12];
  tmp[13] = state[1];
  tmp[14] = state[6];
  tmp[15] = state[11];

  for (int i = 0; i < 16; i++)
  {
    state[i] = tmp[i];
  }
}

/* MixColumns uses mul2, mul3 look-up tables
 * Source of diffusion
 */
void MixColumns(unsigned char *state)
{
  unsigned char tmp[16];

  tmp[0] = (unsigned char)mul2[state[0]] ^ mul3[state[1]] ^ state[2] ^ state[3];
  tmp[1] = (unsigned char)state[0] ^ mul2[state[1]] ^ mul3[state[2]] ^ state[3];
  tmp[2] = (unsigned char)state[0] ^ state[1] ^ mul2[state[2]] ^ mul3[state[3]];
  tmp[3] = (unsigned char)mul3[state[0]] ^ state[1] ^ state[2] ^ mul2[state[3]];

  tmp[4] = (unsigned char)mul2[state[4]] ^ mul3[state[5]] ^ state[6] ^ state[7];
  tmp[5] = (unsigned char)state[4] ^ mul2[state[5]] ^ mul3[state[6]] ^ state[7];
  tmp[6] = (unsigned char)state[4] ^ state[5] ^ mul2[state[6]] ^ mul3[state[7]];
  tmp[7] = (unsigned char)mul3[state[4]] ^ state[5] ^ state[6] ^ mul2[state[7]];

  tmp[8] = (unsigned char)mul2[state[8]] ^ mul3[state[9]] ^ state[10] ^ state[11];
  tmp[9] = (unsigned char)state[8] ^ mul2[state[9]] ^ mul3[state[10]] ^ state[11];
  tmp[10] = (unsigned char)state[8] ^ state[9] ^ mul2[state[10]] ^ mul3[state[11]];
  tmp[11] = (unsigned char)mul3[state[8]] ^ state[9] ^ state[10] ^ mul2[state[11]];

  tmp[12] = (unsigned char)mul2[state[12]] ^ mul3[state[13]] ^ state[14] ^ state[15];
  tmp[13] = (unsigned char)state[12] ^ mul2[state[13]] ^ mul3[state[14]] ^ state[15];
  tmp[14] = (unsigned char)state[12] ^ state[13] ^ mul2[state[14]] ^ mul3[state[15]];
  tmp[15] = (unsigned char)mul3[state[12]] ^ state[13] ^ state[14] ^ mul2[state[15]];

  for (int i = 0; i < 16; i++)
  {
    state[i] = tmp[i];
  }
}

/* Each round operates on 128 bits at a time
 * The number of rounds is defined in AESEncrypt()
 */
void RoundEncryp(unsigned char *state, unsigned char *key)
{
  SubBytesEncryp(state);
  ShiftRowsEncryp(state);
  MixColumns(state);
  AddRoundKey(state, key);
}

// Same as Round() except it doesn't mix columns
void FinalRound(unsigned char *state, unsigned char *key)
{
  SubBytesEncryp(state);
  ShiftRowsEncryp(state);
  AddRoundKey(state, key);
}

/* The AES encryption function
 * Organizes the confusion and diffusion steps into one function
 */
void AESEncrypt(unsigned char *message, unsigned char *expandedKey, unsigned char *encryptedMessage)
{
  unsigned char state[16]; // Stores the first 16 bytes of original message

  for (int i = 0; i < 16; i++)
  {
    state[i] = message[i];
  }

  int numberOfRounds = 9;

  AddRoundKey(state, expandedKey); // Initial round

  for (int i = 0; i < numberOfRounds; i++)
  {
    RoundEncryp(state, expandedKey + (16 * (i + 1)));
  }

  FinalRound(state, expandedKey + 160);

  // Copy encrypted state to buffer
  for (int i = 0; i < 16; i++)
  {
    encryptedMessage[i] = state[i];
  }
}

// Decrypt fuctions

/* Used in Round() and serves as the final round during decryption
 * SubRoundKey is simply an XOR of a 128-bit block with the 128-bit key.
 * So basically does the same as AddRoundKey in the encryption
 */
void SubRoundKey(unsigned char *state, unsigned char *roundKey)
{
  for (int i = 0; i < 16; i++)
  {
    state[i] ^= roundKey[i];
  }
}

/* InverseMixColumns uses mul9, mul11, mul13, mul14 look-up tables
 * Unmixes the columns by reversing the effect of MixColumns in encryption
 */
void InverseMixColumns(unsigned char *state)
{
  unsigned char tmp[16];

  tmp[0] = (unsigned char)mul14[state[0]] ^ mul11[state[1]] ^ mul13[state[2]] ^ mul9[state[3]];
  tmp[1] = (unsigned char)mul9[state[0]] ^ mul14[state[1]] ^ mul11[state[2]] ^ mul13[state[3]];
  tmp[2] = (unsigned char)mul13[state[0]] ^ mul9[state[1]] ^ mul14[state[2]] ^ mul11[state[3]];
  tmp[3] = (unsigned char)mul11[state[0]] ^ mul13[state[1]] ^ mul9[state[2]] ^ mul14[state[3]];

  tmp[4] = (unsigned char)mul14[state[4]] ^ mul11[state[5]] ^ mul13[state[6]] ^ mul9[state[7]];
  tmp[5] = (unsigned char)mul9[state[4]] ^ mul14[state[5]] ^ mul11[state[6]] ^ mul13[state[7]];
  tmp[6] = (unsigned char)mul13[state[4]] ^ mul9[state[5]] ^ mul14[state[6]] ^ mul11[state[7]];
  tmp[7] = (unsigned char)mul11[state[4]] ^ mul13[state[5]] ^ mul9[state[6]] ^ mul14[state[7]];

  tmp[8] = (unsigned char)mul14[state[8]] ^ mul11[state[9]] ^ mul13[state[10]] ^ mul9[state[11]];
  tmp[9] = (unsigned char)mul9[state[8]] ^ mul14[state[9]] ^ mul11[state[10]] ^ mul13[state[11]];
  tmp[10] = (unsigned char)mul13[state[8]] ^ mul9[state[9]] ^ mul14[state[10]] ^ mul11[state[11]];
  tmp[11] = (unsigned char)mul11[state[8]] ^ mul13[state[9]] ^ mul9[state[10]] ^ mul14[state[11]];

  tmp[12] = (unsigned char)mul14[state[12]] ^ mul11[state[13]] ^ mul13[state[14]] ^ mul9[state[15]];
  tmp[13] = (unsigned char)mul9[state[12]] ^ mul14[state[13]] ^ mul11[state[14]] ^ mul13[state[15]];
  tmp[14] = (unsigned char)mul13[state[12]] ^ mul9[state[13]] ^ mul14[state[14]] ^ mul11[state[15]];
  tmp[15] = (unsigned char)mul11[state[12]] ^ mul13[state[13]] ^ mul9[state[14]] ^ mul14[state[15]];

  for (int i = 0; i < 16; i++)
  {
    state[i] = tmp[i];
  }
}

// Shifts rows right (rather than left) for decryption
void ShiftRowsDecryp(unsigned char *state)
{
  unsigned char tmp[16];

  /* Column 1 */
  tmp[0] = state[0];
  tmp[1] = state[13];
  tmp[2] = state[10];
  tmp[3] = state[7];

  /* Column 2 */
  tmp[4] = state[4];
  tmp[5] = state[1];
  tmp[6] = state[14];
  tmp[7] = state[11];

  /* Column 3 */
  tmp[8] = state[8];
  tmp[9] = state[5];
  tmp[10] = state[2];
  tmp[11] = state[15];

  /* Column 4 */
  tmp[12] = state[12];
  tmp[13] = state[9];
  tmp[14] = state[6];
  tmp[15] = state[3];

  for (int i = 0; i < 16; i++)
  {
    state[i] = tmp[i];
  }
}

/* Perform substitution to each of the 16 bytes
 * Uses inverse S-box as lookup table
 */
void SubBytesDecryp(unsigned char *state)
{
  for (int i = 0; i < 16; i++)
  { // Perform substitution to each of the 16 bytes
    state[i] = inv_s[state[i]];
  }
}

/* Each round operates on 128 bits at a time
 * The number of rounds is defined in AESDecrypt()
 * Not surprisingly, the steps are the encryption steps but reversed
 */
void RoundDecryp(unsigned char *state, unsigned char *key)
{
  SubRoundKey(state, key);
  InverseMixColumns(state);
  ShiftRowsDecryp(state);
  SubBytesDecryp(state);
}

// Same as Round() but no InverseMixColumns
void InitialRound(unsigned char *state, unsigned char *key)
{
  SubRoundKey(state, key);
  ShiftRowsDecryp(state);
  SubBytesDecryp(state);
}

/* The AES decryption function
 * Organizes all the decryption steps into one function
 */
void AESDecrypt(unsigned char *encryptedMessage, unsigned char *expandedKey, unsigned char *decryptedMessage)
{
  unsigned char state[16]; // Stores the first 16 bytes of encrypted message

  for (int i = 0; i < 16; i++)
  {
    state[i] = encryptedMessage[i];
  }

  InitialRound(state, expandedKey + 160);

  int numberOfRounds = 9;

  for (int i = 8; i >= 0; i--)
  {
    RoundDecryp(state, expandedKey + (16 * (i + 1)));
  }

  SubRoundKey(state, expandedKey); // Final round

  // Copy decrypted state to buffer
  for (int i = 0; i < 16; i++)
  {
    decryptedMessage[i] = state[i];
  }
}

/**Method to verify if the last broadcast was sent from the node before your turn */
bool prevNode(int prevnode_id, std::map<int, bool> node_map)
{
  bool var = false; // boolean that indicates if it is prevNode
  bool none = false;
  std::map<int, bool>::iterator itr;
  std::map<int, bool>::iterator pol;
  pol = node_map.end();
  pol->second = false;
  pol = node_map.find(THIS_NODE_ADDRESS);
  itr = node_map.find(prevnode_id);
  // so we start on the node after
  itr++;
  for (; itr != node_map.end(); itr++)
  {
    std::cout << itr->first << " :: " << itr->second << std::endl;
    printf(" while 1");
    if (itr->second == true) // If the node is "up"
    {
      if (itr->first == THIS_NODE_ADDRESS) // If the node's id is MY id
      {
        itr = node_map.find(prevnode_id);
        itr->second = false; // change previous node's status to false
        var = true;
        break;
      }
      break;
    }
    if (itr == node_map.end())
    {
      none = true;
      break;
    }
  }
  if (none)
  {
    for (itr = node_map.begin(); itr != node_map.end(); itr++)
    {
      std::cout << itr->first << " :: " << itr->second << std::endl;
      printf(" for 1");
      if (itr->second == true)
      {
        if (itr->first == THIS_NODE_ADDRESS)
        {
          itr = node_map.find(prevnode_id);
          itr->second = false;
          var = true;
          break;
        }
        break;
      }
    }
  }
  printf("return var\n");
  return var;
}

// Main Function
int main(int argc, const char *argv[])
{

  if (gpioInitialise() < 0) // pigpio library function that initiliazes gpio
  {
    printf("\n GPIOs could not be initialized");
    return 1;
  }
  gpioSetSignalFunc(2, sig_handler); // 2 is SIGINT. Ctrl+C will cause signal.

  // Verify Raspi startup
  printf("\nRPI rf95_test startup OK.\n");
  printf("\nRPI GPIO settings:\n");
  printf("CS-> GPIO %d\n", (uint8_t)RFM95_CS_PIN);
  printf("IRQ-> GPIO %d\n", (uint8_t)RFM95_IRQ_PIN);

  // Verify driver initialization
  if (!rf95.init())
  {
    printf("\n\nRF95 Driver Failed to initialize.\n\n");
    return 1;
  }

  if (!manager.init())
  {
    printf("\n\nMesh Manager Failed to initialize.\n\n");
    return 1;
  }

  /* Begin Manager/Driver settings code */
  printf("\nRFM 95 Settings:\n");
  printf("Frequency= %d MHz\n", (uint16_t)RFM95_FREQUENCY);
  printf("Power= %d\n", (uint8_t)RFM95_TXPOWER);
  printf("This Address= %d\n", THIS_NODE_ADDRESS);
  rf95.setTxPower(RFM95_TXPOWER, true);
  rf95.setFrequency(RFM95_FREQUENCY);
  rf95.setModemConfig(RH_RF95::Bw125Cr48Sf4096);
  // Bw500Cr45Sf128
  /* End Manager/Driver settings code */

  /*Node map status initialise*/
  node_status_map.insert(std::pair<int, bool>(NODE1_ADDRESS, false));
  node_status_map.insert(std::pair<int, bool>(NODE2_ADDRESS, false));
  node_status_map.insert(std::pair<int, bool>(THIS_NODE_ADDRESS, false));
  node_status_map.insert(std::pair<int, bool>(NODE4_ADDRESS, false));
  node_status_map.insert(std::pair<int, bool>(NODE5_ADDRESS, false));
  node_status_map.insert(std::pair<int, bool>(NODE6_ADDRESS, false));

  // File wrIter variables
  std::array<std::string, 10> packetContent;
  DNP3Packet packet;

  /* Placeholder Message  */
  uint8_t data[50];
  uint8_t buf[50];
  uint8_t dupe_buf[50];
  /* End Placeholder Message */

  // Encryption key
  unsigned char key[16] = {0x01, 0x04, 0x02, 0x03, 0x01, 0x03, 0x04, 0x0a, 0x09, 0x0b, 0x07, 0x0f, 0x0c, 0x06, 0x03, 0x00};
  char message[50];

  /* timeouts start */
  // retry to send your broadcast
  unsigned long retrySend = 15000;
  unsigned long retryStartTimer;
  // Node turn timer
  unsigned long turnTimer = 60000;
  unsigned long startTurnTimer;
  // Join request resend timer
  unsigned long joinResendTimer = 15000;
  unsigned long joinResendStartTimer;
  // Retry sending turn
  unsigned long retry_turn_timeout = 15000;
  unsigned long retry_turn_timer;
  // wait 7 seconds in receive mode to make receiving join requests easier
  unsigned long wait_timer;
  // last broadcast received timout
  unsigned long last_broadcast_received = 60000;
  unsigned long last_broadcast_received_timer;

  /* timeouts end */

  // Keep track of how many times the node has resent the join request.
  int retry = 0;
  // Keep track of how many times the node has resent the turn message.
  int turn_retry = 0;
  // Edge case where there are only 2 nodes in the network.
  bool two_nodes = false;
  // Edge case
  bool master_node = false;
  // Acknowledge the turn message
  bool turn_ack = false;
  // new node has joined the network
  bool new_node = false;
  // new node id
  int new_node_id = 0;
  // wait in receive for any join requests
  bool send_turn = false;
  // Network Session Key (4 digits)
  uint8_t NSK;
  // Keep track of who sent the join request
  uint8_t _from;

  uint8_t from, to; // stores the address of the node that the message was from, and the destination address respectively
  uint8_t buflen = sizeof(buf);
  uint8_t dupe_buflen = sizeof(buf);

  while (!flag)
  {
    /*State 1: Node sends broadcast of DNP3 packet*/
    if (state == 1) // sending
    {
      sleep(2);
      master_node = true;
      uint8_t datalen = sizeof(data);
      std::string timeStamp = "";
      message[0] = RH_FLAGS_RETRY;

      // Providing a seed value
      srand((unsigned)time(NULL));

      // Generates random data simulating the data from the substation
      message[2] = 1 + (rand() % 91);
      message[3] = 1 + (rand() % 101);
      message[4] = 1 + (rand() % 101);
      message[5] = 1 + (rand() % 101);
      message[6] = 0 + (rand() % 2);

      std::cout << "Message to encrypt:" << std::endl;
      printf("%d", message[2]);
      std::cout << " ";
      printf("%d", message[3]);
      std::cout << " ";
      printf("%d", message[4]);
      std::cout << " ";
      printf("%d", message[5]);
      std::cout << " ";
      printf("%d", message[6]);
      std::cout << " ";

      timeStamp = getCurrentDateTime(packetTimeStamp);

      int j = 0;

      for (int i = 7; i <= 25; i++)
      {
        message[i] = timeStamp[j];
        j++;
        std::cout << message[i];
      }

      std::cout << std::endl;

      // Pad message to 16 bytes
      int originalLen = 26;

      int paddedMessageLen = originalLen;

      if ((paddedMessageLen % 16) != 0)
      {
        paddedMessageLen = (paddedMessageLen / 16 + 1) * 16;
      }

      unsigned char *paddedMessage = new unsigned char[paddedMessageLen];
      for (int i = 0; i < paddedMessageLen; i++)
      {
        if (i >= originalLen)
        {
          paddedMessage[i] = 0;
        }
        else
        {
          paddedMessage[i] = message[i];
        }
      }

      unsigned char *encryptedMessage = new unsigned char[paddedMessageLen];

      unsigned char expandedKey[176];

      KeyExpansion(key, expandedKey);

      // Encrypt the message
      for (int i = 0; i < paddedMessageLen; i += 16)
      {
        AESEncrypt(paddedMessage + i, expandedKey, encryptedMessage + i);
      }

      // Prints the encrypted message in hex form
      std::cout << "Encrypted message in hex:" << std::endl;
      for (int i = 0; i < paddedMessageLen; i++)
      {
        std::cout << std::hex << (int)encryptedMessage[i];
        std::cout << " ";
        data[i] = encryptedMessage[i];
      }

      std::cout << std::endl;

      startTurnTimer = millis();
      if (manager.sendto(data, datalen, RH_BROADCAST_ADDRESS))
      {
        printf((char *)&data);
        printf("\n");
        printf("size %d\n", datalen);
        printf("Sending broadcast... \n");
        // wait for packet to be sent
        rf95.waitPacketSent();
        printf("waited \n");
        rf95.setModeRx();
        state = 2;
      }
      retryStartTimer = millis();
    }
    /*State 2: Node receives acknowledgement from other node that its broadcast was received
    If the acknowledgement was not received, keep resending the message*/
    else if (state == 2) // recv ack
    {
      if (manager.recvfrom(buf, &buflen, &from))
      {
        int encryptedMessageLen = 32;

        int j = 0;

        unsigned char *_encryptedMessage = new unsigned char[encryptedMessageLen];
        for (int i = 2; i < encryptedMessageLen; i++)
        {
          _encryptedMessage[j] = (unsigned char) buf[i];
          j++;
        }

        unsigned char expandedKeyDecrypt[176];

        KeyExpansion(key, expandedKeyDecrypt);

        unsigned char *decryptedMessage = new unsigned char[encryptedMessageLen];

        // Decrypt the message
        for (int i = 0; i < encryptedMessageLen; i += 16)
        {
          AESDecrypt(_encryptedMessage + i, expandedKeyDecrypt, decryptedMessage + i);
        }

        int decryptMessageLen = 26;

        // Prints the decrypted message in hex form
        std::cout << "Decrypted message in hex:" << std::endl;
        for (int i = 0; i < decryptMessageLen; i++)
        {
          std::cout << std::hex << (int)decryptedMessage[i];
          std::cout << " ";
        }

        std::cout << std::endl;

        // Prints the decrypted message
        std::cout << "Decrypted message:" << std::endl;

        printf("%d", decryptedMessage[2]);
        std::cout << " ";
        printf("%d", decryptedMessage[3]);
        std::cout << " ";
        printf("%d", decryptedMessage[4]);
        std::cout << " ";
        printf("%d", decryptedMessage[5]);
        std::cout << " ";
        printf("%d", decryptedMessage[6]);
        std::cout << " ";

        for (int i = 7; i < decryptMessageLen; i++)
        {
          std::cout << decryptedMessage[i];
        }

        std::cout << std::endl;

        int k = 0;

        for (int i = 2; i < decryptMessageLen; i++)
        {
          buf[i] = decryptedMessage[k];
          k++;
        }

        if ((int)buf[1] == THIS_NODE_ADDRESS) // If acknowledgement was directed towards this node, print it, verify its integrity and store it
        {
          Serial.print("Got acknowledgement from : 0x");
          Serial.print(from);
          Serial.print(": ");
          Serial.println((char *)&buf);

          std::string timeStamp = "";
          char temp[50] = "";
          int len = 2;

          // Checks message integrity by comparing it with the ack you receive
          while (len < 25)
          {
            if (buf[len] == message[len])
            {
              len++;
            }
            else
            {
              break;
            }
          }

          int j = 0;

          for (int i = 7; i <= 25; i++)
          {
            temp[j] = buf[i];
            j++;
          }

          timeStamp = temp;

          // If ack is the same as the message you send save your own data
          if (len == 25)
          {
            fileName = "Node3 Data ";
            packetContent = packetReader(buf, timeStamp);
            fileWriter(path, fileName, packetContent);
          }
          // rf95.waitAvailableTimeout(1000); // wait time available inside of 15s
          state = 13;
        }
      }
      else if (millis() - retryStartTimer >= retrySend && (millis() - startTurnTimer <= turnTimer)) // Has not received ackowledgement and its turn is not over
      {
        state = 6; // retry send
      }
      else if (millis() - startTurnTimer >= turnTimer) ////Has not received ackowledgement and its turn is  over
      {
        state = 4; // send broadcast - tell next node its his turn
        printf("turn over timeout\n");
      }

      send_turn = true;
      wait_timer = millis();
      last_broadcast_received_timer = millis();
    }
    /*State 3: Node sends acknowledgement that a broadcast or a turn broadcast was received*/
    else if (state == 3) // send ack and turn ack
    {
      // If broadcast received was a turn broadcast, send a turn acknowledgement
      if (turn_ack == true)
      {
        uint8_t turn_ackarr[50];
        uint8_t turn_ackarrlen = sizeof(turn_ackarr);
        turn_ackarr[0] = RH_FLAGS_ACK;
        turn_ackarr[1] = from;
        if (manager.sendto(turn_ackarr, turn_ackarrlen, RH_BROADCAST_ADDRESS))
        {
          printf("Sending turn acknowledgement \n");
          // wait for packet to be sentlast_broadcast_received_timer = millis();
          rf95.waitPacketSent();
          printf("waited \n");
          rf95.setModeRx();
          state = 1;
          turn_ack = false;
        }
        startTurnTimer = millis();
      }
      // If broadcast received was a normal broadcast, send a normal acknowledgement
      else
      {
        srand(THIS_NODE_ADDRESS);
        // random delay so not all nodes send an acknowledgement at the same time
        int sleepTime = rand() % 4;
        sleep(sleepTime);
        printf("rand %d \n", sleepTime);
        buf[0] = RH_FLAGS_ACK;
        buf[1] = from;

        // std::cout << "Message to encrypt:" << std::endl;

        // for (int i = 0; i <= 25; i++)
        // {
        //   message[i] = (int) buf[i];
        //   std::cout << message[i];
        // }

        // std::cout << std::endl;

        // // Pad message to 16 bytes
        // int originalLen = 26;

        // int paddedMessageLen = originalLen;

        // if ((paddedMessageLen % 16) != 0)
        // {
        //   paddedMessageLen = (paddedMessageLen / 16 + 1) * 16;
        // }

        // unsigned char *paddedMessage = new unsigned char[paddedMessageLen];
        // for (int i = 0; i < paddedMessageLen; i++)
        // {
        //   if (i >= originalLen)
        //   {
        //     paddedMessage[i] = 0;
        //   }
        //   else
        //   {
        //     paddedMessage[i] = message[i];
        //   }
        // }

        // unsigned char *encryptedMessage = new unsigned char[paddedMessageLen];

        // unsigned char expandedKey[176];

        // KeyExpansion(key, expandedKey);

        // // Encrypt the message
        // for (int i = 0; i < paddedMessageLen; i += 16)
        // {
        //   AESEncrypt(paddedMessage + i, expandedKey, encryptedMessage + i);
        // }

        // // Prints the encrypted message in hex form
        // std::cout << "Encrypted message in hex:" << std::endl;
        // for (int i = 0; i < paddedMessageLen; i++)
        // {
        //   std::cout << std::hex << (int)encryptedMessage[i];
        //   std::cout << " ";
        //   buf[i] = encryptedMessage[i];
        // }

        // std::cout << std::endl;

        if (manager.sendto(buf, buflen, RH_BROADCAST_ADDRESS))
        {
          printf("Sending acknowledgement \n");
          // wait for packet to be sent
          rf95.waitPacketSent();
          printf("waited \n");
          rf95.setModeRx();
          state = 4;
        }
      }
      sleep(2);
    }
    /*State 4: Node receives: message that indicates a new node joined the network, join request, turn broadcast, acknowledgement */
    else if (state == 4) // recv
    {
      if (manager.recvfrom(buf, &buflen, &from))
      {
        printf("len %d\n", buflen);
        printf("recvd something\n");
        last_broadcast_received_timer = millis();
        if (buflen <= 30)
        {
          if ((int)buf[0] == RH_FLAGS_JOIN_REQUEST) // Broadcast that indicates a new node joined the network
          {
            printf(" A new node joined the network\n");
            std::map<int, bool>::iterator itr;
            itr = node_status_map.find((int)buf[2]);
            printf("id %d, %d, buf %d\n", itr->first, itr->second, (int)buf[2]);
            if (itr->second == false && itr != node_status_map.end())
            {
              itr->second = true;
              new_node = true;
              new_node_id = (int)buf[2];
            }
          }
          if (buflen <= 13) // Turn Broadcast was received
          {
            printf("Received a turn broadcast\n");
            printf("id %d\n", (int)buf[1]);
            if ((int)buf[1] == THIS_NODE_ADDRESS) // Verify if it is this node's turn
            {
              Serial.print("Got message that it's MY turn: 0x");
              Serial.print(from);
              Serial.print(": ");
              Serial.println((char *)buf);
              rf95.waitAvailableTimeout(1000);
              _from = from;
              state = 3; // send turn msg ack
              turn_ack = true;
            }
          }
          buflen = 50;
        }
        else if ((int)buf[0] == RH_FLAGS_JOIN_REQUEST) // Join Request Broadcast was received
        {
          printf("Got join request \n");
          state = 9;
          _from = from;
          new_node = true;
          new_node_id = _from;
        }
        else if ((int)buf[0] == RH_FLAGS_ACK) // Acknowledgement that is not for this node
        {
          printf("Got acknowledgement, but it's not for me!\n");
        }
        else
        {
          last_broadcast_received_timer = millis();

          int encryptedMessageLen = 32;

          unsigned char *_encryptedMessage = new unsigned char[encryptedMessageLen];
          for (int i = 0; i < encryptedMessageLen; i++)
          {
            _encryptedMessage[i] = (unsigned char) buf[i];
          }

          unsigned char expandedKeyDecrypt[176];

          KeyExpansion(key, expandedKeyDecrypt);

          unsigned char *decryptedMessage = new unsigned char[encryptedMessageLen];

          // Decrypt the message
          for (int i = 0; i < encryptedMessageLen; i += 16)
          {
            AESDecrypt(_encryptedMessage + i, expandedKeyDecrypt, decryptedMessage + i);
          }

          int decryptMessageLen = 26;

          // Prints the decrypted message in hex form
          std::cout << "Decrypted message in hex:" << std::endl;
          for (int i = 0; i < decryptMessageLen; i++)
          {
            std::cout << std::hex << (int)decryptedMessage[i];
            std::cout << " ";
          }

          std::cout << std::endl;

          // Prints the decrypted message
          std::cout << "Decrypted message:" << std::endl;

          // printf("%d", decryptedMessage[0]);
          // std::cout << " ";
          // printf("%d", decryptedMessage[1]);
          // std::cout << " ";
          printf("%d", decryptedMessage[2]);
          std::cout << " ";
          printf("%d", decryptedMessage[3]);
          std::cout << " ";
          printf("%d", decryptedMessage[4]);
          std::cout << " ";
          printf("%d", decryptedMessage[5]);
          std::cout << " ";
          printf("%d", decryptedMessage[6]);
          std::cout << " ";

          for (int i = 7; i < decryptMessageLen; i++)
          {
            std::cout << decryptedMessage[i];
          }

          std::cout << std::endl;

          for (int i = 0; i < decryptMessageLen; i++)
          {
            buf[i] = decryptedMessage[i];
          }

          // timer since last broadcast received
          if ((int)buf[0] == RH_FLAGS_RETRY)
          {
            state = 3;
          }
          rf95.waitAvailableTimeout(1000);

          std::string timeStamp = "";
          char temp[50] = "";

          int j = 0;

          for (int i = 7; i <= 25; i++)
          {
            temp[j] = buf[i];
            j++;
          }

          timeStamp = temp;

          packetContent = packetReader(buf, timeStamp);

          // Creates the name from the file according to the id of the node that send the packet
          if ((int)from == NODE1_ADDRESS)
          {
            fileName = "Node1 Data ";
          }
          else if ((int)from == NODE2_ADDRESS)
          {
            fileName = "Node2 Data ";
          }
          else if ((int)from == THIS_NODE_ADDRESS)
          {
            fileName = "Node3 Data ";
          }
          else if ((int)from == NODE4_ADDRESS)
          {
            fileName = "Node4 Data ";
          }
          else if ((int)from == NODE5_ADDRESS)
          {
            fileName = "Node5 Data ";
          }
          else if ((int)from == NODE6_ADDRESS)
          {
            fileName = "Node6 Data ";
          }

          std::ifstream file;

          // Concatenates the the path, fileName, timestamp and the type together into the variable fileDirectory.
          std::string fileDirectory = path + fileName + timeStamp + ".csv";

          // Creates the nodes data file. If the file exist it will open it.
          file.open(fileDirectory);

          // If the file was able to be open it will write into the file else it will not write into the file.
          // After the file was written it will close the file and chage the variable fileWasCreated to true.
          if (file.is_open())
          {
            file_exists = true;
            file.close();
          }

          if (file_exists || timeStamp == "")
          {
            std::cout << "File already exists" << std::endl;
            file_exists = false;
          }
          else
          {
            Serial.print("Got broadcast from : 0x");
            Serial.print(from);
            Serial.print(": ");
            Serial.println((char *)buf);

            fileWriter(path, fileName, packetContent);

            packet = DNP3PacketGenerator(packetContent);

            // Prints to terminal the content of the DNP3Packet
            std::cout << "DNP3Packet \n";
            std::cout << packet.sync << "\n";
            std::cout << packet.length << "\n";
            std::cout << packet.link_control << "\n";
            std::cout << packet.destination_address << "\n";
            std::cout << packet.source_address << "\n";
            std::cout << packet.crc << "\n";

            std::cout << packet.phase_angle << "\n";
            std::cout << packet.phase_on_each_bus << "\n";
            std::cout << packet.power_flow_on_each_transmission_line << "\n";
            std::cout << packet.substation_load << "\n";
            std::cout << packet.substation_component_status << "\n";

            std::cout << packet.time_stamp << "\n";
          }
        }
      }
      // 30 second timer since last broadcast received, if surpassed check who was last broadcast from, check in map if its your turn after this broadcast
      // go to state 1 and change other node to false, send changed node to other nodes
      // if not turn do nothing and wait for other nodes to fix problem
      else if (millis() - last_broadcast_received_timer >= last_broadcast_received)
      {
        if (prevNode(from, node_status_map))
        {
          state = 1;
          printf("state 1\n");
        }
        last_broadcast_received_timer = millis();
      }
      if (send_turn && (millis() - wait_timer >= 25000))
      {
        send_turn = false;
        printf("state 5\n");
        state = 5;
      }
    }
    /*State 5: Node sends turn broadcast */
    else if (state == 5) // send turn broadcast
    {
      sleep(2);
      uint8_t turn[10];
      uint8_t turnlen = sizeof(turn);
      turn[0] = NSK;
      std::map<int, bool>::iterator itr;
      printf("I will send the turn now\n");
      if ((itr = node_status_map.find(NODE4_ADDRESS))->second == true)
      {
        turn[1] = NODE4_ADDRESS;
        printf("node 3's turn\n");
        if (manager.sendto(turn, turnlen, RH_BROADCAST_ADDRESS))
        {
          printf("sent turn\n");
          state = 12;
          rf95.setModeRx();
        }
      }
      else if ((itr = node_status_map.find(NODE5_ADDRESS))->second == true)
      {
        turn[1] = NODE5_ADDRESS;
        printf("node 4's turn\n");
        if (manager.sendto(turn, turnlen, RH_BROADCAST_ADDRESS))
        {
          printf("sent turn\n");
          state = 12;
          rf95.setModeRx();
        }
      }
      else if ((itr = node_status_map.find(NODE6_ADDRESS))->second == true)
      {
        turn[1] = NODE6_ADDRESS;
        printf("node 5's turn\n");
        if (manager.sendto(turn, turnlen, RH_BROADCAST_ADDRESS))
        {
          printf("sent turn\n");
          state = 12;
          rf95.setModeRx();
        }
      }
      else if ((itr = node_status_map.find(NODE1_ADDRESS))->second == true)
      {
        turn[1] = NODE1_ADDRESS;
        printf("node 6's turn\n");
        if (manager.sendto(turn, turnlen, RH_BROADCAST_ADDRESS))
        {
          printf("sent turn\n");
          state = 12;
          rf95.setModeRx();
        }
      }
      else if ((itr = node_status_map.find(NODE2_ADDRESS))->second == true)
      {
        turn[1] = NODE2_ADDRESS;
        printf("node1 turn\n");
        if (manager.sendto(turn, turnlen, RH_BROADCAST_ADDRESS))
        {
          printf("sent turn\n");
          rf95.setModeRx();
          state = 12;
        }
      }
      else
      {
        printf("There's no one else in the network to send the turn to\n");
        two_nodes = false;
        state = 11; // you are the only node in the network. wait for a join req
        rf95.setModeRx();
      }

      // start retry turn timer
      retry_turn_timer = millis();
      // this node is the master node
      master_node = true;
      printf("tx %d\n", rf95.txGood());
      last_broadcast_received_timer = millis();
    }
    /*State 6: Node retries to send broadcast*/
    else if (state == 6) // retry send
    {
      uint8_t datalen = sizeof(data);
      if (manager.sendto(data, datalen, RH_BROADCAST_ADDRESS))
      {
        printf("Sending retry... \n");
        // wait for packet to be sent
        rf95.waitPacketSent();
        printf("waited \n");
        rf95.setModeRx();
        state = 2;
      }
      retryStartTimer = millis();
    }
    /*State 7: Node sends a join request */
    else if (state == 7) // join-send
    {
      // join request data
      printf("Join request started\n");
      uint8_t join[50];
      uint8_t joinlen = sizeof(join);
      join[0] = RH_FLAGS_JOIN_REQUEST; // Flag that indicates join request
      join[1] = THIS_NODE_ADDRESS;

      /*send a broadcast with a join request message and setting join request flag*/
      manager.sendto(join, joinlen, RH_BROADCAST_ADDRESS);
      rf95.waitPacketSent();
      // change to join-recv state
      state = 8;
      joinResendStartTimer = millis();
      printf("Join request ended\n");
      printf("tx %d \n", rf95.txGood());
    }
    /*State 8: Node receives join request acknowledgement*/
    else if (state == 8) // join-recv-ack
    {
      bool recvd = false;
      // join-recv start time for 10 second timeout
      if (manager.recvfrom(buf, &buflen, &from))
      {
        printf("Join request acknowledgement receive started\n");

        if ((int)buf[0] == RH_FLAGS_JOIN_REQUEST)
        {
          printf("Received join request acknowledgement\n");
          printf((char *)buf);

          std::map<int, bool>::iterator itr;
          for (int i = 1; i <= buflen; i++)
          {
            printf("id %d\n", (int)buf[i]);
            itr = node_status_map.find((int)buf[i]);
            itr->second = true;

            if (buf[i] == '\0')
            {
              itr = node_status_map.find(THIS_NODE_ADDRESS);
              itr->second = true;
              printf("break null\n");
              break;
            }
          }
          rf95.waitAvailableTimeout(2000);
          retry = 0;
          recvd = true;
        }
      }
      // if ive sent join-send more than 7 times create nertwork
      if (retry > 2)
      {
        // change to create new network state
        state = 10;
        retry = 0;
      }
      else if (recvd)
      {
        printf("going to state 4\n");
        state = 4;
        recvd = false;
        last_broadcast_received_timer = millis();
      }
      // wait 10 seconds to receive join request ack
      // if no ack received switch back to join-send state
      else if (millis() - joinResendStartTimer >= joinResendTimer)
      {
        state = 7;
        retry++;
        printf("retry join send\n");
      }
    }
    /*State 9: Node sends join request acknowledgement */
    else if (state == 9) // join-send-ack
    {
      printf("Sending join request acknowdledgement...\n");
      uint8_t data[50];
      std::map<int, bool>::iterator itr;
      int i = 2;
      data[0] = RH_FLAGS_JOIN_REQUEST;
      data[1] = _from;
      printf("%d\n", _from);
      for (itr = node_status_map.begin(); itr != node_status_map.end(); ++itr)
      {
        if (itr->second == true)
        {
          data[i] = itr->first;
          i++;
        }
      }
      Serial.println((char *)buf);
      uint8_t datalen = sizeof(data);
      srand(THIS_NODE_ADDRESS);
      // random delay so not all nodes send an acknowledgement at the same time
      int sleepTime = rand() % 4;
      sleep(sleepTime);
      printf("rand %d \n", sleepTime);
      manager.sendto(data, datalen, RH_BROADCAST_ADDRESS);
      rf95.waitPacketSent(2000);
      printf("waited\n");
      state = 4;
      rf95.setModeRx();
      if (two_nodes)
      {
        state = 5;
        two_nodes = false;
        rf95.setModeTx();
        sleep(5);
      }
      itr = node_status_map.find(_from);
      if (itr != node_status_map.end())
      {
        itr->second = true;
      }
      printf("Sent join request acknowledgement\n");
    }
    /*State 10: Node creates the network */
    else if (state == 10) // create new network
    {
      NSK = random(1000, 9999);
      // go to recv state, wait for a join request
      state = 11;
      std::map<int, bool>::iterator itr;
      itr = node_status_map.find(THIS_NODE_ADDRESS);
      if (itr != node_status_map.end())
      {
        itr->second = true;
      }
      printf("Created network\n");
    }
    /*State 11: Node receives join request*/
    else if (state == 11)
    {
      if (manager.recvfrom(buf, &buflen, &from))
      {
        printf("recv node join req send join ack start\n");
        if ((int)buf[0] == RH_FLAGS_JOIN_REQUEST)
        {
          rf95.waitAvailableTimeout(2000);
          _from = from;
          state = 9;
          two_nodes = true;
        }
        printf("recv node join req send join ack end\n");
      }
    }
    /*State 12: Node receives turn acknowledgement */
    else if (state == 12) // receive turn acknowledgement
    {
      if (manager.recvfrom(buf, &buflen, &from))
      {
        printf("recvd something\n");
        if (master_node && ((int)buf[0] == RH_FLAGS_ACK))
        {
          printf("recv node turn ack start\n");
          rf95.waitAvailableTimeout(2000);
          //_from = from;
          state = 4;
          printf("go to state 4\n");
          master_node = false;
        }
      }
      else if (turn_retry >= 3) // after 3 retries change node to false and send to next node
      {
        bool none = true;
        printf("retry counter\n");
        std::map<int, bool>::iterator itr;
        itr = node_status_map.find(THIS_NODE_ADDRESS);
        while (itr != node_status_map.end())
        {
          itr++;
          std::cout << itr->first << " :: " << itr->second << std::endl;
          printf(" while 2");
          if (itr->second == true)
          {
            itr->second = false;
            none = false;
            break;
          }
        }
        if (none)
        {
          for (itr = node_status_map.begin(); itr != node_status_map.end(); itr++)
          {
            std::cout << itr->first << " :: " << itr->second << std::endl;
            printf(" for 2");
            if (itr->second == true)
            {
              if (itr != node_status_map.find(THIS_NODE_ADDRESS))
              {
                itr->second = false;
              }
              break;
            }
          }
        }
        printf("state 5\n");
        state = 5;
        turn_retry = 0;
      }
      else if (millis() - retry_turn_timer >= retry_turn_timeout) // after x seconds resend turn msg
      {
        // start timer after sending turn
        printf("retry timer %ld\n", retry_turn_timer);
        state = 5;
        turn_retry++;
      }
      // sleep(4);
    }
    /*State 13: Node sends received data*/
    else if (state == 13) // rebroadcast received data
    {
      printf("state 13:I will attempt to rebroadcast my saved data\n");
      sleep(2);
      if (!two_nodes)
      {

        std::cout << "Message to encrypt:" << std::endl;
        
        for (int i = 0; i <= 25; i++)
        {
          message[i] = dupe_buf[i];
          std::cout << message[i];
        }

        std::cout << std::endl;

        // Pad message to 16 bytes
        int originalLen = 26;

        int paddedMessageLen = originalLen;

        if ((paddedMessageLen % 16) != 0)
        {
          paddedMessageLen = (paddedMessageLen / 16 + 1) * 16;
        }

        unsigned char *paddedMessage = new unsigned char[paddedMessageLen];
        for (int i = 0; i < paddedMessageLen; i++)
        {
          if (i >= originalLen)
          {
            paddedMessage[i] = 0;
          }
          else
          {
            paddedMessage[i] = message[i];
          }
        }

        unsigned char *encryptedMessage = new unsigned char[paddedMessageLen];

        unsigned char expandedKey[176];

        KeyExpansion(key, expandedKey);

        // Encrypt the message
        for (int i = 0; i < paddedMessageLen; i += 16)
        {
          AESEncrypt(paddedMessage + i, expandedKey, encryptedMessage + i);
        }

        // Prints the encrypted message in hex form
        std::cout << "Encrypted message in hex:" << std::endl;
        for (int i = 0; i < paddedMessageLen; i++)
        {
          std::cout << std::hex << (int)encryptedMessage[i];
          std::cout << " ";
          dupe_buf[i] = encryptedMessage[i];
        }

        std::cout << std::endl;
        
        if (manager.sendto(dupe_buf, dupe_buflen, RH_BROADCAST_ADDRESS))
        {
          printf("Sending broadcast... \n");
          rf95.waitPacketSent();
          printf("waited \n");
          rf95.setModeRx();
          state = 4;
          printf("state 4\n");
        }
      }
      if (master_node)
      {
        send_turn = true;
        wait_timer = millis();
      }
      if (new_node)
      {
        state = 14;
        printf("state 14\n");
      }
      sleep(2);
      printf("state 13 end: I finished brodcasting my data\n");
      wait_timer = millis();
    }
    /*State 14: Node notifies other nodes that there is a new node in the network */
    else if (state == 14) // sending new node
    {
      printf("state 14 start\n");
      sleep(3);
      uint8_t new_node_arr[20];
      uint8_t new_node_arrlen = sizeof(new_node_arr);
      new_node_arr[0] = RH_FLAGS_JOIN_REQUEST;
      new_node_arr[2] = new_node_id;
      if (manager.sendto(new_node_arr, new_node_arrlen, RH_BROADCAST_ADDRESS))
      {
        printf((char *)&new_node_arr);
        printf("Sending new node... \n");
        rf95.waitPacketSent();
        printf("waited \n");
        rf95.setModeRx();
      }
      new_node = false;
      printf("got to 4\n");
      state = 4;
      printf("state 14 end\n");
      wait_timer = millis();
    }
  }
  printf("\n Test has ended \n");
  gpioTerminate();
  return 0;
}
void sig_handler(int sig)
{
  flag = 1;
}