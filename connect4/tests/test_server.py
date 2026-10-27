import pytest
import serial
import time

# Adjust to your device port (e.g., "COM19" on Windows or "/dev/ttyACM0" on Linux)
PORT = "COM19"
BAUD = 115200

@pytest.fixture(scope="module")
def serial_conn():
    """Fixture to open and close the serial connection once per test module."""
    ser = serial.Serial(PORT, BAUD, timeout=1)
    time.sleep(0.1) # Allow port to settle
    yield ser
    ser.close()

def send_cmd(ser, cmd):
    """Helper function to send a command and read the response line."""
    ser.write((cmd + "\n").encode('ascii'))
    return ser.readline().decode('ascii').strip()

def test_connect_and_idle(serial_conn):
    """Test connection handshake and returning to IDLE state."""
    resp = send_cmd(serial_conn, "CONNECT")
    assert resp == "OK"

    # Trying to get state before starting a game should return NOGAME error
    resp = send_cmd(serial_conn, "STATE")
    assert resp == "NACK: NOGAME"

def test_new_game_initialization(serial_conn):
    """Test starting a new game returns a fresh 42-character empty board."""
    send_cmd(serial_conn, "CONNECT")
    resp = send_cmd(serial_conn, "NEW")
    assert resp.startswith("OK: BOARD:")
    
    board_str = resp.split(": ")[2]
    assert len(board_str) == 42
    assert board_str == "." * 42

def test_column_range_validation(serial_conn):
    """Test boundary checks for column selections (valid cols: 0 to 6)."""
    send_cmd(serial_conn, "CONNECT")
    send_cmd(serial_conn, "NEW")
    
    # Out of range columns
    assert send_cmd(serial_conn, "MOVE:7") == "NACK: RANGE"
    assert send_cmd(serial_conn, "MOVE:-1") == "NACK: RANGE"
    assert send_cmd(serial_conn, "MOVE:9") == "NACK: RANGE"

def test_gravity_and_piece_drop(serial_conn):
    """Test that a piece falls to the bottom-most row of the chosen column."""
    send_cmd(serial_conn, "CONNECT")
    send_cmd(serial_conn, "NEW")
    
    # Drop X into column 3
    resp = send_cmd(serial_conn, "MOVE:3")
    assert resp.startswith("OK: BOARD:")
    
    board_str = resp.split(": ")[2]
    # Row 5, Column 3 corresponds to index (5 * 7) + 3 = 38
    assert board_str[38] == 'X'

def test_full_column_error(serial_conn):
    """Test that filling a column returns NACK: FULL on overflow."""
    send_cmd(serial_conn, "CONNECT")
    send_cmd(serial_conn, "NEW")
    
    # Drop 6 pieces into column 0
    for _ in range(6):
        resp = send_cmd(serial_conn, "MOVE:0")
        assert resp.startswith("OK: BOARD:")
        
    # 7th drop into column 0 should fail
    assert send_cmd(serial_conn, "MOVE:0") == "NACK: FULL"

def test_resignation(serial_conn):
    """Test game resignation command."""
    send_cmd(serial_conn, "CONNECT")
    send_cmd(serial_conn, "NEW")
    
    resp = send_cmd(serial_conn, "RESIGN")
    assert resp == "OK: GAMEOVER"
    
    # Subsequent moves should fail because game is over
    assert send_cmd(serial_conn, "MOVE:3") == "NACK: NOGAME"

def test_disconnect(serial_conn):
    """Test clean disconnection back to IDLE state."""
    resp = send_cmd(serial_conn, "DISCONNECT")
    assert resp == "OK"