# diyBMS
Do it yourself battery management system to Lithium ion battery packs/cells

More discussion
https://community.openenergymonitor.org/t/diy-lithium-battery-balancer-and-monitoring-bms/5594/10



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
