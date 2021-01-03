<h1 align="center">jw-agua</h1>
<p align="center">
<img src="https://img.shields.io/github/last-commit/jwieland2/jw-agua">
<img src="https://img.shields.io/badge/-Raspberry%20Pi-C51A4A?style=for-the-badge&logo=Raspberry-Pi">
<img src="https://img.shields.io/badge/made%20by-jwieland2-blue.svg">
</p>

<h2>Usage</h2>
1. Run init.c on boot
2. Schedule daily.c via crontab

<h2>Details</h2>
Home automation via headless Raspberry Pi. In a house with a well on the property, this system will one day hopefully save all the plants from dying when left unattended over the course of a mediterranean summer. An ultrasonic sensor measures the water level of a cistern and a 2-channel relay controls the well pump and the 24V-supply of a magnetic valve that leads to an irrigation system.

Progress is slow unfortunately, because my time to tinker with it on site is very limited.

<h2>ToDo</h2>
1. Add schematic of the whole system
2. Add module specs of sensor and relay
3. Make it prettier and move hardcoded values to preprocessor or args
4. Remove delay() commands. Leaving the program running over extended periods of time makes it prone to error. It should be way safer to store the values temporarily on disk, shut down the program and execute it anew for the next measurement cycle.
5. Attach a modem and add remote control for when Spain finally decides to fix their cellphone coverage.
