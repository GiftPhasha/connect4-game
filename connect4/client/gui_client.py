import tkinter as tk
from tkinter import messagebox
import serial
import serial.tools.list_ports
import random

class Connect4GUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Connect-4 Serial Game Server Client")
        self.ser = None

        # Control Frame
        ctrl_frame = tk.Frame(root)
        ctrl_frame.pack(pady=10)

        tk.Label(ctrl_frame, text="Port:").pack(side=tk.LEFT, padx=5)
        self.port_entry = tk.Entry(ctrl_frame, width=10)
        self.port_entry.insert(0, "COM19")
        self.port_entry.pack(side=tk.LEFT, padx=5)

        self.conn_btn = tk.Button(ctrl_frame, text="Connect", command=self.toggle_connection)
        self.conn_btn.pack(side=tk.LEFT, padx=5)

        self.new_btn = tk.Button(ctrl_frame, text="New Game", command=self.new_game, state=tk.DISABLED)
        self.new_btn.pack(side=tk.LEFT, padx=5)

        # AI Mode Checkbox
        self.ai_var = tk.BooleanVar(value=False)
        self.ai_chk = tk.Checkbutton(ctrl_frame, text="Play vs AI (O)", variable=self.ai_var)
        self.ai_chk.pack(side=tk.LEFT, padx=5)

        # Status Label: Shows which player's turn it is or game status
        self.status_label = tk.Label(root, text="Status: Disconnected", font=("Arial", 12, "bold"), fg="blue")
        self.status_label.pack(pady=5)

        # Board Grid Frame (6 rows x 7 cols) with Click-to-Play
        self.board_frame = tk.Frame(root, bg="blue", padx=5, pady=5)
        self.board_frame.pack(pady=10)

        self.cells = [[None for _ in range(7)] for _ in range(6)]
        for r in range(6):
            for c in range(7):
                lbl = tk.Label(self.board_frame, text="", width=4, height=2, bg="white", font=("Arial", 16, "bold"), relief="raised")
                lbl.grid(row=r, column=c, padx=3, pady=3)
                lbl.bind("<Button-1>", lambda event, col=c: self.make_cell_click(col))
                self.cells[r][c] = lbl

        self.game_active = False
        self.current_board = [['.' for _ in range(7)] for _ in range(6)]
        self.turn = 'X'

    def toggle_connection(self):
        if self.ser is None or not self.ser.is_open:
            try:
                port = self.port_entry.get()
                self.ser = serial.Serial(port, 115200, timeout=1)
                self.send_command("CONNECT")
                self.conn_btn.config(text="Disconnect")
                self.new_btn.config(state=tk.NORMAL)
                self.status_label.config(text="Status: Connected. Click 'New Game' to start.")
                messagebox.showinfo("Success", f"Connected to {port}")
            except Exception as e:
                messagebox.showerror("Error", str(e))
        else:
            self.send_command("DISCONNECT")
            self.ser.close()
            self.ser = None
            self.conn_btn.config(text="Connect")
            self.new_btn.config(state=tk.DISABLED)
            self.game_active = False
            self.status_label.config(text="Status: Disconnected")

    def send_command(self, cmd):
        if self.ser and self.ser.is_open:
            self.ser.write((cmd + "\n").encode('ascii'))
            resp = self.ser.readline().decode('ascii').strip()
            return resp
        return ""

    def new_game(self):
        resp = self.send_command("NEW")
        if resp.startswith("OK: BOARD:"):
            board_str = resp.split(": ")[2]
            self.update_board_display(board_str)
            self.game_active = True
            self.turn = 'X'
            self.status_label.config(text="Turn: Player X's Turn")
        else:
            messagebox.showerror("Server Error", resp)

    def make_cell_click(self, col):
        if not self.game_active:
            messagebox.showwarning("Not Started", "Please click 'New Game' first.")
            return
        
        # If AI mode is active and it's O's turn, block manual clicks
        if self.ai_var.get() and self.turn == 'O':
            return

        self.make_move(col)

    def make_move(self, col):
            resp = self.send_command(f"MOVE:{col}")
            if resp.startswith("OK: BOARD:"):
                board_str = resp.split(": ")[2]
                self.update_board_display(board_str)
                
                self.turn = 'O' if self.turn == 'X' else 'X'
                self.status_label.config(text=f"Turn: Player {self.turn}'s Turn")

                if self.game_active and self.ai_var.get() and self.turn == 'O':
                    self.root.after(400, self.make_ai_move)

            elif resp.startswith("WIN:") or resp == "DRAW":
                final_state = self.send_command("STATE")
                if final_state.startswith("BOARD:"):
                    self.update_board_display(final_state.split(": ")[1])
                self.game_active = False
                self.status_label.config(text=f"Game Over! Result: {resp}")
                messagebox.showinfo("Game Over", f"Result: {resp}")
            
            elif resp.startswith("NACK:"):
                # Handles NACK: OCCUPIED, NACK: RANGE, NACK: NOTURN, NACK: NOGAME, NACK: BADCMD cleanly
                messagebox.showwarning("Server Error / Invalid Move", resp)

    def make_ai_move(self):
        if not self.game_active:
            return
        
        # Simple heuristic AI: Checks for immediate win, blocks opponent win, or picks valid columns
        col = self.calculate_ai_col()
        if col is not None:
            self.make_move(col)

    def calculate_ai_col(self):
        valid_cols = []
        for c in range(7):
            if self.current_board[0][c] == '.':
                valid_cols.append(c)
        
        if not valid_cols:
            return None

        # 1. Check if AI ('O') can win in 1 move
        for c in valid_cols:
            if self.simulates_win(c, 'O'):
                return c

        # 2. Check if Player ('X') can win in 1 move, and block it
        for c in valid_cols:
            if self.simulates_win(c, 'X'):
                return c

        # 3. Otherwise pick center-biased random valid column
        center_preferred = [3, 2, 4, 1, 5, 0, 6]
        for c in center_preferred:
            if c in valid_cols:
                return c
        return random.choice(valid_cols)

    def simulates_win(self, col, player):
        # Simulate drop on local board copy
        sim_board = [row[:] for row in self.current_board]
        row = -1
        for r in range(5, -1, -1):
            if sim_board[r][col] == '.':
                row = r
                break
        if row == -1:
            return False
        sim_board[row][col] = player
        return self.check_win_matrix(sim_board, player)

    def check_win_matrix(self, b, p):
        for r in range(6):
            for c in range(7):
                if b[r][c] == p:
                    if c + 3 < 7 and b[r][c+1] == p and b[r][c+2] == p and b[r][c+3] == p: return True
                    if r + 3 < 6 and b[r+1][c] == p and b[r+2][c] == p and b[r+3][c] == p: return True
                    if r + 3 < 6 and c + 3 < 7 and b[r+1][c+1] == p and b[r+2][c+2] == p and b[r+3][c+3] == p: return True
                    if r + 3 < 6 and c - 3 >= 0 and b[r+1][c-1] == p and b[r+2][c-2] == p and b[r+3][c-3] == p: return True
        return False

    def update_board_display(self, bstr):
        idx = 0
        for r in range(6):
            for c in range(7):
                char = bstr[idx]
                self.current_board[r][c] = char
                idx += 1
                bg = "white"
                if char == 'X':
                    bg = "red"
                elif char == 'O':
                    bg = "yellow"
                self.cells[r][c].config(text=char if char != '.' else '', bg=bg)

if __name__ == "__main__":
    root = tk.Tk()
    app = Connect4GUI(root)
    root.mainloop()