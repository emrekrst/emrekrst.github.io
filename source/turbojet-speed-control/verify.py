"""Run MATLAB in a temporary working directory with isolated preferences/cache."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parent
matlab=os.environ.get('MATLAB_BIN','/Applications/MATLAB_R2024b.app/bin/matlab')
def quote(path): return str(path).replace("'", "''")
with tempfile.TemporaryDirectory(prefix='matlab-release-') as scratch:
    env=os.environ.copy()
    env['MATLAB_PREFDIR']=str(Path(scratch)/'prefs')
    env['TMPDIR']=scratch
    result_dir=root/'results'
    expression="addpath('%s'); " % quote(root)
    expression+="addpath('%s'); test_model_ownership; " % quote(root/'tests')
    expression+="run_all('%s');" % quote(result_dir)
    subprocess.run([matlab,'-batch',expression],cwd=scratch,env=env,timeout=600,check=True)
