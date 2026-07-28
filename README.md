# Ripple Glasses

Code for ripple glasses electronics (microcontroller and embedded system)

## Setup with PlatformIO

1. Download VS Code and enable the WSL distro. Follow [these](https://code.visualstudio.com/docs/remote/wsl) instructions for more information.
2. Add the extension Remote Development, which will allow you to connect to WSL on your system. 
3. Download python using the following:
```
sudo apt update
sudo apt install python3 python3-pip python3-venv
```
4. Add the Platform IO extension. [These](https://docs.platformio.org/en/latest/integration/ide/vscode.html#quick-start) instructions can give you more detail if needed.
5. Navigate to the revision you would like to update/explore on your terminal
6. Execute the command `code {folder to open}`. If the revision does not exist yet, navigate to Platform IO home and add your project by Creating New Project. Make sure you add it to the github repository in `~/crocker_fellowship/platformio`
7. Link the device to your project. Since WSL2 can't access the device scanner, we must manually link it. Run the following commands in windows PowerShell as administrato:
	- `winget install usbipd`
	- `usbipd list`
	- Find your board on the list (should be something like "Silicon Labs CP210x USB to UART Bridge") and note its BUSID
	- Bind the board `usbipd bind --busid <BUSID>`
	- Attach it to WSL `usbipd attach --wsl --busid <BUSID>`
	- Verify that it exists in WSL (move to a WSL terminal) `ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null`
8. Ensure that the baud rate in the `src/` folder matches the `platformio.ini` file. If unsure, explicitly define it in `platformio.ini` to match what is in `src/`:
```
monitor_speed = {BAUD_RATE}
```
8. Try uploading it. Everything should work now! After uploading, you can access the serial monitor on the PlatformIO terminal "Monitor" tab.
9. If you have issues with PSRAM allocation, follow this:
	- In `platformio.ini`, add 
```
board_build.arduino.memory_type = qio_qspi
build_flags =
    -DBOARD_HAS_PSRAM
```
	- Run `rm -rf .pio`
	- Run `pio run --target clean`
	- Run `pio run`
	- PSRAM should allocate correctly now
10. If you have issues running `pio` commands from the VS terminal, follow this:
	- Run `find ~/.platformio -iname "pio" 2>/dev/null`
	- Take the output from that command and use it here: 
```
echo 'export PATH=$PATH:{path you found minus last pio folder}' >> ~/.bashrc
source ~/.bashrc
which pio
pio --version
```
	- You should now be able to use `pio` commands in the terminal
## Changelog

- 1/13/2026 Arduino MKR WiFi 1010 was received
- 1/14/2026 Arduino MKR WiFi 1010 was configured
- 1/15/2026 Arduino MKR WiFi 1010 is able to sent digital pulses to L>
- 1/16/2026 Arduino MKR WiFi 1010 connected successfully to the inter>
- 1/17/2026 Arduino MKR WiFi 1010 is battery-powered and disconnected>
- 1/20/2026 Arduino MKR WiFi 1010 successfully outputted voltage to v>
- 1/21/2026 Implemented button logic to light up an LED
- 1/23/2026 Attached Thing to ESP32S3
- 1/27/2026 ESP32S3 added battery functionality
- 1/27/2026 Put in housing for further testing
- 1/30/2026 Added LED_PIN functionality for led blink
- 2/2/2026 Made Thing unique to prototype (Rev 6)
- 2/13/2026 Added App functionality and global variable
- 2/26/2026 Made Revision 7, making device smaller with smaller batte>
- 3/4/2026 Made Revision 8, connecting device to Firebase
- 3/16/2026 Made Revision 9, connecting device to App and creating ap>
- 3/18/2026 Finished defining finite states for app and microcontroll>
- 3/20/2026 Shifted to new prototype (Ripple Glasses)
- 4/8/2026 Completed Rev 1.3, able to capture sound bites 5 seconds i>
- 7/28/2026 Migrated to platformIO and updated git repo

