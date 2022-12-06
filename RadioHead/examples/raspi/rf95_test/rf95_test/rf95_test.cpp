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

// Network of 6 nodes
map<string, bool> node_status_map;
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

RHDatagram manager(rf95, THIS_NODE_ADDRESS);

// Flag for Ctrl-C
int flag = 0;

// indicates if it's the node's turn to transmit or not
bool myturn = true;

// void scheduler (bool myturn)
// {
// //verify if im available

// //verify if other nodes are available
// myturn = true;
// }

void nodeStatus(char arr[4])
{
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
  rf95.setTxPower(RFM95_TXPOWER, false);
  rf95.setFrequency(RFM95_FREQUENCY);
  rf95.setModemConfig(RH_RF95::Bw125Cr48Sf4096);

  /* End Manager/Driver settings code */

  /*node map status initilise*/
  node_status_map.insert(pair<int, bool>(THIS_NODE_ADDRESS, false));
  node_status_map.insert(pair<int, bool>(NODE2_ADDRESS, false));
  node_status_map.insert(pair<int, bool>(NODE3_ADDRESS, false));
  node_status_map.insert(pair<int, bool>(NODE4_ADDRESS, false));
  node_status_map.insert(pair<int, bool>(NODE5_ADDRESS, false));
  node_status_map.insert(pair<int, bool>(NODE6_ADDRESS, false));

  // Network session key (4 digit)
  uint8_t NSK;

  unsigned long resendtimer = 10000;

  /* Placeholder Message  */
  uint8_t data[] = "Hello World!";
  uint8_t buf[RH_MESH_MAX_MESSAGE_LEN];
  /* End Placeholder Message */

  // join-send state = 1 || start
  // join request data
  uint8_t join[5] = THIS_NODE_ADDRESS;

  /*send a broadcast with a join request message*/
  rf95.setHeaderFlags(RH_FLAGS_APPLICATION_SPECIFIC);
  manager.sendto(data, sizeof(join), RH_BROADCAST_ADDRESS);
  // change to join-recv state
  state = ;
  /*end of join-send*/

  /*join-recv-ack state = 2 || only accept things with RH_FLAGS_APPLICATION_SPECIFIC flag*/
  uint8_t len = sizeof(buf);
  uint8_t from;
  uint8_t dest;
  uint8_t id;
  uint8_t flags;
  // join-recv start time for 10 second timeout
  unsigned long starttime = millis();
  manager.recvfrom(buf, &len, &from, &dest, &id, &flags);

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
      arr[i-1] = buf[i];
    }
    // forma de dividir contenido de array cada 2 cosas
    // verificar el tamano sin nsk para saber cuantos nodos hay
    
    // loop y cada dos index hacer funcion de verificar map?


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
    state = 1;
    retry++;
  }
  /*end of join-recv-ack*/

  /*start join-send-ack*/
  // send ack to from

  /*end join-send-ack*/

  /*start join-recv inside of normal recv*/
  // anadir logica para cuando 
  uint8_t len = sizeof(buf);
  uint8_t from;
  uint8_t dest;
  uint8_t id;
  uint8_t flags;

  manager.recvfrom(buf, &len, &from, &dest, &id, &flags);

  // sacar la info recibida del array
  // network session key, nodes and status of nodes in the network
  // cambiar a recv mode normal esperando mensaje que es mi turno
  if (flags == RH_FLAGS_APPLICATION_SPECIFIC)
  {
    // change state to join-send-ack
    state = join-send-ack;
  }
  /*end join-recv*/

  /*start create new network*/
  NSK = random(1000,9999);
  // go to recv state? send state?
  state = ;
  /*end create new network*/

  while (!flag)
  {
    // Serial.println("Sending message");

    /*
    If it's my turn, I'm transmitting (client mode), else, I'm listening for packets (server mode)
    */
    if (myturn)
    {
      // Client mode

      //"UDP BROADCAST"

      //"TCP"
      Serial.println("Sending to...");
      if (manager.sendto(data, sizeof(data), SERVER_ADDRESS_1))
      {

        printf("Inside send \n");
        // Size of acknowledgement
        uint8_t len = sizeof(buf);
        uint8_t from;
        // if(manager.waitAvailableTimeout(5000)){
        // Acknowledgement
        rf95.waitPacketSent(1000);
        printf("waited\n");
        myturn = false;
        rf95.setModeRx();
        // if (manager.recvfrom(buf, &len, &from)){
        //   //Display acknowledgement message and address of sender
        //   Serial.print("got reply from : 0x");
        //   Serial.print(from, HEX);
        //   Serial.print(": ");
        //   Serial.println((char*)buf);
        //   myturn=false;
        //    printf("Switching to server \n");
        // }
        // //  else {
        // //   Serial.println("No reply - Acknowledgement failed");
        // //    }
      }
      else
      // Message could not be sent
      {
        Serial.println("sendto failed");
      }
    }
    else
    {
      // Server mode
      // erial.println("im now a recv");
      // Size of message being received
      uint8_t len = sizeof(buf);
      uint8_t from;
      // if (manager.recvfrom(buf, &len, &from))
      // if(manager.available()){
      //  Serial.println("im available");

      if (manager.recvfrom(buf, &len, &from))
      {
        Serial.print("got message from : 0x");
        Serial.print(from, HEX);
        Serial.print(": ");
        Serial.println((char *)buf);

        // Store data here
        // Send a reply back to the originator client
        // uint8_t data[] = "My acknowledgement";
        // if (manager.sendto(data, sizeof(data), from)){
        //   Serial.println("sendt");
        //   rf95.waitPacketSent(5000);
        // }
        // else{
        //   Serial.println("sendtoWait failed");
        // }
        myturn = true;
        // rf95.setModeTx();
        rf95.waitAvailableTimeout(5000);
      }
      //}
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