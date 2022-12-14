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

// Indicates the start state of the node.
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
  node_status_map.insert(std::pair<int, bool>(NODE1_ADDRESS, false));
  node_status_map.insert(std::pair<int, bool>(THIS_NODE_ADDRESS, false));
  node_status_map.insert(std::pair<int, bool>(NODE3_ADDRESS, false));
  node_status_map.insert(std::pair<int, bool>(NODE4_ADDRESS, false));
  node_status_map.insert(std::pair<int, bool>(NODE5_ADDRESS, false));
  node_status_map.insert(std::pair<int, bool>(NODE6_ADDRESS, false));



  /* Placeholder Message  */
  uint8_t data[50] = "hello there";
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
  unsigned long retryTurn = 10000;
  unsigned long retryTurnTimer;
  

  /* timeouts end */

  // Keep track of how many times the node has resent the join request.
  int retry = 0;
  // Edge case where there are only 2 nodes in the network.
  bool two_nodes = false;

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
      startTurnTimer = millis();
      if (manager.sendto(data, datalen, RH_BROADCAST_ADDRESS))
      {
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
        Serial.println((char *)buf);
         rf95.waitAvailableTimeout(1000); // wait time available inside of 15s
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
      // uint8_t from;
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
      uint8_t id, flags;

      if (manager.recvfrom(buf, &buflen, &from, &to, &id, &flags))
      {
        printf("len %d\n", buflen);
        printf("recvd something\n");
        if (buflen <= 40)
        {
          printf("recv a turn msg\n");
          printf("id %d\n", (int) buf[1]);
          if ((int)buf[1] == THIS_NODE_ADDRESS)
          {
            //rf95.waitAvailableTimeout(1000);
            Serial.print("got message THAT ITS MY TURN : 0x");
            Serial.print(from, HEX);
            Serial.print(": ");
            Serial.println((char *)buf);
             rf95.waitAvailableTimeout(1000);
            state = 1; // client
            startTurnTimer = millis();
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
        }
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
          state = 4;
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
          state = 4;
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
          state = 4;
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
          state = 4;
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
          state = 4;
        }
      }
      else
      {
        printf("im alone\n");
        two_nodes = false;
        state = 11; // you are the only node in the network. wait for a join req
        rf95.setModeRx();
      }
      // dunno what this is. can it be deleted?????
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
      retryStartTimer = millis();
    }
    else if (state == 6) // retry send
    {
      if (manager.sendto(data, sizeof(data), RH_BROADCAST_ADDRESS))
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
      join[0] = THIS_NODE_ADDRESS;

      /*send a broadcast with a join request message and setting join request flag*/
      printf("flag %d\n", manager.headerFlags());
      manager.setHeaderFlags(RH_FLAGS_JOIN_REQUEST, RH_FLAGS_APPLICATION_SPECIFIC);
      printf("flag %d\n", manager.headerFlags());
      manager.sendto(join, joinlen, RH_BROADCAST_ADDRESS);
      rf95.waitPacketSent();
      // change to join-recv state
      state = 8;
      joinResendStartTimer = millis();
      printf("join send end\n");
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
        //uint8_t arr[50];
        // sacar la info recibida del array
        // network session key, nodes and status of nodes in the network
        // cambiar a recv mode normal esperando mensaje que es mi turno
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
          printf("id %d\n", (int) buf[i]);
          itr = node_status_map.find((int) buf[i]);
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
      //recv from 5, go to 4 or 5 if 5 after timeout of no broadcast
      uint8_t id;
      uint8_t flags;
      if (manager.recvfrom(buf, &buflen, &from, &to, &id, &flags))
      {
        printf("recv node turn ack start\n");
        if (flags == RH_FLAGS_ACK)
        {
          rf95.waitAvailableTimeout(2000);
          _from = from;
          state = 4;
          two_nodes = true;
          printf("flag %d\n", manager.headerFlags());
          manager.setHeaderFlags(RH_FLAGS_NONE, RH_FLAGS_APPLICATION_SPECIFIC);
          printf("flag %d\n", manager.headerFlags());
        }
        printf("recv node turn ack end\n");
      }
      else if () // after 3 retries change node to false and send to next node
      {

      }
      else if () // after x seconds resend turn msg
      {
        // start timer after sending turn
      }
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