# DISTRIBUTED-SYSTEM-FOR-MEASUREMENT-OF-PHYSICAL-PARAMETERS-OF-THE-ENVIRONMENT
Distributed system for collecting and displaying meteorological data to remote users. An overview of numerous tools and approaches used for the realization of the diploma thesis is given. At the sensor or microcontroller level, various libraries in the Arduino IDE environment were used, a dynamic calibration of the SGP30 sensor was performed, as well as dual voltage cycles for the MQ7 sensor, and a routine with interrupts for reading values from the anemometer was implemented. At the level of the local network or microcomputer, the MQTT protocol, SQLite database, Node-RED tool, NGINX server as a reverse proxy, Dataplicity agent were used. The Dataplicity tool as well as the purchased web domain has been used at the Internet level for remote access. The system has been tested for most faults that may occur in continuous system operation, such as continued system operation if a single sensor or subsystem fails, system resume when resetting each subsystem individually or all subsystems, system resumption in case of current local network unavailability or the router reset.

![](Images/Meteopostaja%20kompl%20prikaz.png)

# 1.  Subsystem Arduino Industrial 101



![](Images/fritzing%20ind.png)

Wiring diagram subsystem Arduino Industrial 101 (Fritzing)

![](Images/blok%20ind.png)

Flow diagram of  Arduino industrial 101 subsystem

# 2.  Subsystem Croduino nova

![](Images/blok%20nova.png)

Wiring diagram of Croduino Nova2 subsystem


![](Images/frizing%20nova.png)

Flow diagram of Croduino Nova2 subsystem

