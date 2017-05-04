# iotsaMotorServer - web server to drive a stepper motor

This is a wifi http server to control a steppermotor, which can then be used to move something, under control of a REST interface. It is built using the iotsa framework for ESP8266.

Home page is <https://github.com/cwi-dis/iotsaMotorServer>.
This software is licensed under the [MIT license](LICENSE.txt) by the   CWI DIS group, <http://www.dis.cwi.nl>.

One application that has been built with this server is a device that moves a plant (or anything else) up and down, so you can use the height of the plant to visualise the value of a variable. An example of such a variable would be your current electricity consumption, the temperature (inside or outside), the number of minutes until it starts to rain again, or any other scalar value you can find on the internet.

At the moment minimum and maximum position and conversion rate of values to steps are hard-coded, so you need to modify the code to change these.

## Software requirements

* Arduino IDE, v1.6 or later.
* The iotsa framework, download from <https://github.com/cwi-dis/iotsa>.
* The AccelStepper library by Mike McCauley, download from <http://www.airspayce.com/mikem/arduino/AccelStepper/>.

## Hardware requirements

* a iotsa board. Alternatively another esp8266 board, such as an ESP-12 or ESP-201 can also be used.
* A stepper motor (5-12v) such as the 28BYJ-48 (5 volt 4-input halfstep motor).
* An ULN2003A (darlington driver array).
* A microswitch to detect the minimum position, and optionally a second one to detect maximum position.
* Mechanical hardware to put everything together.

## Electronics

In _extras/fritzing_ you find a Fritzing schematic to allow you to build the server plus the steppermotor driver. As-is the code is meant to interface to a 5-wire steppermotor such as the cheap and popular 28BYJ-48 (5 volt 4-input halfstep motor), but similar motors should work as well. For other motors (fullstep, different wiring) you will need to modify the code.

You don't need Fritzing to build the board, if you have a iotsa board there is a schematic in [extras/fritzing/IotsaMotorServerBoard.pdf](extras/fritzing/IotsaMotorServerBoard.pdf) to show you how to populate the board. The iotsa board can handle 12 volt, so you can use a 12v stepper motor if that is more convenient.

## Mechanics

If you want to build an assembly to pull a plant up and down (or anything else up to a weight of about 500 grams) you can find a 3D-printable design at <http://www.thingiverse.com/thing:2281030>. 

The Thingyverse page has links to the Tinkercad files, you will probably need to tweak the design a little to fit your application.

The page also has assembly intructions.

## Building the software

Near the top you need to define `numMotors` and parameters such as `STEPS_PER_MM`. Most of these will have to be determined using trial and error. `DETECT_LIMIT` governs whether a second microswitch is used (to determine the maximum position, you need at least one microswitch to determine minimum position).

Next you specify GPIO pins for the stepper motor(s) and the microswitch(es).

Now compile, and flash either using an FTDI or (if your esp board supports it) over-the-air.

## Operation

The first time the board boots it creates a Wifi network with a name similar to _config-iotsa1234_.  Connect a device to that network and visit <http://192.168.4.1>. Configure your device name (let's say _motor_, as an example), WiFi name and password, and after reboot the iotsa board should connect to your network and be visible as <http://motor.local>.

There is nothing more to configure in the web interface, all settings are made in the code. This is on purpose: settings like GPIO pins used and number of steps needed to raise the object 1mm depend on the hardware and not changeable on the fly.

When the board is powered on (or reset) it will start pulling in the wire until the microswitch engages, to determine the zero position, and then move back a few millimeter. You will probably need to fiddle the microswitch and the end stop hardware to get this to work consistently.

The server provides a REST-like interface on <http://motor.local/stepper> and <http://motor.local/stepper/0> for reading the current state. Changing positions, however, is done with an _http GET_ request to <http://motor.local/stepper/0?pos=123>, which will set _target_ to _123_ millimeter and start moving there.

If you move to a large negative value the zero-limit microswitch will engage, the motor will stop and the position will be set to zero. This can be used to recalibrate (without rebooting), if needed.

