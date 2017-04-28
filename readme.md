# StepperMotor Server

This is a wifi http server to control a steppermotor, which can then be used to move something, under control of a REST interface. It is built using the iotsa framework for ESP8266.

One application that has been built with this server is a device that moves a plant (or anything else) up and down, so you can use the height of the plant to visualise the value of a variable. An example of such a variable would be your current electricity consumption, the temperature (inside or outside), the number of minutes until it starts to rain again, or any other scalar value you can find on the internet.

At the moment minimum and maximum position and conversion rate of values to steps are hard-coded, so you need to modify the code to change these.

## Electronics

In extras/fritzing you find a Fritzing schematic to allow you to build the server plus the steppermotor driver. As-is the code is meant to interface to a 5-wire steppermotor such as the cheap and popular 28BYJ-48 (5 volt 4-input halfstep motor), but similar motors should work as well. For other motors (fullstep, different wiring) you will need to modify the code.

You don't need Fritzing to build the board, if you have a iotsa board there is a schematic in [extras/fritzing/IotsaMotorServerBoard.pdf]() to show you how to populate the board.

## Mechanics

If you want to build an assembly to pull a plant up and down (or anything else up to a weight of about 500 grams) you can find a 3d-printable design at [https://tinkercad.com/things/bGmsS4nAfCj](). A copy of that design, showing you how to put it together in stead of laid out ready for 3d printing, can be found at [https://tinkercad.com/things/4QuS2dBOHn3]().