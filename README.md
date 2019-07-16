# diyBMS
Do it yourself battery management system to Lithium ion battery packs/cells

More discussion
https://community.openenergymonitor.org/t/diy-lithium-battery-balancer-and-monitoring-bms/5594/10

** PLEASE NOTE THIS DESIGN HAS BEEN REPLACED WITH VERSION 4 **

https://github.com/stuartpittaway/diyBMSv4


# YouTube

https://www.youtube.com/watch?v=-CusvENqC4I&t=12s

Some Youtube videos on the BMS building and testing can be found at
https://www.youtube.com/playlist?list=PLiYbY8lzAN-2qB9rR0Sc7Z7sUrsgYzsiV

# Problem

A DIY Powerwall is the DIY construction of a pack of battery cells to create an energy store which can be used via inverters to power electrical items in the home. Generally cells are salvaged/second hand, and typically use Lithium 18650 cells.

Lithium batteries need to be kept at the same voltage level across a parallel pack. This is done by balancing each cell in the pack to raise or lower its voltage to match the others.

Existing balancing solutions are available in the market place, but at a relatively high cost compared to the cost of the battery bank, so this project is to design a low-cost, simple featured BMS/balancer.

A large number of people have utilised the commercial BATRIUM BMS system in their powerwall devices.

# My Situtation/Environment

UK based, but with only a small setup.  I have a 7s96p setup which i need to keep in balance.

# Existing Energy Usage

Through the monitoring of my home electric usage using the emonTX and emonSD solutions I typically use around 9kW/H per day (excluding electric car charging). Apart from odd spikes, load it generally less than 500W for the majority of the day/night.

# The Plan

Install a DIY powerwall with the ability to supply a 1000W load to cover all regular home usage (excluding spikes which the grid will handle).

A grid tied inverter would be required to convert the battery DC into mains level AC.

A controllable charger would be required to charge the battery during the day (or at cheap energy times from the grid). Ideally the charger should be able to match the generation of export from the solar array(s) to prevent import.

It may be possible to combine both charger and inverter into a single unit (so called hybrid units)

For obvious reasons, safety is critical, so care must be taken to ensure components are installed and certified as needed, along with the correct specification for cables, breakers and fuses etc.

Why am I doing this? For fun :slight_smile: I really must get out more!

# WARNING

Just a reminder that this is a DIY product/solution so don’t use this for safety critical systems or in any situation where there could be a risk to life.  There is no warranty, it may not work as expected or at all.

# Parameters

I’m working to the the following design parameters, which are taken from research into the area (what most others are doing!)

Assumption that second hand 18650 cells are made into “packs” containing tens or hundreds of cells in parallel.
Voltage of 18650 cell between 3.0V (empty) and 4.2V (full)
48V DC battery pack - running 14 packs/cells in series (aka 14S) this gives between 42V and 58.8V DC
Battery expected to output (1000W) so produce up to 25A current (depending on battery voltage)
Charger and inverter would be commercial products (not DIY) to comply with necessary approvals

# BMS Design

Building upon my existing skill set and knowledge, and also building something that others can contribute to using regular standard libraries and off the shelf components.

* Design a hub and spoke arrangement, for a central controller and individual cell monitoring nodes on each pack (so 14 in my case)
* Use Arduino based libraries and tools (possible move to platform.io) **
* Use esp8266-12e as the controller - to take advantage of built in WIFI for monitoring/alerting and CPU/RAM performance
* Use AVR ATTINY85 for each node
* Ensure each cell voltage is isolated from other cells and that ground voltage is isolated
* Isolated communications between controller and node is needed
* Put everything on GITHUB
* Document it!

** There are better more capable CPUs than the 8-bit Arduino however they lack user friendliness and the community to support and nuture the development of code

# Bill Of Materials

NOTE SIZES OF COMPNENTS SHOULD BE 0805 SIZED FOR THE SMALLER PCB VARIANT!

See BMS_Cell_Module_BOM.html for details

Links to main components
ATTINY85V-10SU
http://uk.farnell.com/atmel/attiny85v-10su/mcu-8bit-attiny-10mhz-soic-8/dp/1455166

REG710NA-3.3
http://uk.farnell.com/texas-instruments/reg710na-3-3-250/charge-pump-regulator-buck-boost/dp/1535743

Analog Devices ADUM1250ARZ
http://uk.farnell.com/analog-devices/adum1250arz-rl7/digital-isolator-4-ch-1mbps-nsoic/dp/2758723

SI2312BDS-T1-E3 MOSFET Transistor, N Channel, 3.9 A, 20 V, 0.025 ohm, 4.5 V, 850 mV
http://uk.farnell.com/vishay/si2312bds-t1-e3/mosfet-n-ch-20v-3-9a-sot-23-3/dp/2396085

B57891M0103K000 Thermistor, NTC, 10 kohm, B57891M Series, 3950 K, Through Hole
http://uk.farnell.com/epcos/b57891m0103k000/thermistor-ntc-radial-leaded/dp/2285471?ost=B57891M0103K000&iscrfnonsku=false&ddkey=http%3Aen-GB%2FElement14_United_Kingdom%2Fsearch
