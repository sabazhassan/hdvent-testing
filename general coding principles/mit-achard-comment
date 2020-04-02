

Hi,

Many thanks to the team for working on this project. It feels good to see people are working on such a potentially life-saving product in an open-source environment. I find this effort very inspiring, and I would like to share my thoughts, in the hope that they would be useful to make the system better.

Due to the nature of this system, I believe extreme care must be taken to prevent any corruption and common glitches from compromising the operating state of the system. The goal of the following suggestions is to maximize the system reliability and safety. If these suggestions are implemented from the very beginning, they should not be too complex to implement.

Even with all these measures, I think the system will be far simpler and much less reliable than normal medical systems, but hopefully 80% of the required reliability will be achieved with minimal effort and a short development.

1) WATCHDOG TIMER

First, I think it is paramount that any crash of the microcontroller should be detected.
The first step would be to enable the watchdog timer on the microcontroller to detect any hard crash or fault, and automatically reboot the system if that happens.

2) HANDLE UNEXPECTED RESETS

Because there can be a reset at any moment (software glitch, watchdog timer, power spike, …), make sure that the software can handle a reset whatever the current state of the system is. i.e. if someone resets the microcontroller while the system is operating, all parameters must be intact and the system must resume operation transparently.

What I mean here, is that it is OK that the system does nothing at initial power up, but once it is running, it must resume if the software resets at any random moment. There should be a distinction between the state of the system, and the state of the microcontroller (i.e. it is not because the microcontroller is rebooting, that the system is being powered up for the first time). The current state of the system (e.g. RUNNING, STOPPED, …) can be stored in non-volatile memory.

I am a bit concerned when I read sentences as “(no motion at power up)”, this is OK as a functional requirement at a system level, but that does not mean the microcontroller itself must do nothing when it resets, because in that case, if it crashes, it will stop working.

It also means the system must be able to resume operation whatever the current state is (exhaling, inhaling).

3) USE CHECKSUM ON ALL DATA

I believe all parameters should be stored in non-volatile memory (the microcontroller has an EEPROM), with a checksum.

Checksum of the parameters must be verified before being used. After each write, re-read the data to make sure it has been written properly, then re-compute the checksum for all the settings and make sure nothing got corrupted.
This process can be repeated up to 5 times (for example) if it is not successful. If it still fails, assume it is uncorrectable and sound an alarm and use fail-safe defaults.

To prevent the non-volatile memory from wearing out, only store significant changes (e.g. unless an operating parameter is changed, nothing new should be written on the internal memory).

4) COPY-ON-WRITE SCHEME

To prevent corruption if the system if rebooted for any reason during a write operation in the internal EEPROM, there should be two sets of parameters (numbered 0 and 1) and one pointer determining which set is active (the other one is standby). Any new write is first written into the standby bank. Once the standby bank has been proofread, the pointer is updated, and then the system applies the new parameter.
The pointer must be a full EEPROM erase unit, so that it can be atomically erased or programmed.
To know which set to use, the software reads the pointer. If its value is 0, then the active parameter set is parameter set 0, and any other value make it the other set of parameter.

This way, there can be no corruption. If the system is reset during a write of the new parameters, it will simply revert back to the previous parameters which have been left untouched. And it will only change to the new parameters once they have been proofread.

As above, any write must be checked and attempted again up to 5 times if it fails.

5) USER INTERFACE CLARITY AND POSITIVE FEEDBACK

I am very concerned by the current user interface of the product as described in the page currently, and I believe it is unsafe and error prone.

What happens if one potentiometer is broken ? The operator can change the position of the potentiometer, and believe they have changed the parameter, but in fact, they have not, as far as the microcontroller is concerned. And now they have to try to understand what the problem is, loosing valuable time, and debugging the system instead of debugging the patient’s health.

This type of problem has been the source of multiple accidents, in the nuclear field (TMI for example), and also in the medical field (THERAC-25) and has killed people. We don’t want this to happen here.

I would suggest to change the user interface as following :
- Change the I2C LED display to an I2C LCD display, such as a 2×16 display.
- Display ALL operating parameters and current mode, as seen by the microcontroller, on the LCD display at all times, allowing the operator to positively make sure the settings are what they believe it is at any moment
- Instead of using one potentiometer for each parameter, use one optical encoder (example: KY-040 or similar), one SELECT button, and one SET button (which could be the encoder’s integrated pushbutton).

To change a setting :
- The operator enters the setting mode by long-pressing the SET button
- The last changed parameter starts blinking. The operate select the parameter they wish to change using the SELECT button which cycles through all available parameters. The currently selected parameter is blinking.
- The operator changes the value by turning the encoder
- The operator can continue changing values of other parameters by pressing the SELECT button and modifying them
- Once the operator has done all the changes, they press SET for 2 seconds to latch the value (which triggers the storage in memory as described above and apply the parameter).
- The display is then updated with the new values, the system emits a short BEEP, and applies the new parameters
- If the operator does not press SET to latch the new values after 10 seconds, no change is made and the system keeps the current values, and emits a long BEEP.

