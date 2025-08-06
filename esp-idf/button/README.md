| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C5 | ESP32-C6 | ESP32-C61 | ESP32-H2 | ESP32-P4 | ESP32-S2 | ESP32-S3 | Linux |
| ----------------- | ----- | -------- | -------- | -------- | -------- | --------- | -------- | -------- | -------- | -------- | ----- |

# ESP32 Button LCD Example

This is a simple ESP-IDF example that demonstrates:
- Button press detection using GPIO interrupts
- I2C LCD display control (16x2 character LCD)
- LCD on/off toggling with button presses
- FreeRTOS tasks and queues

## Features

1. **Multiple Button Support**: Uses GPIO 0 (boot button), GPIO 4, and GPIO 5 for button input
2. **LCD Toggle**: Any button press toggles the LCD display on/off
3. **LCD Display**: Shows welcome message and button information when turned on
4. **Status Updates**: LCD shows a running counter when active
5. **Debouncing**: Simple software debouncing for button presses

## Hardware Setup

### Required Components:
- ESP32 development board
- 16x2 I2C LCD display (HD44780 with I2C backpack)
- Jumper wires

### Connections:
- **I2C LCD**:
  - VCC → 3.3V or 5V (depending on your LCD)
  - GND → GND
  - SDA → GPIO 21 (default I2C SDA)
  - SCL → GPIO 22 (default I2C SCL)

- **Button GPIOs**: 
  - GPIO 0 (boot button - built-in on most ESP32 boards)
  - GPIO 4 (connect external button between GPIO 4 and GND) - Optional
  - GPIO 5 (connect external button between GPIO 5 and GND) - Optional

### LCD I2C Address:
The code uses I2C address `0x27` by default. If your LCD uses a different address (commonly `0x3F`), update the `LCD_I2C_ADDRESS` definition in the code.

## How to Build and Flash

1. Make sure you have ESP-IDF installed and configured
2. Navigate to this project directory
3. Set the target (if needed): `idf.py set-target esp32`
4. Build the project: `idf.py build`
5. Flash to your ESP32: `idf.py flash`
6. Monitor the output: `idf.py monitor`

## Usage

1. Press any of the configured buttons to toggle the LCD on/off:
   - Boot button (GPIO 0) - built-in button
   - GPIO 4 button (if connected)
   - GPIO 5 button (if connected)
2. When LCD is ON:
   - First line shows "LCD is ON!"
   - Second line shows which button was pressed or a running counter
3. When LCD is OFF, the display is completely turned off
4. Watch the serial monitor for debug messages

## Code Structure

- **ISR Handler**: `gpio_isr_handler()` - Handles button press interrupts
- **GPIO Task**: `gpio_task()` - Processes button events and controls LCD
- **LCD Status Task**: `lcd_status_task()` - Updates LCD with running counter
- **LCD Functions**: Various helper functions for I2C LCD communication
- **Main Function**: `app_main()` - Initializes I2C, LCD, GPIO, tasks, and interrupts

## Key ESP-IDF Components Used

- GPIO driver for button control
- I2C driver for LCD communication
- FreeRTOS tasks for concurrent operation
- FreeRTOS queues for ISR-to-task communication
- GPIO interrupts for responsive button detection

## Troubleshooting

1. **LCD not working**: 
   - Check I2C connections and try address 0x3F instead of 0x27
   - Try different GPIO pins: SCL=22, SDA=21 (original ESP32 default)
   - Verify LCD power supply (3.3V or 5V depending on your LCD)
2. **No button response**: Ensure GPIO connections and pull-up resistors are properly configured
3. **I2C errors**: 
   - Verify SDA/SCL connections
   - Check if your ESP32 variant supports the configured GPIO pins
   - Common alternative pins: ESP32-C3 (SCL=8, SDA=10), ESP32-S2 (SCL=7, SDA=8)

## Technical Support

* For technical queries, go to the [esp32.com](https://esp32.com/) forum
* For a feature request or bug report, create a [GitHub issue](https://github.com/espressif/esp-idf/issues)
