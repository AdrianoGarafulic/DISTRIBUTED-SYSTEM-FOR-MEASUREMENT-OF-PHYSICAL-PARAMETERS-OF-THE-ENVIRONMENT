# DISTRIBUTED-SYSTEM-FOR-MEASUREMENT-OF-PHYSICAL-PARAMETERS-OF-THE-ENVIRONMENT

As part of the thesis, a system was created that gives remote users at http://www.nerezisca-meteo.com/ an insight into
current meteorological data as well as historical data display. The parameters to be measured are temperature, humidity, air
pressure, solar radiation, wind speed and direction, carbon dioxide, carbon monoxide and air quality. The system collects
data from sensors using two Arduino microcontrollers and sends them over the local TCP / IP network using the MQTT
protocol to the Raspberry PI on which uses the Node-RED framework to store data in a SQLite database and display
meteorological data locally on the system application interface. Using the NGINX web server and Dataplicity service, the
purchased web domain was redirected to the system application interface.

![](Images/Meteopostaja%20kompl%20prikaz.png)

The figure shows the complete distributed system. It is divided into three subsystems. The Croduino Nova2 subsystem with the associated sensors is shown in brown, and the Arduino Industrial 101 subsystem with the associated sensors is shown in green. In the local network, they communicate with Raspbery PI through MQTT broker. The Raspberry PI subsystem is highlighted in red, with locally installed tools and services for storing and displaying data locally, and services for port forwarding to the Internet. The part that takes place on the Internet, outside the local network, is marked in blue, in order to give the remote user access to the Node-RED application of this system via the web domain.

![](Images/trenutna%20mjerenja.png)
![](Images/podaci%20iz%20prošlosti.png)

# 1.  Subsystem Arduino Industrial 101



![](Images/fritzing%20ind.png)

Wiring diagram subsystem Arduino Industrial 101 (Fritzing)

![](Images/blok%20ind.png)

Flow diagram of  Arduino industrial 101 subsystem

![](Images/WhatsApp%20Image%202020-09-21%20at%2015.54.08%20(1).jpeg)

# 2.  Subsystem Croduino nova

![](Images/frizing%20nova.png)

Wiring diagram of Croduino Nova2 subsystem

![](Images/blok%20nova.png)

Flow diagram of Croduino Nova2 subsystem

![](Images/WhatsApp%20Image%202020-09-21%20at%2015.54.07%20(1).jpeg)

# 3.  Subsystem Raspberry PI

![](Images/podsustav%20raspberry%20PI.png)

The third subsystem consists of a Raspbery PI microcomputer with SQLite database,NGINX web server, eclipse MQTT broker and a Node-RED framework.
NGINX web server is used to reverse the proxy redirect port that displays the Node-RED application user interface to port 80 that supports the Dataplicity service. The user interface of the Node-RED application is redirected to the Dataplicity web service via a Dataplicity agent installed locally on the raspberry PI.


![](Images/blok%20rasp.png)

