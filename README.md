# DSA Queue Simulator

This project simulates a traffic condition at a crossroads of two roads. It makes use of linear data structure to simulate the vehicles in different lanes.

**NOTE**: This project is native to linux only.

## Build

To build the project, run the command:
```sh
make
```
This should generate two executables named **simulator** and **traffic-generator**.

## Run

To run the application, first of all run the **simulator** using the command:
```sh
./simulator
```
This will start the simulator where empty roads are seen. To generate traffic in the road and simulate the traffic, in a separate terminal, run the command:
```sh
./traffic-generator
```
This will start generating traffic randomly and send the generated traffic to the simulator using sockets.
\
On receiving the randomly generated traffic, the simulator should properly simulate the vehicles and the traffic management.
