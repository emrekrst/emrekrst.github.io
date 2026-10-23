"""Local pseudoterminal transport test; never opens a physical serial port."""
import importlib.util, os, pathlib, pty, select, subprocess, threading
import serial
root=pathlib.Path(__file__).parents[1]
exe=root/'build/pwm_cli'
errors=[]
def peer(master, expected, reply):
    try:
        _peer(master,expected,reply)
    except Exception as exc:
        errors.append(exc)
def _peer(master, expected, reply):
    data=b''
    while not data.endswith(b'\r'):
        ready,_,_=select.select([master],[],[],3)
        assert ready,'No command received'
        data+=os.read(master,4096)
    assert data==expected,(data,expected)
    os.write(master,reply)
for cmd,reply in [('SET 128',b'Manual PWM: 128\r\n'),('AUTO 512',b'PID ON. Target: 512\r\n'),('READ',b'ADC: 512 | PWM: 128\r\n'),('KP 500',b'KP updated\r\n'),('STOP',b'Monitor stopped\r\n')]:
    master,slave=pty.openpty()
    worker=threading.Thread(target=peer,args=(master,(cmd+'\r').encode(),reply));worker.start()
    result=subprocess.run([str(exe),os.ttyname(slave),*cmd.split()],capture_output=True,timeout=4)
    worker.join();os.close(master);os.close(slave)
    assert not errors,errors
    assert result.returncode==0,result.stderr
    assert reply.replace(b'\r',b'') in result.stdout.replace(b'\r',b''),result.stdout
r=subprocess.run([str(exe),'/no/device','SET'],capture_output=True)
assert r.returncode==2
# Exercise actual recovered GUI methods over real pySerial and a PTY.
spec=importlib.util.spec_from_file_location('gui_plus',root/'gui_plus.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
class Label:
    def __init__(self):self.text='Current: --'
    def cget(self,_):return self.text
    def config(self,**kw):self.text=kw.get('text',self.text)
master,slave=pty.openpty()
a=m.IlMattoGUI.__new__(m.IlMattoGUI);a.ser=serial.Serial(os.ttyname(slave),9600,timeout=.5)
a.lbl_current=Label();a.lbl_prev=Label()
worker=threading.Thread(target=peer,args=(master,b'READ\r',b'ADC: 512 | PWM: 128\r\n'));worker.start()
a.read_once();worker.join();a.ser.close();os.close(master);os.close(slave)
assert not errors,errors
assert 'ADC: 512' in a.lbl_current.text,a.lbl_current.text
print('C++ CLI and GUI pySerial pseudoterminal roundtrips PASS')
