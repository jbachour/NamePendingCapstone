#include <pigpio.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

//Function Definitions
void sig_handler(int sig);

//Driver for module used
#include <RH_RF95.h>

//Driver for mesh capability
#include <RHMesh.h>

//Max message length
#define RH_MESH_MAX_MESSAGE_LEN 50

//Pins used
#define RFM95_CS_PIN 8
#define RFM95_IRQ_PIN 4

//Mesh network of 6 nodes
#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS_1 2
#define SERVER_ADDRESS_2 3
#define SERVER_ADDRESS_3 4
#define SERVER_ADDRESS_4 5
#define SERVER_ADDRESS_5 6

//RFM95 Configuration
#define RFM95_FREQUENCY  915.00
#define RFM95_TXPOWER 14

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS_PIN, RFM95_IRQ_PIN);

RHMesh manager(rf95, CLIENT_ADDRESS);

//Flag for Ctrl-C
int flag = 0;

//indicates if it's the node's turn to transmit or not
bool myturn = false;

// void scheduler (bool myturn)
// {
// //verify if im available

// //verify if other nodes are available
// myturn = true;
// }

//Main Function
int main (int argc, const char* argv[] )
{
if (gpioInitialise()<0) //pigpio library function that initiliazes gpio
  {
    printf( "\n GPIOs could not be initialized" );
    return 1;
  }
   gpioSetSignalFunc(2, sig_handler); //2 is SIGINT. Ctrl+C will cause signal.

//Verify Raspi startup
  printf( "\nRPI rf95_test startup OK.\n" );
  printf( "\nRPI GPIO settings:\n" );
  printf("CS-> GPIO %d\n", (uint8_t) RFM95_CS_PIN);
  printf("IRQ-> GPIO %d\n", (uint8_t) RFM95_IRQ_PIN);

//Verify driver initialization
if (!rf95.init())
{
  printf( "\n\nRF95 Driver Failed to initialize.\n\n" );
  return 1;
}

if (!manager.init())
{
  printf( "\n\nMesh Manager Failed to initialize.\n\n" );
  return 1;
}

/* Begin Manager/Driver settings code */
  printf("\nRFM 95 Settings:\n");
  printf("Frequency= %d MHz\n", (uint16_t) RFM95_FREQUENCY);
  printf("Power= %d\n", (uint8_t) RFM95_TXPOWER);
  printf("This Address= %d\n", CLIENT_ADDRESS);
  rf95.setTxPower(RFM95_TXPOWER, false);
  rf95.setFrequency(RFM95_FREQUENCY);
  rf95.setModemConfig(RH_RF95::Bw125Cr48Sf4096);

/* End Manager/Driver settings code */

/* Placeholder Message  */
uint8_t data[] = "Hello World!";
uint8_t buf[RH_MESH_MAX_MESSAGE_LEN];
/* End Placeholder Message */


while(!flag)
{
/*
If it's my turn, I'm transmitting (client mode), else, I'm listening for packets (server mode)
*/
if (myturn)
{
  //Client mode

  //"TCP"
  Serial.println("Sending to...");
  if(manager.sendto(data, sizeof(data),SERVER_ADDRESS_1)) //message is being sent to address
  {
    printf("Inside send \n");
    
    uint8_t len = sizeof(buf); //Size of message to be sent
    uint8_t from;
    rf95.waitPacketSent(1000);
    printf("waited\n");
    myturn=false; //After sending the message switch to server mode
    rf95.setModeRx(); //sets node's mode to Receiving (server)
  
  }
  else 
  //Message could not be sent
  {
    Serial.println("Send message failed");
  }

}
else 
{
  //Server mode
  uint8_t len = sizeof(buf); //Size of message being received
  uint8_t from;

  /* 
  Check if message being received is a broadcast. 
  If it's broadcast check if it's a duplicate, else broadcast it
  If message is not a broadcast, print it, switch to client, and send a broadcast of the message being received
   */

  if( from == RH_BROADCAST_ADDRESS)
  {
    //Check for duplicate
    //else broadcast
    //Broadcast message received
    manager.sendto(buf, len, RH_BROADCAST_ADDRESS);

  }
  else //Message received is not a broadcast
  {
    if(manager.recvfrom(buf, &len, &from))
    {
      //print message being received
      Serial.print("got message from : 0x");
      Serial.print(from, HEX);
      Serial.print(": ");
      Serial.println((char*)buf);

      //switch to client mode
      myturn = true;
      rf95.waitAvailableTimeout(5000);

      //Broadcast message received
      manager.sendto(buf, len, RH_BROADCAST_ADDRESS);

    }
  }
   
 
}

}
printf( "\n Test has ended \n" );
gpioTerminate();
return 0;

}

void sig_handler(int sig)
{
  flag=1;
}