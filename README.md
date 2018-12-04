# diyBMS
Do it yourself battery management system to Lithium ion battery packs/cells

More discussion
https://community.openenergymonitor.org/t/diy-lithium-battery-balancer-and-monitoring-bms/5594/10

# YouTube

https://www.youtube.com/watch?v=-CusvENqC4I&t=12s


# Problem

A DIY Powerwall is the DIY construction of a pack of battery cells to create an energy store which can be used via inverters to power electrical items in the home. Generally cells are salvaged/second hand, and typically use Lithium 18650 cells.

Lithium batteries need to be kept at the same voltage level across a parallel pack. This is done by balancing each cell in the pack to raise or lower its voltage to match the others.

Existing balancing solutions are available in the market place, but at a relatively high cost compared to the cost of the battery bank, so this project is to design a low-cost, simple featured BMS/balancer.

A large number of people have utilised the commercial BATRIUM BMS system in their powerwall devices.

# My Situtation/Environment

UK based, with existing solar panel installation and “feed in tariff” - this is a grid tied install.
I do not want to change/interfere with the existing installation as I get the maximum FIT rates for the next 20 years!

I may also install additional solar panels to a separate inverter in the future.

Therefore, I am looking at an “AC coupled” solution where excess electric generation is diverted to store in a battery rather than exported to grid. This energy is then released at night (or at peak costs) to reduce cost of importing energy.

I am NOT looking to go off-grid (or islanding) as UK power cuts are a rare event in my area.

# Existing Energy Usage

Through the monitoring of my home electric usage using the emonTX and emonSD solutions I typically use around 9kW/H per day (excluding electric car charging). Apart from odd spikes, load it generally less than 500W for the majority of the day/night.

# The Plan

Install a DIY powerwall with the ability to supply a 1000W load to cover all regular home usage (excluding spikes which the grid will handle).
A grid tied inverter would be required to convert the battery DC into mains level AC.
A controllable charger would be required to charge the battery during the day (or at cheap energy times from the grid). Ideally the charger should be able to match the generation of export from the solar array(s) to prevent import.
It may be possible to combine both charger and inverter into a single unit (so called hybrid units)

For obvious reasons, safety is critical, so care must be taken to ensure components are installed and certified as needed, along with the correct specification for cables, breakers and fuses etc.

Why am I doing this? For fun :slight_smile: I really must get out more!

# Parameters

I’m working to the the following design parameters, which are taken from research into the area (what most others are doing!)

Assumption that second hand 18650 cells are made into “packs” containing tens or hundreds of cells in parallel.
Voltage of 18650 cell between 3.0V (empty) and 4.2V (full)
48V DC battery pack - running 14 packs/cells in series (aka 14S) this gives between 42V and 58.8V DC
Battery expected to output (1000W) so produce up to 25A current (depending on battery voltage)
Charger and inverter would be commercial products (not DIY) to comply with necessary approvals
BMS Design
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

Typical bill of materials for each cell module, costs and part numbers provided from Farnell (UK) so other suppliers may be able to provide better costs or similar products.  The ADUM1250 unit is particularly expensive from Farnell and can be purchased for less than 1 USD if you shop around.  Excluding the ADUM1250 chip the component cost is around £5.50 GBP.

