#include <pigpio.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
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

// RFM95 Configuration
#define RFM95_FREQUENCY 915.00
#define RFM95_TXPOWER 14

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS_PIN, RFM95_IRQ_PIN);

RHMesh manager(rf95, CLIENT_ADDRESS);

// Flag for Ctrl-C
int flag = 0;

std::string path = "/media/node4/node4ssd/Node Data/";
std::string fileName = "";
std::string packetTimeStamp = "packetTimeStamp";
std::string logTimeStamp = "logFileTimeStamp";
//std::string nodeId = "3";

// indicates if it's the node's turn to transmit or not
int state = 1;

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
DNP3Packet DNP3PacketGenerator(std::array<std::string, 6> packetContent)
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
  ;
  packet.power_flow_on_each_transmission_line = packetContent[2];
  packet.substation_load = packetContent[3];
  packet.substation_component_status = packetContent[4];

  // DNP3Packet timestamp
  packet.time_stamp = getCurrentDateTime(packetTimeStamp);

  return packet;
}

// Recieved the array where the data of the LoRa packet is stored when the message is send and the timestamp of when the packet was created.
// Extracts the data from the packet and returns a string array with the data and the timestamp of the packet
std::array<std::string, 6> packetReader(uint8_t data[], std::string timeStamp)
{

  std::array<std::string, 6> packetContent;

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
void fileWriter(std::string path, std::string fileName, std::array<std::string, 6> packetContent)
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

  std::array<std::string, 6> packetContent;
  DNP3Packet packet;

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

  /* Placeholder Message  */
  uint8_t data[50] = "";
  uint8_t datalen = sizeof(data);
  uint8_t buf[RH_MESH_MAX_MESSAGE_LEN];
  //uint8_t buf[] = {0};

  // Providing a seed value
  // srand((unsigned)time(NULL));

  // data[0] = 1 + (rand() % 91);
  // data[1] = 1 + (rand() % 101);
  // data[2] = 1 + (rand() % 101);
  // data[3] = 1 + (rand() % 101);
  // data[4] = 0 + (rand() % 2);
  // printf("%d ", data[0]);
  // printf("%d ", data[1]);
  // printf("%d ", data[2]);
  // printf("%d ", data[3]);
  // printf("%d ", data[4]);

  // timeStamp = getCurrentDateTime(packetTimeStamp);

  // int j = 0;

  // for (int i = 5; i <= 23; i++)
  // {
  //   data[i] = data[i] + timeStamp[j];
  //   j++;
  // }
  // /* End Placeholder Message */

  unsigned long retrystarttime;
  unsigned long retrysend = 10000;
  unsigned long startturntimer;
  unsigned long turntimer = 45000;

  // global var
  uint8_t from, to;
  uint8_t buflen = sizeof(buf);

  while (!flag)
  {

    if (state == 1) // sending
    {
      std::string timeStamp = "";

      // Providing a seed value
      srand((unsigned)time(NULL));

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

      uint8_t datalen = sizeof(data);
      startturntimer = millis();
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
      retrystarttime = millis();
    }
    else if (state == 2) // recv ack
    {
      // uint8_t len = sizeof(buf);
      // uint8_t from;

      if (manager.recvfrom(buf, &buflen, &from))
      {
        rf95.waitAvailableTimeout(1000);
        Serial.print("got ack from : 0x");
        Serial.print(from, HEX);
        Serial.print(": ");
        Serial.println((char *)&buf);

        std::string timeStamp = "";
        char temp[50] = "";
        int len = 0;

        while(len < 23){
          if(buf[len] == data[len]){
            //std::cout << "They are equal \n";
            len++;
          }
        }

        int j = 0;

        for (int i = 5; i <= 23; i++)
        {
          temp[j] = buf[i];
          j++;
        }
        
        timeStamp = temp;

        if(len == 23){
          fileName = "Node1 Data ";
          packetContent = packetReader(buf, timeStamp);
          fileWriter(path, fileName, packetContent);
        }
        // rf95.waitAvailableTimeout(1000); // wait time available inside of 15s
        state = 5;
      }
      else if (millis() - retrystarttime >= retrysend && (millis() - startturntimer <= turntimer))
      {
        state = 6; // retry send
      }
      else if (millis() - startturntimer >= turntimer)
      {
        state = 5; // send broadcast - tell next node its his turn
        printf("turn over \n");
      }
    }
    else if (state == 3) // send ack
    {
      // uint8_t from;
      printf("from %d\n", from);
      buflen = sizeof(buf);
      // sleep(3);
      if (manager.sendto(buf, buflen, from))
      {

        printf("Sending ack \n");
        // Size of message
        // uint8_t len = sizeof(buf);
        // uint8_t from;
        // wait for packet to be sent
        rf95.waitPacketSent();
        printf("waited \n");
        rf95.setModeRx();
        state = 4;
      }
    }
    else if (state == 4) // recv
    {
      // uint8_t len = sizeof(buf);
      // uint8_t buf_1[50];
      // uint8_t buf_1_len = sizeof(buf_1);
      // uint8_t _buf[] = {50};

      if (manager.recvfrom(buf, &buflen, &from, &to))
      {
        // int item_count = 0;
        // for (int i = 0; i <= buflen; i++)
        // {
        //   if ((char *) _buf[i] == "/0")
        //   {
        //     break;
        //   }
        //   item_count++;
        // }

        printf("%d\n", (int)buf[0]);
        printf("len %d\n", buflen);
        if (buflen <= 40)
        {
          if ((int)buf[0] == CLIENT_ADDRESS)
          // if (to == SERVER_ADDRESS_1)
          {
            rf95.waitAvailableTimeout(1000);
            Serial.print("got message THAT ITS MY TURN : 0x");
            Serial.print(from, HEX);
            Serial.print(": ");
            printf("%d", (int)buf[0]);
            printf("\n");
            // rf95.waitAvailableTimeout(1000);
            state = 1; // client
            startturntimer = millis();
          }
          buflen = 50;
        }
        else
        {
          rf95.waitAvailableTimeout(1000);
          Serial.print("got broadcast from : 0x");
          Serial.print(from, HEX);
          Serial.print(": ");
          printf((char *)&buf);
          printf("\n");
          printf("this is to %d\n", to);
          // rf95.waitAvailableTimeout(1000);
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
          if ((int)from == 1)
          {
            fileName = "Node1 Data ";
          }
          else if ((int)from == 2)
          {
            fileName = "Node2 Data ";
          }
          else if ((int)from == 3)
          {
            fileName = "Node3 Data ";
          }
          else if ((int)from == 4)
          {
            fileName = "Node4 Data ";
          }
          else if ((int)from  == 5)
          {
            fileName = "Node5 Data ";
          }
          else if ((int)from  == 6)
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
    else if (state == 5) // send broadcast
    {
      // sleep(3);
      uint8_t turn[10];
      // uint8_t turn[2 + rand() % 40];
      // uint8_t turn[] = {0};
      turn[0] = SERVER_ADDRESS_1;
      uint8_t turnlen = sizeof(turn);
      // uint8_t buf[50];
      if (manager.sendto(turn, turnlen, RH_BROADCAST_ADDRESS))
      {
        printf("tx %d\n", rf95.txGood());
        // rf95.waitPacketSent(2000);
        Serial.print("sent turn to server 1\n");
        rf95.setModeRx();
        state = 4;
      }
      retrystarttime = millis();
    }
    else if (state == 6) // retry send
    {
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
      retrystarttime = millis();
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