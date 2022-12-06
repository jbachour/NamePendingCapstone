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
std::map<std::string, bool> node_status_map;
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
int state = 1;

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
  node_status_map.insert(pair<int, bool>(THIS_NODE_ADDRESS, false));
  node_status_map.insert(pair<int, bool>(NODE2_ADDRESS, false));
  node_status_map.insert(pair<int, bool>(NODE3_ADDRESS, false));
  node_status_map.insert(pair<int, bool>(NODE4_ADDRESS, false));
  node_status_map.insert(pair<int, bool>(NODE5_ADDRESS, false));
  node_status_map.insert(pair<int, bool>(NODE6_ADDRESS, false));

  // Network Session Key (4 digits)
  uint8_t NSK;

  /* Placeholder Message  */
  uint8_t data[] = "Hello World!";
  uint8_t buf[RH_MESH_MAX_MESSAGE_LEN];
  /* End Placeholder Message */

  // timeouts
  unsigned long retrystarttime;
  unsigned long retrysend = 4000;
  unsigned long startturntimer;
  unsigned long turntimer = 15000;
  unsigned long resendtimer = 10000;

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
        rf95.waitAvailableTimeout(1000); // wait time available inside of 15s
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
      uint8_t from, to;

      if (manager.recvfrom(buf, &len, &from, &to))
      {

        if (to == SERVER_ADDRESS_1)
        {
          Serial.print("got message THAT ITS MY TURN : 0x");
          Serial.print(from, HEX);
          Serial.print(": ");
          Serial.println((char *)buf);
          rf95.waitAvailableTimeout(1000);
          state = 1; // client
          startturntimer = millis();
        }
        else
        {
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
    else if (state == 5) // send broadcast
    {
      uint8_t data[] = "Node 2";
      uint8_t buf[RH_MESH_MAX_MESSAGE_LEN];

      if (manager.sendto(data, sizeof(data), SERVER_ADDRESS_1))
      {
        rf95.waitPacketSent();
        Serial.print("sent turn to server 1");
        state = 4;
      }
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
      uint8_t join[5] = THIS_NODE_ADDRESS;

      /*send a broadcast with a join request message and setting join request flag*/
      rf95.setHeaderFlags(RH_FLAGS_APPLICATION_SPECIFIC);
      manager.sendto(data, sizeof(join), RH_BROADCAST_ADDRESS);
      rf95.waitPacketSent();
      // change to join-recv state
      state = ;
    }
    else if (state == 8) // join-recv-ack
    {
      uint8_t len = sizeof(buf);
      uint8_t from;
      uint8_t dest;
      uint8_t id;
      uint8_t flags;
      // join-recv start time for 10 second timeout
      unsigned long starttime = millis();
      if (manager.recvfrom(buf, &len, &from, &dest, &id, &flags);)
      {
        // sacar la info recibida del array
        // network session key, nodes and status of nodes in the network
        // cambiar a recv mode normal esperando mensaje que es mi turno
        if (flags == RH_FLAGS_APPLICATION_SPECIFIC)
        {
          NSK = buf[0];
          // take from buf nodes that are active in network
          for (int i = 1; i < len; i++)
          {
            if (buf[i] == "\0")
            {
              break;
            }
            arr[i - 1] = buf[i];
          }
          // forma de dividir contenido de array cada 2 cosas
          // verificar el tamano sin nsk para saber cuantos nodos hay

          // loop y cada dos index hacer funcion de verificar map?
        }
        rf95.waitAvailableTimeout(1000);
      }
      // if ive sent join-send more than 5 times create nertwork
      if (retry > 5)
      {
        // change to create new network state
        state = newnet;
      }
      // wait 10 seconds to receive join request ack
      // if no ack received switch back to join-send state
      else if (millis() - starttime >= resendtimer)
      {
        state = 7;
        retry++;
      }
    }
    else if (state == 9) // join-send-ack
    {
      
    }
    else if (state == 10) // create new network
    {
      NSK = random(1000, 9999);
      // go to recv state? send state?
      state = ;
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