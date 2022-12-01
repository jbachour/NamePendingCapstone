#include <pigpio.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <map>

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
bool myturn = true; 

struct timeval last_transmission_time;
struct timeval starttime;
unsigned long TURN_TIMER = 15000;
unsigned long RETRY_DELAY = 6000;
std::map<int, bool> NODE_NETWORK_MAP;
int num_nodes;
int networkSessionKey;

// void scheduler (bool myturn)
// {
// //verify if im available

// //verify if other nodes are available
// myturn = true; 
// }

unsigned long difference() {
  //Declare a variable to store current time
  struct timeval RHCurrentTime;
  //Get current time
  gettimeofday(&RHCurrentTime,NULL);
  //Calculate the difference between our start time and the end time
  unsigned long difference = ((RHCurrentTime.tv_sec - last_transmission_time.tv_sec)*1000);
  difference += ((RHCurrentTime.tv_usec - last_transmission_time.tv_usec)/1000);
  //printf("%ld\n", difference);
  //Return the calculated value
  return difference;
}

unsigned long mymillis() {
  //Declare a variable to store current time
  struct timeval RHCurrentTime;
  //Get current time
  gettimeofday(&RHCurrentTime,NULL);
  //Calculate the difference between our start time and the end time
  unsigned long difference = ((RHCurrentTime.tv_sec - starttime.tv_sec)*1000);
  difference += ((RHCurrentTime.tv_usec - starttime.tv_usec)/1000);
  //printf("%ld\n", difference);
  //Return the calculated value
  return difference;
}

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

//setup
num_nodes = 2;
bool sent = false;


while(!flag)
{
// Serial.println("Sending message");

/*
If it's my turn, I'm transmitting (client mode), else, I'm listening for packets (server mode)
*/
if (myturn){

  //Client mode
  
    //"UDP BROADCAST"

    //"TCP"
    //Serial.println("Sending to...");
    if (!sent)
    {
      gettimeofday(&last_transmission_time,NULL);
      gettimeofday(&starttime,NULL);
      manager.sendto(data, sizeof(data), SERVER_ADDRESS_1);
      sent = true;
      //rf95.setModeIdle();

    }
    //printf("Inside send \n");
          //Size of acknowledgement
          uint8_t len = sizeof(buf);
          uint8_t from;
            // printf("%ld\n", mymillis());
            // printf("%ld\n", difference());
            if(rf95.waitAvailableTimeout(3000)) {
            if(manager.recvfrom(buf,&len, &from ))
            { 
              Serial.print("got message from : 0x");
              Serial.print(from, HEX);
              Serial.print(": ");
              Serial.println((char*)buf);
            }
              //Serial.println("I didnt receive it");
            else if(mymillis() >= RETRY_DELAY) //4 segundos
            {
                printf("4 seconds\n");
                //Serial.println("keep sending it!");
                gettimeofday(&starttime, NULL);
                manager.sendto(data, sizeof(data), SERVER_ADDRESS_1);
                rf95.waitPacketSent(10000);
                //rf95.setModeIdle();
            }
            }
          //wait for turn timer to end
            if (difference() >= TURN_TIMER){
              myturn = false;
              gettimeofday(&last_transmission_time,NULL);
              //rf95.setModeIdle();
              printf("switching to recv\n");
            }
    
    // if (manager.recvfrom(buf, &len, &from)){
    //   //Display acknowledgement message and address of sender
    //   Serial.print("got reply from : 0x");
    //   Serial.print(from, HEX);
    //   Serial.print(": ");
    //   Serial.println((char*)buf);
    //   myturn=false;
    // }
    // }
    //  else {
    //   Serial.println("No reply - Acknowledgement failed");
    //    }

}
else {
    //Server mode
    if (sent){
      gettimeofday(&last_transmission_time,NULL);
      Serial.println("im now a recv");
      sent = false;
    }
    //Size of message being received
    uint8_t len = sizeof(buf);
    uint8_t from;
    // if (manager.recvfrom(buf, &len, &from))
   //if(manager.available()){
    //Serial.println("im available");
    if(rf95.waitAvailableTimeout(3000)) {
    if(manager.recvfrom(buf, &len, &from))
    {
      Serial.print("got message from : 0x");
      Serial.print(from, HEX);
      Serial.print(": ");
      Serial.println((char*)buf);
      manager.sendto(data, sizeof(data), SERVER_ADDRESS_1);
      printf("sent ack\n");
      rf95.waitPacketSent(10000);
      //rf95.setModeIdle();

       //Store data here
      // Send a reply back to the originator client
      // uint8_t data[] = "My acknowledgement";
      // if (manager.sendto(data, sizeof(data), from)){
      //   Serial.println("sendt");
      //   rf95.waitPacketSent(5000);
      // }
      // else{
      //   Serial.println("sendtoWait failed");
      // }
     
      //rf95.setModeTx();
//rf95.waitAvailableTimeout(5000);
    }
    }
    if (difference() >= TURN_TIMER * num_nodes) {
      myturn = true;
    }
  // }
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
