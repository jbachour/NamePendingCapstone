# Design of a Smart Network System for Distributed and Redundant Data Broadcast and Storage


## Description

This project was developed by Jamiliette Bachour, Ismael Rios and Angel Hernandez, as their final engineering project for the Capstone Course.
The team implemented a LoRa mesh network that is fault-tolerant, secure and reliable, even in the presence of natural disasters.
It is built on top of the [Radio Head packet library](https://www.airspayce.com/mikem/arduino/RadioHead/). The team designed this network to be used with 6 nodes.

## Getting Started

### Dependencies

* Ubuntu
* [pigpio](https://abyz.me.uk/rpi/pigpio/)

### Hardware Used

* Raspberry Pi 4 Model B
* LoRa inAir 9b 915MHz module
* 5bi gain 915MHz omnidirectional antenna

### Executing program

* After cloning the repository, navigate to the following folders:
```
cd RadioHead/examples/raspi/rf95_test
```
* To run the file rf95_test, you must first compile the code.
```
make clean && make
```
* Run the file
```
sudo ./rf95_test
```
## Help

* After making any change remember to make clean and compile the code before running it
```
make clean && make
```

## Authors

Jamiliette Bachour, [@jbachour](https://github.com/jbachour) 
<br>Angel Hernandez, [@AngelHernandezDenis](https://github.com/AngelHernandezDenis)
<br>Ismael Rios, [@ismael-rios1](https://github.com/ismael-rios-1)

## Version History
* 0.1
    * Initial Release


## Acknowledgments
* [pigpio](https://abyz.me.uk/rpi/pigpio/)
* [Radio Head](https://www.airspayce.com/mikem/arduino/RadioHead/)
* [AES-128 encryption](https://github.com/ceceww/aes)
