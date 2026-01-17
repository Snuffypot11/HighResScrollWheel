# HighResScrollWheel
A high-resolution smooth (step/notch less) scroll wheel project for Windows 11 using a magnetic encoder.

*Gemini AI was used to help write the code including the HID descriptor and to tidy up a lot of my inefficient code as well as to format and add clear comments.*

Project inspired by this similar project from YouTube video https://www.youtube.com/watch?v=FSy9G6bNuKA

* Has only been tested to work in Windows 11
* Includes dynamic speed scaling, high precision at slow speeds and fast scrolling at quicker speeds.
* User tunable variables are at the top of the script. Values provided are what I found personally to be suitable.

This has been built using an RP2040-Zero board from Waveshare and an AS5600 magnetic encoder module.
Code is set to SDA on pin 4 and SCL on pin 5. 

<img width="700" alt="image" src="https://github.com/user-attachments/assets/4146a1fd-b3d1-4af1-8991-057c4f423820" />

<img width="700" alt="image" src="https://github.com/user-attachments/assets/7a0de0f3-702f-43ce-9cb8-fc8c69c757a9" />
