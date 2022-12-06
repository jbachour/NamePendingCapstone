#include <pigpio.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <map>
#include <string>

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

// // Network of 6 nodes
std::map<int, bool> node_status_map;
#define THIS_NODE_ADDRESS 111
#define NODE2_ADDRESS 222
#define NODE3_ADDRESS 333
#define NODE4_ADDRESS 444
#define NODE5_ADDRESS 555
#define NODE6_ADDRESS 666

// RFM95 Configuration
#define RFM95_FREQUENCY 915.00
#define RFM95_TXPOWER 14

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS_PIN, RFM95_IRQ_PIN);

RHMesh manager(rf95, CLIENT_ADDRESS);

// Flag for Ctrl-C
int flag = 0;

// indicates if it's the node's turn to transmit or not
int state = 7;

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
  node_status_map.insert(std::pair<int, bool>(THIS_NODE_ADDRESS, false));
  node_status_map.insert(std::pair<int, bool>(NODE2_ADDRESS, false));
  node_status_map.insert(std::pair<int, bool>(NODE3_ADDRESS, false));
  node_status_map.insert(std::pair<int, bool>(NODE4_ADDRESS, false));
  node_status_map.insert(std::pair<int, bool>(NODE5_ADDRESS, false));
  node_status_map.insert(std::pair<int, bool>(NODE6_ADDRESS, false));

  // Network Session Key (4 digits)
  uint8_t NSK;
  uint8_t _from;

  /* Placeholder Message  */
  uint8_t data[] = "Hello World!";
  uint8_t buf[50];
  /* End Placeholder Message */

  // timeouts
  unsigned long retrystarttime;
  unsigned long retrysend = 4000;
  unsigned long startturntimer;
  unsigned long turntimer = 15000;
  unsigned long resendtimer = 10000;
  unsigned long starttime;

  int retry = 0;
  bool two_nodes = false;

  while (!flag)
  {

    if (state == 1) // sending
    {
      startturntimer = millis();
      if (manager.sendto(data, sizeof(data), RH_BROADCAST_ADDRESS))
      {
        printf("Sending broadcast... \n");
        // Size of message
        uint8_t len = sizeof(buf);
        uint8_t from;
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
      uint8_t len = sizeof(buf);
      uint8_t from;

      if (manager.recvfrom(buf, &len, &from))
      {
        Serial.print("got ack from : 0x");
        Serial.print(from, HEX);
        Serial.print(": ");
        Serial.println((char *)buf);
        rf95.waitAvailableTimeout(2000); // wait time available inside of 15s
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
      uint8_t from;
      if (manager.sendto(data, sizeof(data), from))
      {

        printf("Sending ack \n");
        // Size of message
        uint8_t len = sizeof(buf);
        uint8_t from;
        // wait for packet to be sent
        rf95.waitPacketSent();
        printf("waited \n");
        rf95.setModeRx();
        state = 4;
      }
    }
    else if (state == 4) // recv
    {
      uint8_t len = sizeof(buf);
      uint8_t from, to, dest, id, flags;

      if (manager.recvfrom(buf, &len, &from, &to, &id, &flags))
      {
        if (to == SERVER_ADDRESS_1)
        {
          Serial.print("got message THAT ITS MY TURN : 0x");
          Serial.print(from, HEX);
          Serial.print(": ");
          Serial.println((char *)buf);
          rf95.waitAvailableTimeout(2000);
          state = 1; // client
          startturntimer = millis();
        }
        // might need to change bc of same flags
        else if (flags == RH_FLAGS_APPLICATION_SPECIFIC)
        {
          state = 9;
          _from = from;
        }
        else
        {
          Serial.print("got broadcast from : 0x");
          Serial.print(from, HEX);
          Serial.print(": ");
          Serial.println((char *)buf);
          // printf("this is to %d", to);
          rf95.waitAvailableTimeout(2000);
          state = 3;
        }
      }
    }
    else if (state == 5) // send turn broadcast
    {
      uint8_t data[7];
      std::map<int, bool>::iterator itr;
      if ((itr = node_status_map.find(NODE2_ADDRESS))->second == true)
      {
        data[7] = 'Node 2';
        if (manager.sendto(data, sizeof(data), itr->first))
          {
            rf95.waitPacketSent();
            Serial.print("sent turn to server 2");
            state = 4;
          }
      }
      else if ((itr = node_status_map.find(NODE3_ADDRESS))->second == true)
      {
        data[7] = 'Node 3';
        if (manager.sendto(data, sizeof(data), itr->first))
          {
            rf95.waitPacketSent();
            Serial.print("sent turn to server 3");
            state = 4;
          }
      }
      else if ((itr = node_status_map.find(NODE4_ADDRESS))->second == true)
      {
        data[7] = 'Node 4';
        if (manager.sendto(data, sizeof(data), itr->first))
          {
            rf95.waitPacketSent();
            Serial.print("sent turn to server 4");
            state = 4;
          }
      }
      else if ((itr = node_status_map.find(NODE5_ADDRESS))->second == true)
      {
        data[7] = 'Node 5';
        if (manager.sendto(data, sizeof(data), itr->first))
          {
            rf95.waitPacketSent();
            Serial.print("sent turn to server 5");
            state = 4;
          }
      }
      else if ((itr = node_status_map.find(NODE6_ADDRESS))->second == true)
      {
        data[7] = 'Node 6';
        if (manager.sendto(data, sizeof(data), itr->first))
          {
            rf95.waitPacketSent();
            Serial.print("sent turn to server 6");
            state = 4;
          }
      }
      else
      {
        state = 11; //you are the only node in the network. wait for a join req
      }
      // for (itr = node_status_map.find(THIS_NODE_ADDRESS); itr != node_status_map.end(); ++itr)
      // {
      //   if (itr->second == true)
      //   {
      //     if (manager.sendto(data, sizeof(data), itr->first))
      //     {
      //       rf95.waitPacketSent();
      //       Serial.print("sent turn to server 1");
      //       state = 4;
      //     }
      //   }
      // }

      // if (manager.sendto(data, sizeof(data), SERVER_ADDRESS_1))
      // {
      //   rf95.waitPacketSent();
      //   Serial.print("sent turn to server 1");
      //   state = 4;
      // }
      retrystarttime = millis();
    }
    else if (state == 6) // retry send
    {
      if (manager.sendto(data, sizeof(data), RH_BROADCAST_ADDRESS))
      {
        printf("Sending retry... \n");
        // Size of message
        uint8_t len = sizeof(buf);
        uint8_t from;
        // wait for packet to be sent
        rf95.waitPacketSent();
        printf("waited \n");
        rf95.setModeRx();
        state = 2;
      }
      retrystarttime = millis();
    }
    else if (state == 7) // join-send
    {
      // join request data
      printf("join send start\n");
      uint8_t join[5];
      join[0] = THIS_NODE_ADDRESS;

      /*send a broadcast with a join request message and setting join request flag*/
      manager.setHeaderFlags(RH_FLAGS_APPLICATION_SPECIFIC);
      manager.sendto(join, sizeof(join), RH_BROADCAST_ADDRESS);
      rf95.waitPacketSent();
      // change to join-recv state
      state = 8;
      starttime = millis();
      printf("join send end\n");
    }
    else if (state == 8) // join-recv-ack
    {
      //printf("join recv ack start\n");
      uint8_t len = sizeof(buf);
      uint8_t from;
      uint8_t dest;
      uint8_t id;
      uint8_t flags;
      bool recvd = false;
      // join-recv start time for 10 second timeout
      if (manager.recvfrom(buf, &len, &from, &dest, &id, &flags))
      {
        uint8_t arr[sizeof(buf) - 1];
        // sacar la info recibida del array
        // network session key, nodes and status of nodes in the network
        // cambiar a recv mode normal esperando mensaje que es mi turno
        if (flags == RH_FLAGS_APPLICATION_SPECIFIC)
        {
          NSK = (int) buf[0];
          // take from buf nodes that are active in network
          for (int i = 1; i < len; i++)
          {
            if (buf[i] == '\0')
            {
              break;
            }
            arr[i - 1] = (int) buf[i];
          }
        }
        std::map<int, bool>::iterator itr;
        for (int i = 0; i >= sizeof(arr); i++)
        {
          itr = node_status_map.find(arr[i]);
          if (itr != node_status_map.end())
          {
            itr->second = true;
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
      else if(recvd)
      {
        state = 4;
        recvd = false;
      }
      // wait 10 seconds to receive join request ack
      // if no ack received switch back to join-send state
      else if (millis() - starttime >= resendtimer)
      {
        state = 7;
        retry++;
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
      // tiene que ser diferente flag para que otros nodos no
      manager.setHeaderFlags(RH_FLAGS_APPLICATION_SPECIFIC);
      manager.sendto(data, sizeof(data), _from);
      rf95.waitPacketSent(1000);
      state = 4;
      if (two_nodes)
      {
        state = 5;
        two_nodes = false;
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
      state = 4;
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
      printf("recv node join req send join ack start\n");
      uint8_t len = sizeof(buf);
      uint8_t from;
      uint8_t dest;
      uint8_t id;
      uint8_t flags;
      if (manager.recvfrom(buf, &len, &from, &dest, &id, &flags))
      {
        if (flags == RH_FLAGS_APPLICATION_SPECIFIC)
        {
        rf95.waitAvailableTimeout(2000);
        state = 9;
        two_nodes = true;
        }
      }
      printf("recv node join req send join ack end\n");
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