This way :
- There can be no undue change of the parameters (someone touching a potentiometer inadvertently)
- In case a button no longer works or returns unplausible values, the operator will know immediately
- At any moment, the operator can read the state of the system and make sure it is right and expected (this allow nurses to come and check the state of the systems)

Again, if this is not done, there is a safety risk (ergonomic problem).

To save valuable screen real-estate, use the front panel of the system to write any unit and legend, and only write useful data on the LCD itself.

6) EXTERNAL INPUT VALIDATION

The system should detect if any external sensor goes bad and returns erroneous data. This is not always easy to do but there can be general safeguards.
Example : a pressure sensor goes bad and always return 0.004 or 23.34542, the system must not start pumping too much or too little air. Instead, it should detect that no operating parameter has been changed, and yet the values returned are not plausible and changed significantly. The system could also detect stuck values (example, if we expect the pressure to change at least by a given number during each breath, we should check for this).

There are different ways to accomplish this :
- Store in memory a table with a range of plausible values. If any external input is out of the range, detect it
- Use multiple sensors and cross-check the values
- Every time a new parameter is changed, monitor the result at the sensors and detect any change greater than the expected change.

When an unplausible value is detected, different things can happen depending on the affected sensor and current operating state :
- Stop the system
- Revert back to hardcoded open-loop defaults. This is akin what a car ECU would do when a sensor goes bad, go into limp mode.

Every time a fault like this is detected, the alarm must be sounded.

7) SENSOR LIFESPAN

Care must be taken with the selection of the potentiometers and especially the angle measurement pot due to the high duty cycle. A potentiometer may worn out due to the high duty cycle. Non-contact measurement techniques such as hall-effect sensing would possibly be more reliable, maybe coupled with an optical encoder + an optical limit switch to recalibrate it. If using a potentiometer, the firmware should implement debouncing as the track can age and cause glitches at some points.

8) MONITORING THE SYSTEM and EXTERNAL WATCHDOG

It is not possible to make sure the system is working if it is not being monitored.

I would suggest the system outputs machine readable (CSV) data over the UART, each message starting with a respirator ID stored in EEPROM, software version ID, and message ID, so that the respirators can be later networked and monitored using a PC or similar, showing all sensor inputs and settings and system status. It would also open the possibility of data logging on the PC, this way if there is a problem, it can be audited and debugged.

This also allows for external detection of any crashed, disconnected or generally malfunctioning respirator.

9) REMOTE MONITORING

We cannot expect a nurse/technician/doctor to be next to each patient at any moment. If an alarm sounds, there might be nobody around to react in a timely manner. So there must be a way to remotely monitor the respirators.

There should be :
- An output to the serial port for each alarm condition (CSV like above). Later, if a PC software for monitoring/logging is developped, the alert can be relayed to any system over the network, including pagers, SMS, …
- A normally closed dry contact relay for alarms. This way all ventilators can be daisy changed and if one goes in alarm, it can be reported in a central place. Or an external transmitter can be connected.

10) SANE CODE HANDLING AND AUDITING

All code and design documents should be on a version control system. All version should have a unique release ID and the binary files be archived for debugging purposes. For each version, the same binary file must be used (to prevent different compiler versions/settings from generating different codes for each same source code, making it a nightmare to debug in case of failures in the field). Keep a copy of the compiled code and only distribute that binary, do not allow multiple people to compile the same code and call it the same version (it is OK for development but NOT for production use).

Ideally, an hardware ID should be used, but I don’t see one in the current system. One could be stored in the Arduino EEPROM but it creates logistical issues (it needs to be programmed somewhere). So I would suggest using a 1-wire sensor (DS18B20 or similar) and use its ID as the system ID. This way, every respirator has a globally unique ID.

11) SANE CODING TECHNIQUES

Care should be taken when designing the software architecture. Communication between each software block should be well defined, and coupling between each different part must be avoided (example : the user interface handling code must not directly call code everywhere in the system, instead it should have a well defined interface).

12) SECURITY

If the respirators are connected to a PC for monitoring, they should not be connected using the Arduino USB port as it has the capability to reprogram the Arduino. Instead, a serial USB converter with only the Arduino TX and GND connected should be used. This way, no data can be transmitted from the PC to the Arduino, no parameter can be changed and no reprogramming can be done.

13) OTHER RELIABILITY CONSIDERATIONS

All registers and external actuations should be rewritten in their entirety regularly, because we cannot assume external systems have not been reset or changed (due to glitches) without us knowing. This includes the LCD interface.

These concepts should not be too hard to implement and should greatly enhance the reliability and the safety of the system. Hope this helps !
