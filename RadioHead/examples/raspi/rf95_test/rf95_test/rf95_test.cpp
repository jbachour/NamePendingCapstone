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

// Function Definitions
void sig_handler(int sig);

// Driver for module used
#include <RH_RF95.h>

// Driver for mesh capability
#include <RHMesh.h>

// Max message length
#define RH_MESH_MAX_MESSAGE_LEN 50

#define RH_FLAGS_JOIN_REQUEST 0x1f

// Pins used
#define RFM95_CS_PIN 8
#define RFM95_IRQ_PIN 4

// Mesh network of 6 nodes
#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS_1 2
#define SERVER_ADDRESS_2 3
#define SERVER_ADDRESS_3 4
#define SERVER_ADDRESS_4 5
#define SERVER_ADDRESS_5 6

// Network of 6 nodes
std::map<int, bool> node_status_map;
#define NODE1_ADDRESS 111
#define THIS_NODE_ADDRESS 222
#define NODE3_ADDRESS 333
#define NODE4_ADDRESS 444
#define NODE5_ADDRESS 555
#define NODE6_ADDRESS 666

// RFM95 Configuration
#define RFM95_FREQUENCY 915.00
#define RFM95_TXPOWER 14

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS_PIN, RFM95_IRQ_PIN);

RHMesh manager(rf95, THIS_NODE_ADDRESS);

// Flag for Ctrl-C to end the program.
int flag = 0;

// File writer global variables
std::string path = "/media/node2/node2ssd/Node Data/";
std::string fileName = "";
std::string packetTimeStamp = "packetTimeStamp";
std::string logTimeStamp = "logFileTimeStamp";

// Indicates the start state of the node.
int state = 7;

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

// Create a timestamp using the computer's date and time and returns it as a sting.
// Recieved a string that would tell the fuction which format to use for the timestamp.
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

  return std::string(timeStamp);
}

// Recieved a array containing the data from the LoRa packet.
// It stored the data in a struct simulating the DNP3Packet and returns it.
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

// Recieved the array where the data of the LoRa packet is stored when the message is send and the timestamp of when the packet was created.
// Extracts the data from the packet and returns a string array with the data and the timestamp of the packet
std::array<std::string, 10> packetReader(uint8_t data[], std::string timeStamp)
{
  std::array<std::string, 10> packetContent;

  packetContent[0] = std::to_string(int(data[0]));
  packetContent[1] = std::to_string(int(data[1]));
  packetContent[2] = std::to_string(int(data[2]));
  packetContent[3] = std::to_string(int(data[3]));
  packetContent[4] = std::to_string(int(data[4]));
  packetContent[5] = timeStamp;

  return packetContent;
}

