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

### I²C Device Configuration

| Device     | I²C Address | MUX Channel | Description                         |
| ---------- | ----------- | ----------- | ----------------------------------- |
| TCA9548A   | `0x70`      | N/A         | I²C multiplexer                     |
| MPU6050    | `0x68`      | N/A         | Accelerometer and gyroscope         |
| HMC5883L   | `0x1E`      | N/A         | Magnetometer                        |
| VL53L0X #1 | `0x29`      | 0           | Time-of-Flight distance sensor      |
| VL53L0X #2 | `0x29`      | 1           | Time-of-Flight distance sensor      |

## Hardware Connections

The system uses an ESP32 microcontroller to communicate with the sensors through an I²C bus. A TCA9548A multiplexer allows multiple VL53L0X distance sensors to share the same bus.

### ESP32 — TCA9548A Multiplexer

| Signal | ESP32 Pin | TCA9548A Pin | Description           |
| ------ | --------- | ------------ | --------------------- |
| SDA    | GPIO 21   | SDA          | I²C serial data line  |
| SCL    | GPIO 22   | SCL          | I²C serial clock line |
| VCC    | 3.3V      | VIN          | Power supply          |
| GND    | GND       | GND          | Ground reference      |
| A0     | 3.3V      | A0           | I²C address selection |
| A1     | 3.3V      | A1           | I²C address selection |
| A2     | 3.3V      | A2           | I²C address selection |

The TCA9548A address is configured as `0x77`, with A0, A1, and A2 connected to 3.3V.

### TCA9548A — VL53L0X Distance Sensors

| Signal | TCA9548A Pin | Sensor Pin   | Description             |
| ------ | ------------ | ------------ | ----------------------- |
| SDA    | SD0          | SDA (U3)     | I²C data for sensor U3  |
| SCL    | SC0          | SCL (U3)     | I²C clock for sensor U3 |
| SDA    | SD1          | SDA (U4)     | I²C data for sensor U4  |
| SCL    | SC1          | SCL (U4)     | I²C clock for sensor U4 |
| VCC    | 3.3V         | VIN (U3, U4) | Power supply            |
| GND    | GND          | GND (U3, U4) | Ground reference        |

Both VL53L0X sensors use the default I²C address `0x29`. The TCA9548A allows them to be accessed independently by selecting the appropriate channel.

The `XSHUT` pins are not connected to ESP32 GPIOs in the current design. Their electrical state must be guaranteed by the sensor modules.

### ESP32 — GY-87 IMU

| Signal | ESP32 Pin     | GY-87 Pin  | Description                             |
| ------ | ------------- | ---------- | --------------------------------------- |
| VCC    | 3.3V          | VCC_IN     | Module power supply                     |
| GND    | GND           | GND        | Ground reference                        |
| SCL    | GPIO 22       | SCL        | I²C serial clock line                   |
| SDA    | GPIO 21       | SDA        | I²C serial data line                    |
| FSYNC  | Not connected | FSYNC      | Optional external synchronization input |
| INTA   | Not connected | INTA       | Optional MPU6050 interrupt output       |
| DRDY   | Not connected | DRDY       | Optional data-ready signal              |

The GY-87 shares the ESP32's main I²C bus and is accessed directly, without passing through the TCA9548A.

The module's `3V3` pin is left unconnected.

### ESP32 — TCS3200 Color Sensor

| Signal | ESP32 Pin | TCS3200 Pin  | Description                            |
| ------ | --------- | ------------ | -------------------------------------- |
| VCC    | 3.3V      |     VCC      | Sensor power supply                    |
| GND    | GND       |     GND      | Ground reference                       |
| S0     | GPIO 25   |     S0       | Output frequency scaling control       |
| S1     | GPIO 26   |     S1       | Output frequency scaling control       |
| S2     | GPIO 27   |     S2       | Color filter selection                 |
| S3     | GPIO 14   |     S3       | Color filter selection                 |  
| OUT    | GPIO 34   |     OUT      | Frequency output for color measurement |
| LED    | GPIO 33   |     LED      | Illumination control                   |

The ESP32 measures the frequency of the TCS3200 output signal to estimate the intensity of the selected color channel.

### Power Supply

All modules share a common ground reference. The sensor modules are powered from the 3.3V supply, as shown in the schematic.

Ensure that the I²C pull-up resistors do not pull SDA or SCL above 3.3V to protect the ESP32 GPIOs.


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
