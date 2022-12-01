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

//node's 3 states: client, ack, server
bool client_mode = true;
bool myturn = true;
bool server_mode = true;

/*Scheduler code begins here*/

struct timeval last_transmission_time;
struct timeval starttime;
unsigned long TURN_TIMER = 15000;
unsigned long RETRY_DELAY = 4000;
std::map<int, bool> NODE_NETWORK_MAP;
int num_nodes;
int networkSessionKey;

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

/*Scheduler code ends here*/

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
  gettimeofday(&last_transmission_time,NULL);
  gettimeofday(&starttime,NULL);

//join boolean of client to time constraint

if (client_mode){
  //Client mode

    //"TCP"
    Serial.println("Sending to...");
    if (manager.sendto(data, sizeof(data), SERVER_ADDRESS_1))
    {
      printf("Inside send \n");
      //Size of message
      uint8_t len = sizeof(buf);
      uint8_t from;
     
      rf95.waitPacketSent(1000);
      printf("waited\n");
      server_mode=true;
      client_mode=false;
      rf95.setModeRx();
    
    }
    else //Message could not be sent
    {
    Serial.println("Message could not be sent");

    }
}
else if (server_mode){
    //Server mode
    uint8_t len = sizeof(buf);
    uint8_t from;
   
    if(manager.recvfrom(buf, &len, &from))
    {
      Serial.print("got message from : 0x");
      Serial.print(from, HEX);
      Serial.print(": ");
      Serial.println((char*)buf);
      client_mode = true;
      server_mode=false;
      //rf95.setModeTx();
      

      if (difference() >= TURN_TIMER && myturn){ //wait for turn timer to end
      rf95.waitAvailableTimeout(difference() - TURN_TIMER);
      server_mode = true;
      client_mode=false;
      myturn=false;
      }

    }
    else{
      //keep trying to catch ack 
      if(myturn && difference() >= TURN_TIMER ){ //i didnt receive ack, and i still have time
        client_mode = true;
        server_mode=false;
      }
      else{
        if(difference() >= TURN_TIMER){ //i didnt recv. and i stil have time
        server_mode = true;
        client_mode=false;
        }
        else{ // i didnt recv msg and i dont have time
        client_mode = true;
        server_mode = false;
        }
      }
    }
}
else{ //not client, not server, not ack, what is it?

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