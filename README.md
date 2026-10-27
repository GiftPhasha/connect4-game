# Connect-4 Serial Game Server (STM32 & Python GUI)

A Connect-4 embedded systems project featuring an **STM32F051 microcontroller** running an authoritative game server. The server communicates with a Python/Tkinter graphical user interface (GUI) client over UART. The GUI supports click-to-play gameplay and an AI opponent mode.

## Features

- **STM32 game server:** Runs the embedded C firmware and manages the game state.
- **UART communication:** Allows the Python GUI to send commands to the STM32 and receive responses.
- **Graphical interface:** Provides an interactive Connect-4 board using Python and Tkinter.
- **Click-to-play:** Players select a column on the board to make a move.
- **AI opponent:** Supports playing against a simple AI opponent.
- **Input validation:** Rejects invalid commands and moves.
- **Automated testing:** Uses pytest and pySerial to test the server's UART protocol.

## Project Structure

```text
connect4/
├── stm/
│   └── main.c              # Embedded C firmware
├── client/
│   └── gui_client.py       # Python GUI client
├── tests/
│   └── test_server.py      # Automated UART protocol tests
└── README.md               # Project documentation
```

## Requirements

Before running the project, ensure that you have the following:

- STM32CubeIDE or another compatible STM32 development environment.
- An STM32F051 development board.
- A USB connection between the STM32 board and your computer.
- Python 3 installed on your computer.
- Visual Studio Code or another preferred code editor.
- The required Python packages: `pyserial` and `pytest`.

## 1. Build and Run the STM32 Firmware

The STM32 firmware must be running before you connect the GUI or execute the hardware-based tests.

1. Open **STM32CubeIDE**.
2. Open the STM32 project containing `main.c`.
3. Ensure that the correct STM32F051 target and project configuration are selected.
4. Build the project and resolve any compilation errors.
5. Connect the STM32 board to your computer using USB.
6. Flash the compiled firmware onto the board.
7. Run the firmware on the STM32.

The firmware acts as the game server and communicates with the Python application through UART.

**Important:** Ensure that the firmware is running and that the board is connected before proceeding.

## 2. Set Up the Python Environment

Open the project in Visual Studio Code or your preferred IDE.

Open a terminal in the project root directory:

```bash
cd connect4
```

Check that Python is installed:

```bash
python --version
```

Install the required packages:

```bash
python -m pip install pyserial pytest
```

If you are using a virtual environment, activate it before installing the packages.

## 3. Configure the Serial Port

The Python GUI and automated tests must use the correct serial port for the STM32 board.

On Windows, open **Device Manager**, expand **Ports (COM & LPT)**, and identify the COM port assigned to the board.

For example, the board may appear as `COM19`.

Ensure that the port and baud rate configured in `gui_client.py` and `test_server.py` match your setup. The project uses a baud rate of **115200**.

Close PuTTY or any other serial terminal before opening the GUI or running tests, because a serial port generally cannot be used by multiple applications at the same time.

## 4. Run the Python GUI

Open a terminal in the project root directory and navigate to the client folder:

```bash
cd client
```

Run the GUI:

```bash
python gui_client.py
```

The graphical interface should open.

Use the interface to connect to the STM32 server, start a new game, select columns to make moves, and play against the AI if desired.

If the application cannot connect, check that:
- The STM32 firmware is running.
- The correct COM port is configured.
- The baud rate is set to 115200.
- No other application is using the serial port.
- The USB connection is working correctly.

## 5. Run the Automated Tests

The test harness checks the server's responses to commands sent over UART. The STM32 firmware must be running for hardware-based tests.

Open a **new terminal** in the project root directory and navigate to the tests folder:

```bash
cd tests
```

Run the tests:

```bash
python -m pytest test_server.py -v
```

Alternatively, if pytest is available directly on your PATH, use:

```bash
pytest test_server.py -v
```

The test harness uses pySerial to communicate with the board. Ensure that the COM port and baud rate in `test_server.py` match the connected STM32 board.

The tests cover implemented cases such as:
- Connecting to the server.
- Starting a new game.
- Initial board-state validation.
- Column-range validation.
- Legal piece placement and gravity.
- Full-column error handling.
- Resignation and rejection of moves after game over.
- Disconnecting from the server.

The test output displays each test result. A passing test is marked `PASSED`, while a failing test is marked `FAILED`.

**Note:** The tests must match the response strings implemented in the firmware. For example, if the firmware returns `NACK: OCCUPIED` but the test expects `NACK: FULL`, that test must be updated to match the intended protocol, or the firmware response must be changed consistently.

## 6. Running the Project in the Correct Order

For a typical demonstration, follow this sequence:

1. Build and flash the STM32 firmware using STM32CubeIDE.
2. Connect the STM32 board to the computer.
3. Confirm the correct COM port and baud rate.
4. Run the Python GUI using `python gui_client.py`.
5. Connect to the server and start a new game.
6. Play Connect-4 using the GUI and demonstrate the AI opponent if required.
7. Close the GUI before running the automated tests.
8. Run the test harness using `python -m pytest test_server.py -v`.

## Troubleshooting

| Problem | Possible solution |
|---|---|
| `python` is not recognized | Install Python and ensure it is available on your system PATH. |
| `pytest` is not recognized | Run `python -m pytest test_server.py -v` or install pytest using pip. |
| Serial port cannot be opened | Check the COM port and close PuTTY or other serial applications. |
| GUI cannot connect | Confirm that the firmware is running and that the serial settings match. |
| Tests fail due to unexpected responses | Compare the expected strings in `test_server.py` with the responses returned by `main.c`. |
| No response from the board | Check the USB connection, UART configuration, firmware execution, and baud rate. |

## Technologies Used

- **C:** Embedded game-server firmware.
- **STM32F051:** Microcontroller platform.
- **UART:** Serial communication between the board and the client.
- **Python:** GUI client and automated test harness.
- **Tkinter:** Python graphical user interface.
- **pySerial:** Serial communication from Python.
- **pytest:** Automated test execution.
- **STM32CubeIDE:** Firmware development, building, and flashing.

## Future Improvements

Potential improvements include:
- More extensive automated tests for win and draw conditions.
- Improved error handling and recovery from malformed or incomplete commands.
- More advanced AI decision-making.
- Improved UART reliability through additional protocol validation.
- Additional hardware and GUI integration tests.

## Author

Developed as part of the EEE3095S embedded systems project at the University of Cape Town.
