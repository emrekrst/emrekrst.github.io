"""Compatibility launcher for the consolidated GUI (including waveform editor)."""
import tkinter as tk
from gui_plus import IlMattoGUI
if __name__ == "__main__":
    root = tk.Tk()
    app = IlMattoGUI(root)
    root.mainloop()
