The project is developed using the ESP-IDF framework and focuses on modular firmware architecture and communication with external sensors through the I²C protocol.

# Copyright and Credits

Copyright © 2026 Samuel. All rights reserved.

Developed as an academic project at the **Instituto Federal do Sudeste de Minas Gerais (IF Sudeste MG) — Campus Juiz de Fora**, Brazil.

This project was developed for educational and academic purposes. All rights to the original work are reserved by its author, subject to applicable institutional policies and the licenses of third-party components.

The use, reproduction, modification, or distribution of this project's original materials requires prior authorization from the copyright holder, except where permitted by applicable law.

## Project status: Under development

# Features

ESP32 firmware development using ESP-IDF.
Modular software architecture.
Sensor initialization and data acquisition.
I²C multiplexer integration.
Sensor data processing.
Data transmission and logging.

(Update this list as features are implemented and tested.)

## Hardware Requirements

| Component                 | Description                               |
| ------------------------- | ----------------------------------------- |
| ESP32                     | Main microcontroller                      |
| GY-87 MPU6050 HMC5883L    | Inertial and magnetic sensor module       |
| GY-520 VL53L0X            | ToF sensor Module                         |
| GY-31 Tcs230              | RGB Sensor Module                         |
| TCA9548A                  | I²C multiplexer                           |
| TCA9548A                  | I²C multiplexer                           |

### I²C Multiplexer Channel Configuration

|  Channel  | Description              | Connected Device |
| --------- | ------------------------ | ---------------- |
| Channel 0 | Reserved / To be defined | TBD              |
| Channel 1 | Reserved / To be defined | TBD              |
| Channel 2 | Reserved / To be defined | TBD              |
| Channel 3 | Reserved / To be defined | TBD              |
| Channel 4 | Reserved / To be defined | TBD              |
| Channel 5 | Reserved / To be defined | TBD              |
| Channel 6 | Reserved / To be defined | TBD              |
| Channel 7 | Reserved / To be defined | TBD              |

### Hardware Connections

The system uses the I²C communication protocol to interface with the sensors and peripheral devices.
The hardware configuration, including GPIO assignments, I²C addresses, and multiplexer channel assignments, is documented below.

### ESP32 Pin Assignments

| Signal | ESP32 Pin | Description             |
| ------ | --------- | ----------------------- |
| SDA    |    TBD    | I²C serial data line    |
| SCL    |    TBD    | I²C serial clock line   |
| VCC    |    TBD    | Power supply connection |
| GND    |    TBD    | Ground reference        |

### I²C Device Configuration

| Device   | I²C Address | MUX Channel | Description                         |
| -------- | ----------- | ----------- | ----------------------------------- |
| TCA9548A |     TBD     |      —      | I²C multiplexer                     |
| GY-87    |     TBD     |     TBD     | Inertial and magnetic sensor module |
| MPU6050  |     TBD     |     TBD     | Accelerometer and gyroscope         |
| HMC5883L |     TBD     |     TBD     | Magnetometer                        |

**Note:** Pin assignments, I²C addresses, and multiplexer channels must be updated according to the actual hardware configuration. Devices connected behind the TCA9548A may use the same I²C address if they are on different channels.



## Software Requirements

ESP-IDF — Embedded development framework.
PlatformIO — Development environment and build system.
Git — Version control.
Project Structure
STData/
├── include/                    # Header files
├── src/                        # Source files
|   ├── components/             # Hardware components
|   |   ├── gy87.cpp            # Inertial module
|   |   ├── TCA9548A.cpp        # i2c Mux module
|   |   └── 
|   |
|   ├── 12cManage.cpp           # i2c protocol initialization and handling
|   └── main.c                  # Main Reading algorithm
|
├── test/             # Tests
├── platformio.ini    # Build configuration
└── README.md         # Project documentation



# Main Modules

| Classes                    | Responsibility                                                          |
| -------------------------- | ----------------------------------------------------------------------- |
| MPU6050                    | Communication with the accelerometer and gyroscope                      |
| HMC5883L                   | Magnetic field measurements                                             |
| GY-87                      | Integration and management of the sensors available on the GY-87 module |
| I²C Multiplexer (TCA9548A) | Communication with devices connected to separate I²C channels           |



# Getting Started

1. Clone the Repository

    git clone "https://github.com/SamuelFeMR/STData"
    cd STData

2. Configure the Development Environment

    Install ESP-IDF and configure PlatformIO to use the ESP-IDF framework.
    Ensure that the selected environment matches your ESP32 board.

3. Build the Project

    Compile the firmware using the PlatformIO build command:
    pio run

4. Upload the Firmware

    Connect the ESP32 to your computer and upload the firmware:
    pio run --target upload

5. Monitor Serial Output

    To inspect messages and sensor readings, open the serial monitor:

    pio device monitor

    Check the configured serial port and baud rate if the output is not displayed correctly.

# Usage

After uploading the firmware:

Initialize the I²C peripheral.
Initialize the required sensors.
Configure the I²C multiplexer, if applicable.
Acquire sensor measurements.
Process or transmit the collected data.

The exact execution sequence depends on the current firmware implementation.

Development Status

This project is under active development.

Feature	Status
ESP32 development environment	In progress / Verify
I²C communication	In progress / Verify
Sensor drivers	In progress
Multiplexer integration	In progress
Data acquisition	Planned
Data processing	Planned

Update the status of each feature as development progresses.

Future Improvements
Improve sensor data acquisition and validation.
Implement robust error handling.
Document sensor calibration procedures.
Add automated tests where practical.
Improve data processing and transmission.
Contributing

Contributions, suggestions, and bug reports are welcome.

Please document significant changes and test new functionality before submitting modifications.