// This function recieve the path to the where the file is gonna be save, the name of the file and a array containing the data of the packetContent.
// It creates a file of type csv and writes a file in csv format of the data from the packet.
// It creates a log file of type txt that contains the date the file was created, the name of the file and its directory path .
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
  printf("This Address= %d\n", CLIENT_ADDRESS);
  rf95.setTxPower(RFM95_TXPOWER, false);
  rf95.setFrequency(RFM95_FREQUENCY);
  rf95.setModemConfig(RH_RF95::Bw125Cr48Sf4096);

  /* End Manager/Driver settings code */

  /*Node map status initialise*/
  node_status_map.insert(std::pair<int, bool>(NODE1_ADDRESS, false));
  node_status_map.insert(std::pair<int, bool>(THIS_NODE_ADDRESS, false));
  node_status_map.insert(std::pair<int, bool>(NODE3_ADDRESS, false));
  node_status_map.insert(std::pair<int, bool>(NODE4_ADDRESS, false));
  node_status_map.insert(std::pair<int, bool>(NODE5_ADDRESS, false));
  node_status_map.insert(std::pair<int, bool>(NODE6_ADDRESS, false));

  // File wruter variables
  std::array<std::string, 10> packetContent;
  DNP3Packet packet;

  /* Placeholder Message  */
  uint8_t data[50] = "";
  uint8_t buf[50];
  /* End Placeholder Message */

  /* timeouts start */
  // retry to send your broadcast
  unsigned long retrySend = 10000;
  unsigned long retryStartTimer;
  // Node turn timer
  unsigned long turnTimer = 45000;
  unsigned long startTurnTimer;
  // Join request resend timer
  unsigned long joinResendTimer = 10000;
  unsigned long joinResendStartTimer;
  // Retry sending turn
  unsigned long retry_turn_timeout = 10000;
  unsigned long retry_turn_timer;

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
  // Network Session Key (4 digits)
  uint8_t NSK;
  // Keep track of who sent the join request
  uint8_t _from;

  uint8_t from, to;
  uint8_t buflen = sizeof(buf);

  while (!flag)
  {

    if (state == 1) // sending
    {
      printf("flag %d\n", manager.headerFlags());
      manager.setHeaderFlags(RH_FLAGS_NONE, RH_FLAGS_APPLICATION_SPECIFIC);
      printf("flag %d", manager.headerFlags());
      uint8_t datalen = sizeof(data);
      std::string timeStamp = "";

      // Providing a seed value
      srand((unsigned)time(NULL));

      // Generates random data simulating the data from the substation
      data[0] = 1 + (rand() % 91);
      data[1] = 1 + (rand() % 101);
      data[2] = 1 + (rand() % 101);
      data[3] = 1 + (rand() % 101);
      data[4] = 0 + (rand() % 2);
      printf("%d ", data[0]);
      printf("%d ", data[1]);
      printf("%d ", data[2]);
      printf("%d ", data[3]);
      printf("%d ", data[4]);

      timeStamp = getCurrentDateTime(packetTimeStamp);

      int j = 0;

      for (int i = 5; i <= 23; i++)
      {
        data[i] = timeStamp[j];
        j++;
      }

      startTurnTimer = millis();
      if (manager.sendto(data, datalen, RH_BROADCAST_ADDRESS))
      {
        printf((char *)&data);
        printf("\n");
        printf("size %d\n", datalen);
        printf("Sending broadcast... \n");
        // Size of message
        // uint8_t len = sizeof(buf);
        // uint8_t from;
        // wait for packet to be sent
        rf95.waitPacketSent();
        printf("waited \n");
        rf95.setModeRx();
        state = 2;
      }
      retryStartTimer = millis();
    }
    else if (state == 2) // recv ack
    {
      // uint8_t len = sizeof(buf);
      // uint8_t from;

      if (manager.recvfrom(buf, &buflen, &from))
      {
        // rf95.waitAvailableTimeout(1000);
        Serial.print("got ack from : 0x");
        Serial.print(from, HEX);
        Serial.print(": ");
        Serial.println((char *)&buf);

        std::string timeStamp = "";
        char temp[50] = "";
        int len = 0;

        // Checks message integrity by comparing it with the ack you receive
        while (len < 23)
        {
          if (buf[len] == data[len])
          {
            len++;
          }
          else
          {
            break;
          }
        }

        int j = 0;

        for (int i = 5; i <= 23; i++)
        {
          temp[j] = buf[i];
          j++;
        }

        timeStamp = temp;

        // If ack is the same as the message you send save your own data
        if (len == 23)
        {
          fileName = "Node2 Data ";
          packetContent = packetReader(buf, timeStamp);
          fileWriter(path, fileName, packetContent);
        }
        // rf95.waitAvailableTimeout(1000); // wait time available inside of 15s
        state = 5;
      }
      else if (millis() - retryStartTimer >= retrySend && (millis() - startTurnTimer <= turnTimer))
      {
        state = 6; // retry send
      }
      else if (millis() - startTurnTimer >= turnTimer)
      {
        state = 5; // send broadcast - tell next node its his turn
        printf("turn over \n");
      }
    }
    else if (state == 3) // send ack
    {
      if (turn_ack == true)
      {
        if (manager.sendto(buf, buflen, from))
        {
          printf("Sending turn ack \n");
          // wait for packet to be sent
          rf95.waitPacketSent();
          printf("waited \n");
          rf95.setModeRx();
          state = 1;
          turn_ack = false;
          sleep(2);
        }
        startTurnTimer = millis();
      }
      // normal ack
      else
      {
        // random delay so not all nodes send an acknowledgement at the same time
        sleep(rand() % 2);
        if (manager.sendto(buf, buflen, from))
        {
          printf("Sending ack \n");
          // wait for packet to be sent
          rf95.waitPacketSent();
          printf("waited \n");
          rf95.setModeRx();
          state = 4;
        }
      }
      sleep(2);
    }
    else if (state == 4) // recv
    {
      // uint8_t len = sizeof(buf);
      uint8_t id, flags;

      if (manager.recvfrom(buf, &buflen, &from, &to, &id, &flags))
      {
        printf("len %d\n", buflen);
        printf("recvd something\n");
        if (buflen <= 25)
        {
          printf("recv a turn msg\n");
          printf("id %d\n", (int)buf[1]);
          if ((int)buf[1] == THIS_NODE_ADDRESS)
          {
            // rf95.waitAvailableTimeout(1000);
            Serial.print("got message THAT ITS MY TURN : 0x");
            Serial.print(from, HEX);
            Serial.print(": ");
            Serial.println((char *)buf);
            rf95.waitAvailableTimeout(1000);
            _from = from;
            state = 3; // send turn msg ack
            turn_ack = true;
          }
          buflen = 50;
        }
        // might need to change bc of same flags
        else if (flags == RH_FLAGS_JOIN_REQUEST)
        {
          printf("got join req\n");
          state = 9;
          _from = from;
          printf("flag %d\n", manager.headerFlags());
          manager.setHeaderFlags(RH_FLAGS_NONE, RH_FLAGS_APPLICATION_SPECIFIC);
          printf("flag %d", manager.headerFlags());
        }
        else
        {
          // rf95.waitAvailableTimeout(1000);
          Serial.print("got broadcast from : 0x");
          Serial.print(from, HEX);
          Serial.print(": ");
          Serial.println((char *)buf);
          // printf("this is to %d", to);
          rf95.waitAvailableTimeout(1000);
          state = 3;

          std::string timeStamp = "";
          char temp[50] = "";

          int j = 0;

          for (int i = 5; i <= 23; i++)
          {
            temp[j] = buf[i];
            j++;
          }

          timeStamp = temp;

          packetContent = packetReader(buf, timeStamp);

          // Creates the name fro the file according to the id of the node that send the packet
          if ((int)from == NODE1_ADDRESS)
          {
            fileName = "Node1 Data ";
          }
          else if ((int)from == THIS_NODE_ADDRESS)
          {
            fileName = "Node2 Data ";
          }
          else if ((int)from == NODE3_ADDRESS)
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
        // else
        // {
        //   printf("you're not supposed to be here!!!\n");
        //   state = 4;
        // }
        printf("from %d\n", from);
        //   for (int i = 0; i <= buflen; i++){
        //     if ((char *) _buf[i] == "/0"){
        //       break;
        //     }
        //     buf[i] = buf_1[i];
        //   }
      }
    }
    else if (state == 5) // send turn broadcast
    {
      sleep(2);
      uint8_t turn[10];
      uint8_t turnlen = sizeof(turn);
      turn[0] = NSK;
      std::map<int, bool>::iterator itr;
      printf("im going to send turn\n");
      if ((itr = node_status_map.find(NODE3_ADDRESS))->second == true)
      {
        turn[1] = NODE3_ADDRESS;
        printf("node3 turn\n");
        if (manager.sendto(turn, turnlen, RH_BROADCAST_ADDRESS))
        {
          printf("sent turn\n");
          state = 12;
          rf95.setModeRx();
        }
      }
      else if ((itr = node_status_map.find(NODE4_ADDRESS))->second == true)
      {
        turn[1] = NODE4_ADDRESS;
        printf("node4 turn\n");
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
        printf("node5 turn\n");
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
        printf("node6 turn\n");
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
        printf("im alone\n");
        two_nodes = false;
        state = 11; // you are the only node in the network. wait for a join req
        rf95.setModeRx();
      }
      // retryStartTimer = millis();

      // start retry turn timer
      retry_turn_timer = millis();
      // this node is the master node
      master_node = true;
    }
    else if (state == 6) // retry send
    {
      uint8_t datalen = sizeof(data);
      if (manager.sendto(data, datalen, RH_BROADCAST_ADDRESS))
      {
        printf("Sending retry... \n");
        // Size of message
        // uint8_t len = sizeof(buf);
        // uint8_t from;
        // wait for packet to be sent
        rf95.waitPacketSent();
        printf("waited \n");
        rf95.setModeRx();
        state = 2;
      }
      retryStartTimer = millis();
    }
    else if (state == 7) // join-send
    {
      // join request data
      printf("join send start\n");
      uint8_t join[50];
      uint8_t joinlen = sizeof(join);
      join[0] = (int) THIS_NODE_ADDRESS;

      /*send a broadcast with a join request message and setting join request flag*/
      printf("flag %d\n", manager.headerFlags());
      manager.setHeaderFlags(RH_FLAGS_JOIN_REQUEST, RH_FLAGS_APPLICATION_SPECIFIC);
      printf("flag %d\n", manager.headerFlags());
      manager.sendto(join, joinlen, RH_BROADCAST_ADDRESS);
      //rf95.waitPacketSent();
      // change to join-recv state
      state = 8;
      joinResendStartTimer = millis();
      printf("join send end\n");
      printf("tx %d \n", rf95.txGood());
    }
    else if (state == 8) // join-recv-ack
    {
      // printf("join recv ack start\n");
      //  uint8_t len = sizeof(buf);
      //  uint8_t from;
      //  uint8_t dest;
      uint8_t id, flags;
      bool recvd = false;
      // join-recv start time for 10 second timeout
      if (manager.recvfrom(buf, &buflen, &from, &to, &id, &flags))
      {
        printf("join recv ack start\n");
        // uint8_t arr[50];
        //  sacar la info recibida del array
        //  network session key, nodes and status of nodes in the network
        //  cambiar a recv mode normal esperando mensaje que es mi turno
        if (flags == RH_FLAGS_JOIN_REQUEST)
        {
          printf("recvd join ack\n");
          printf((char *)buf);
          NSK = (int)buf[0];
          // take from buf nodes that are active in network
          //   for (int i = 1; i < buflen; i++)
          //   {
          //     printf("buf %d\n", (int) buf[i]);
          //     printf("arr %d\n", arr[i - 1]);
          //     if (buf[i] == '\0')
          //     {
          //       printf("break null");
          //       break;
          //     }
          //     arr[i - 1] = (int)buf[i];
          //   }
          printf("flag %d\n", manager.headerFlags());
          manager.setHeaderFlags(RH_FLAGS_NONE, RH_FLAGS_APPLICATION_SPECIFIC);
          printf("flag %d\n", manager.headerFlags());
        }
        std::map<int, bool>::iterator itr;
        for (int i = 1; i <= buflen; i++)
        {
          printf("id %d\n", (int)buf[i]);
          itr = node_status_map.find((int)buf[i]);
          itr->second = true;
          // if (itr != node_status_map.end())
          // {
          //   itr->second = true;
          // }

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
      }
      // wait 10 seconds to receive join request ack
      // if no ack received switch back to join-send state
      else if (millis() - joinResendStartTimer >= joinResendTimer)
      {
        state = 7;
        retry++;
        printf("retry join send\n");
      }
      // printf("join recv ack end\n");
    }
    else if (state == 9) // join-send-ack
    {
      printf("join send ack start\n");
      uint8_t data[50];
      std::map<int, bool>::iterator itr;
      int i = 1;
      data[0] = NSK;
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
      // tiene que ser diferente flag para que otros nodos no
      printf("flag %d\n", manager.headerFlags());
      manager.setHeaderFlags(RH_FLAGS_JOIN_REQUEST, RH_FLAGS_APPLICATION_SPECIFIC);
      printf("flag %d\n", manager.headerFlags());
      manager.sendto(data, datalen, _from);
      rf95.waitPacketSent(2000);
      printf("waited\n");
      state = 4;
      rf95.setModeRx();
      if (two_nodes)
      {
        state = 5;
        two_nodes = false;
        rf95.setModeTx();
        sleep(6);
      }
      itr = node_status_map.find(_from);
      if (itr != node_status_map.end())
      {
        itr->second = true;
      }
      printf("join send ack end\n");
    }
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
      printf("created network\n");
    }
    else if (state == 11) // recv node join req, send join-ack, tell new node its his turn?
    {
      // uint8_t len = sizeof(buf);
      // uint8_t from;
      // uint8_t dest;
      uint8_t id;
      uint8_t flags;
      if (manager.recvfrom(buf, &buflen, &from, &to, &id, &flags))
      {
        printf("recv node join req send join ack start\n");
        if (flags == RH_FLAGS_JOIN_REQUEST)
        {
          rf95.waitAvailableTimeout(2000);
          _from = from;
          state = 9;
          two_nodes = true;
          printf("flag %d\n", manager.headerFlags());
          manager.setHeaderFlags(RH_FLAGS_NONE, RH_FLAGS_APPLICATION_SPECIFIC);
          printf("flag %d\n", manager.headerFlags());
        }
        printf("recv node join req send join ack end\n");
      }
    }
    else if (state == 12) // receive turn acknowledgement
    {
      if (manager.recvfrom(buf, &buflen, &from))
      {
        printf("recvd something\n");
        if (master_node)
        {
          printf("recv node turn ack start\n");
          rf95.waitAvailableTimeout(2000);
          //_from = from;
          state = 4;
          printf("flag %d\n", manager.headerFlags());
          manager.setHeaderFlags(RH_FLAGS_NONE, RH_FLAGS_APPLICATION_SPECIFIC);
          printf("flag %d\n", manager.headerFlags());
          master_node = false;
        }
      }
      else if (turn_retry >= 3) // after 3 retries change node to false and send to next node
      {
        bool none = false;
        printf("retry counter\n");
        std::map<int, bool>::iterator itr;
        itr = node_status_map.find(THIS_NODE_ADDRESS);
        while (itr != node_status_map.end())
        {
          itr++;
          std::cout << itr->first << " :: " << itr->second << std::endl;
          if (itr->second == true)
          {
            if (itr != node_status_map.end())
            {
              itr->second = false;
            }
            none = true;
            break;
          }
        }
        if (none == true)
        {
          for (itr = node_status_map.begin(); itr != node_status_map.find(THIS_NODE_ADDRESS); itr++)
          {
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
      sleep(4);
    }
    else if (state == 13) // rebroadcast received data
    {
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