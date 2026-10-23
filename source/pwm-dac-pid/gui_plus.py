import tkinter as tk
from tkinter import ttk
import serial
import serial.tools.list_ports
import threading
import time

class IlMattoGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Il Matto Control Panel")
        self.root.geometry("600x850")
        
        self.ser = None
        self.monitoring = False
        self.wave_playing = False
        self.wave_after_id = None
        self.wave_index = 0
        
        self.wave_resolution = 50
        self.wave_data = [0] * self.wave_resolution
        
        conn_frame = ttk.LabelFrame(root, text="Connection")
        conn_frame.pack(pady=10, padx=10, fill="x")
        
        ttk.Label(conn_frame, text="Port:").pack(side="left", padx=5)
        self.port_combo = ttk.Combobox(conn_frame, width=25)
        self.port_combo.pack(side="left", padx=5)
        self.refresh_ports()
        
        ttk.Button(conn_frame, text="⟳", width=3, command=self.refresh_ports).pack(side="left")
        self.btn_connect = ttk.Button(conn_frame, text="Connect", command=self.toggle_connection)
        self.btn_connect.pack(side="left", padx=10)
        
        self.status_canvas = tk.Canvas(conn_frame, width=20, height=20)
        self.status_circle = self.status_canvas.create_oval(2, 2, 18, 18, fill="red")
        self.status_canvas.pack(side="left", padx=5)
        
        ctrl_frame = ttk.LabelFrame(root, text="Voltage Control")
        ctrl_frame.pack(pady=10, padx=10, fill="x")
        
        ttk.Label(ctrl_frame, text="Target Voltage(Auto PID):").pack(anchor="w", padx=5)
        self.slider_target = tk.Scale(ctrl_frame, from_=0, to=1023, orient="horizontal", length=500)
        self.slider_target.pack(pady=5)
        ttk.Button(ctrl_frame, text="SET TARGET", command=self.send_auto).pack(pady=5)
        
        ttk.Label(ctrl_frame, text="Constant PWM (Manual):").pack(anchor="w", padx=5, pady=(15,0))
        self.slider_pwm = tk.Scale(ctrl_frame, from_=0, to=255, orient="horizontal", length=500)
        self.slider_pwm.pack(pady=5)
        ttk.Button(ctrl_frame, text="SET PWM", command=self.send_pwm).pack(pady=5)

        pid_frame = ttk.LabelFrame(root, text="PID Tuning")
        pid_frame.pack(pady=10, padx=10, fill="x")
        f_pid = ttk.Frame(pid_frame)
        f_pid.pack(pady=5)
        
        ttk.Label(f_pid, text="Kp:").pack(side="left")
        self.entry_kp = ttk.Entry(f_pid, width=6)
        self.entry_kp.insert(0, "0.5")
        self.entry_kp.pack(side="left", padx=5)
        
        ttk.Label(f_pid, text="Ki:").pack(side="left")
        self.entry_ki = ttk.Entry(f_pid, width=6)
        self.entry_ki.insert(0, "0.05")
        self.entry_ki.pack(side="left", padx=5)
        
        ttk.Label(f_pid, text="Kd:").pack(side="left")
        self.entry_kd = ttk.Entry(f_pid, width=6)
        self.entry_kd.insert(0, "0.001")
        self.entry_kd.pack(side="left", padx=5)
        
        ttk.Button(f_pid, text="UPDATE PID", command=self.send_pid).pack(side="left", padx=10)
        
        wave_frame = ttk.LabelFrame(root, text="Arbitrary Waveform Generator (Draw Here)")
        wave_frame.pack(pady=10, padx=10, fill="x")
        
        self.cv_height = 150
        self.cv_width = 500
        self.cv = tk.Canvas(wave_frame, width=self.cv_width, height=self.cv_height, bg="white", cursor="cross")
        self.cv.pack(pady=5)
        self.cv.bind("<B1-Motion>", self.draw_wave) # Bind mouse drag
        self.cv.bind("<Button-1>", self.draw_wave)  # Bind mouse click
        
        self.draw_grid()
        
        f_wave_btn = ttk.Frame(wave_frame)
        f_wave_btn.pack(pady=5)
        self.btn_play_wave = ttk.Button(f_wave_btn, text="PLAY WAVE LOOP", command=self.start_wave)
        self.btn_play_wave.pack(side="left", padx=5)
        self.btn_stop_wave = ttk.Button(f_wave_btn, text="STOP WAVE", command=self.stop_wave, state="disabled")
        self.btn_stop_wave.pack(side="left", padx=5)
        ttk.Button(f_wave_btn, text="CLEAR", command=self.clear_wave).pack(side="left", padx=5)

        disp_frame = ttk.LabelFrame(root, text="Readings")
        disp_frame.pack(pady=10, padx=10, fill="x")
        
        self.lbl_current = ttk.Label(disp_frame, text="Current: -- V (ADC: --) | PWM: --", font=("Arial", 16, "bold"))
        self.lbl_current.pack(pady=10)
        self.lbl_prev = ttk.Label(disp_frame, text="Previous: N/A", font=("Arial", 12))
        self.lbl_prev.pack(pady=5)
        
        btn_frame = ttk.Frame(disp_frame)
        btn_frame.pack(pady=10)
        ttk.Button(btn_frame, text="READ ONCE", command=self.read_once).pack(side="left", padx=5)
        self.btn_monitor = ttk.Button(btn_frame, text="MONITOR", command=self.start_monitor)
        self.btn_monitor.pack(side="left", padx=5)
        self.btn_stop = ttk.Button(btn_frame, text="STOP MONITOR", command=self.stop_monitor, state="disabled")
        self.btn_stop.pack(side="left", padx=5)


    def refresh_ports(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        self.port_combo['values'] = ports
        if ports: self.port_combo.current(0)

    def toggle_connection(self):
        if self.ser and self.ser.is_open:
            self.stop_monitor()
            self.stop_wave()
            self.ser.close()
            self.status_canvas.itemconfig(self.status_circle, fill="red")
            self.btn_connect.config(text="Connect")
        else:
            try:
                self.ser = serial.Serial(self.port_combo.get(), 9600, timeout=1)
                self.status_canvas.itemconfig(self.status_circle, fill="green")
                self.btn_connect.config(text="Disconnect")
            except Exception as e:
                print("Connection error:", e)


    def send_cmd(self, cmd):
        if self.ser and self.ser.is_open:
            full_cmd = cmd + "\r" 
            self.ser.write(full_cmd.encode())

    def send_auto(self):
        self.stop_wave()
        self.send_cmd(f"AUTO {self.slider_target.get()}")

    def send_pwm(self):
        self.stop_wave()
        self.send_cmd(f"SET {self.slider_pwm.get()}")

    def send_pid(self):
        try:
            kp = int(float(self.entry_kp.get()) * 1000)
            ki = int(float(self.entry_ki.get()) * 1000)
            kd = int(float(self.entry_kd.get()) * 1000)
            self.send_cmd(f"KP {kp}")
            time.sleep(0.05)
            self.send_cmd(f"KI {ki}")
            time.sleep(0.05)
            self.send_cmd(f"KD {kd}")
        except:
            print("Invalid PID")

    def read_once(self):
        if not self.ser: return
        curr = self.lbl_current.cget("text")
        if "Current:" in curr:
            self.lbl_prev.config(text=curr.replace("Current:", "Previous:"))
        self.send_cmd("READ")
        try:
            # Earlier command acknowledgements can precede the requested ADC line.
            for _ in range(10):
                line = self.ser.readline().decode().strip()
                if not line:
                    break
                if line.startswith("ADC:"):
                    self.update_display(line)
                    break
        except: pass

    def update_display(self, line):
        try:
            parts = line.split('|')
            adc_val = int(parts[0].split(':')[1])
            voltage = (adc_val / 1023.0) * 3.3
            pwm_str = parts[1].split(':')[1].strip() if len(parts) > 1 else "--"
            self.lbl_current.config(text=f"Current: {voltage:.2f} V (ADC: {adc_val}) | PWM: {pwm_str}")
        except: pass

    def draw_grid(self):
        self.cv.delete("all")
        self.cv.create_line(0, self.cv_height-1, self.cv_width, self.cv_height-1, fill="gray")
        self.cv.create_line(0, 0, self.cv_width, 0, fill="gray")
        self.cv.create_text(10, 10, text="3.3V (255)", anchor="w")
        self.cv.create_text(10, self.cv_height-10, text="0V (0)", anchor="w")

    def clear_wave(self):
        self.draw_grid()
        self.wave_data = [0] *self.wave_resolution


    def draw_wave(self, event):
        x = event.x
        if x < 0: x = 0
        if x >= self.cv_width: x = self.cv_width - 1
        
        index = int((x / self.cv_width) * self.wave_resolution)
        
        y = event.y
        if y < 0: y = 0
        if y > self.cv_height: y = self.cv_height
        
        pwm_val = int(255 - (y / self.cv_height * 255))
        
        self.wave_data[index] = pwm_val
        
        r = 2
        self.cv.create_oval(x-r, y-r, x+r, y+r, fill="blue", outline="blue")

    def start_wave(self):
        if not self.ser or not self.ser.is_open: return
        self.stop_wave()
        self.stop_monitor()
        self.wave_playing = True
        self.wave_index = 0
        self.btn_play_wave.config(state="disabled")
        self.btn_stop_wave.config(state="normal")
        self.wave_after_id = self.root.after(0, self.wave_loop)

    def stop_wave(self):
        self.wave_playing = False
        if self.wave_after_id is not None:
            self.root.after_cancel(self.wave_after_id)
            self.wave_after_id = None
        self.btn_play_wave.config(state="normal")
        self.btn_stop_wave.config(state="disabled")

    def wave_loop(self):
        # Tk runs this callback and mode changes serially: no stale SET after AUTO.
        self.wave_after_id = None
        if not self.wave_playing or not self.ser or not self.ser.is_open:
            self.stop_wave()
            return
        try:
            self.send_cmd(f"SET {self.wave_data[self.wave_index]}")
            if self.ser.in_waiting:
                self.ser.read_all()
        except Exception:
            self.stop_wave()
            return
        self.wave_index = (self.wave_index + 1) % self.wave_resolution
        self.wave_after_id = self.root.after(50, self.wave_loop)

    def start_monitor(self):
        if not self.ser: return
        self.stop_wave()
        self.monitoring = True
        self.btn_monitor.config(state="disabled")
        self.btn_stop.config(state="normal")
        self.send_cmd("MONITOR")
        threading.Thread(target=self.monitor_loop, daemon=True).start()

    def stop_monitor(self):
        self.monitoring = False
        self.send_cmd("STOP")
        self.btn_monitor.config(state="normal")
        self.btn_stop.config(state="disabled")

    def monitor_loop(self):
        while self.monitoring and self.ser and self.ser.is_open:
            if self.ser.in_waiting:
                try:
                    line = self.ser.readline().decode().strip()
                    if "ADC:" in line:
                        self.root.after(0, self.update_display, line)
                except: pass
            time.sleep(0.01)
if __name__ == "__main__":
    root = tk.Tk()
    app = IlMattoGUI(root)
    root.mainloop()