|	Quantity	|	Board Marking	|	Farnel (cost per single unit)	|	Make Model	|	Farnell URL	|
|	---	|	---	|	---	|	---	|	---	|
|	1	|	ATTINY85V-10SU SOIC-8	|	0.887	|	ATTINY85V-10SU	|	http://uk.farnell.com/atmel/attiny85v-10su/mcu-8bit-attiny-10mhz-soic-8/dp/1455166	|
|	1	|	REG710NA-3.3	|	1.20	|	REG710NA-3.3/250	|	http://uk.farnell.com/texas-instruments/reg710na-3-3-250/charge-pump-regulator-buck-boost/dp/1535743	|
|	1	|	ADUM1250ARZ SOIC-8	|	4.51	|	Analog Devices ADUM1250ARZ	|	http://uk.farnell.com/analog-devices/adum1250arz-rl7/digital-isolator-4-ch-1mbps-nsoic/dp/2758723	|
|	1	|	M1 SI2312BDS-T1-E3 MOSFET Transistor, N Channel, 3.9 A, 20 V, 0.025 ohm, 4.5 V, 850 mV	|	0.241	|	VISHAY SI2312BDS-T1-E3	|	http://uk.farnell.com/vishay/si2312bds-t1-e3/mosfet-n-ch-20v-3-9a-sot-23-3/dp/2396085	|
|	1	|	D1 SE30AFG-M3/6A Standard Recovery Diode, 400 V, 3 A, Single, 1.1 V, 1.5 µs, 40 A	|	0.306	|	VISHAY SE30AFG-M3/6A	|	http://uk.farnell.com/vishay/se30afg-m3-6a/rectifier-esd-400v-3a-do-221ac/dp/2313878	|
|	1	|	F1 MC36228 PPTC Resettable Fuse, 1.5A 8V	|	0.08	|	MultiComp MC36228	|	http://uk.farnell.com/multicomp/mc36228/fuse-ptc-reset-smd-8v-1-5a/dp/1861169	|
|	1	|	B57891M0103K000 Thermistor, NTC, 10 kohm, B57891M Series, 3950 K, Through Hole	|	0.571	|	EPCOS B57891M0103K000	|	http://uk.farnell.com/epcos/b57891m0103k000/thermistor-ntc-radial-leaded/dp/2285471?ost=B57891M0103K000&iscrfnonsku=false&ddkey=http%3Aen-GB%2FElement14_United_Kingdom%2Fsearch	|
|	1	|	1x GREEN LED 1206 SMT	|		|		|		|
|	1	|	1x BLUE LED 1206 SMT	|	0.12	|	King Bright KPL-3015QBC-D	|	http://uk.farnell.com/kingbright/kpl-3015qbc-d/led-blue-200mcd-465nm-smd/dp/2426226?ost=KPL-3015QBC-D&iscrfnonsku=false&ddkey=http%3Aen-GB%2FElement14_United_Kingdom%2Fsearch	|
|	2	|	C1/C2 2.2uF Cap 1210 SMT	|		|	X7R	|		|
|	3	|	C3/C4/C5 0.1uF Cap 1210 SMT SMD Multilayer Ceramic Capacitor, 1210 [3225 Metric], 0.1 µF, 100 V, ± 10%, X7R, Y Series FF-CAP	|	0.15	|	KEMET C1210Y104K1RACTU	|	http://uk.farnell.com/kemet/c1210y104k1ractu/capacitor-1210-100nf-100v-x7r/dp/1520317	|
|	2	|	C6 0.22uF Cap 1210 SMT	|		|	X7R	|		|
|	1	|	R1 3.3R 50W Wire Wound Resistor |		|	HS50 3R3 J |		|
|	1	|	R2 47R Resistor 1206 SMT	|		|		|		|
|	1	|	R3 20K Resistor 1206 SMT	|		|		|		|
|	1	|	R4 10K Resistor 1206 SMT	|		|		|		|
|	2	|	R5/R9 47K Resistor 1206 SMT	|		|		|		|
|	1	|	R6 470K Resistor 1206 SMT	|		|		|		|
|	1	|	R7 680K Resistor 1206 SMT	|		|		|		|
|	3	|	R8/R10/R12 2K2 Resistor 1206 SMT	|		|		|		|
|	2	|	J2/J3 JST_PH_S4B-PH-K_04x2.00mm_Angled	|	0.10	|		S4B-PH-K-S (LF)(SN)	|	http://uk.farnell.com/jst-japan-solderless-terminals/s4b-ph-k-s-lf-sn/header-tht-right-angle-2mm-4way/dp/9492488	|
|	1	|	J1 	S2B-PH-K-S (LF)(SN)  	| 0.07		|	JST 	S2B-PH-K-S (LF)(SN)	|		|
