import importlib.util
import pathlib
from types import SimpleNamespace
spec=importlib.util.spec_from_file_location('gui_plus',pathlib.Path(__file__).parents[1]/'gui_plus.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
class Label:
    def __init__(self): self.text='Current: old'
    def cget(self,key): return self.text
    def config(self,**kwargs): self.text=kwargs.get('text',self.text)
class Serial:
    is_open=True
    def __init__(self): self.sent=[];self.lines=[b'PID ON. Target: 512\r\n',b'ADC: 512 | PWM: 128\r\n']
    def write(self,b):self.sent.append(b)
    def readline(self):return self.lines.pop(0) if self.lines else b''
a=m.IlMattoGUI.__new__(m.IlMattoGUI);a.ser=Serial();a.lbl_current=Label();a.lbl_prev=Label()
a.read_once()
assert a.ser.sent==[b'READ\r'],a.ser.sent
assert 'ADC: 512' in a.lbl_current.text,a.lbl_current.text
assert '1.65 V' in a.lbl_current.text
# Outside-canvas drawing must stay within the 50 sample array and 0..255 range.
a.cv_width=500;a.cv_height=150;a.wave_resolution=50;a.wave_data=[0]*50
a.cv=SimpleNamespace(create_oval=lambda *args,**kwargs:None)
a.draw_wave(SimpleNamespace(x=-20,y=-40));assert a.wave_data[0]==255
a.draw_wave(SimpleNamespace(x=999,y=999));assert a.wave_data[49]==0
print('GUI READ framing, stale response handling, voltage display and 50-sample bounds PASS')

# Playback must share Tk's event loop so mode changes can cancel every future SET.
class Scheduler:
    def __init__(self): self.jobs={}; self.next_id=0
    def after(self,delay,callback,*args):
        self.next_id+=1
        self.jobs[self.next_id]=(delay,callback,args)
        return self.next_id
    def after_cancel(self,job): del self.jobs[job]
    def run_next(self):
        job=next(iter(self.jobs))
        _,callback,args=self.jobs.pop(job)
        callback(*args)
class NoWorker:
    def __init__(self,**kwargs): pass
    def start(self): pass
class PlaybackSerial(Serial):
    in_waiting=0
    def close(self): self.is_open=False
    def write(self,b):
        assert self.is_open,'Write after disconnect'
        super().write(b)
# Suppress only background acquisition; waveform scheduling is exercised unchanged.
m.threading=SimpleNamespace(Thread=NoWorker)
def playback():
    a=m.IlMattoGUI.__new__(m.IlMattoGUI)
    a.root=Scheduler();a.ser=PlaybackSerial();a.monitoring=False
    a.wave_playing=False;a.wave_after_id=None;a.wave_index=0
    a.wave_resolution=50;a.wave_data=list(range(50))
    for name in ['btn_play_wave','btn_stop_wave','btn_monitor','btn_stop','btn_connect']:
        setattr(a,name,Label())
    a.slider_target=SimpleNamespace(get=lambda:512)
    a.slider_pwm=SimpleNamespace(get=lambda:200)
    a.status_canvas=SimpleNamespace(itemconfig=lambda *args,**kwargs:None)
    a.status_circle=1
    return a
for mode in ['stop_wave','send_auto','send_pwm','start_monitor','toggle_connection']:
    a=playback();a.start_wave()
    assert a.root.jobs,'Waveform playback must queue a cancellable Tk callback'
    a.root.run_next() # One sample is allowed before the mode change.
    getattr(a,mode)()
    commands=list(a.ser.sent)
    assert not a.root.jobs,(mode,'uncancelled waveform callback')
    assert not a.wave_playing
    if mode=='send_auto': assert commands[-1]==b'AUTO 512\r',commands
    if mode=='send_pwm': assert commands[-1]==b'SET 200\r',commands
    if mode=='start_monitor': assert commands[-1]==b'MONITOR\r',commands
    if mode=='toggle_connection': assert not a.ser.is_open
# Starting twice must leave only one loop, and all 50 samples must repeat in order.
a=playback();a.start_wave();a.start_wave()
assert len(a.root.jobs)==1
for _ in range(51): a.root.run_next()
a.stop_wave()
assert [cmd for cmd in a.ser.sent if cmd.startswith(b'SET ')]==[
    f'SET {n}\r'.encode() for n in list(range(50))+[0]]
assert not a.root.jobs
print('GUI cancellable waveform loop, mode changes, disconnect and 50-sample wrap PASS')
