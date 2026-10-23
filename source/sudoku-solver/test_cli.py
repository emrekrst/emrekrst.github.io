import pathlib, subprocess, tempfile
exe = str(pathlib.Path("build/sudoku").resolve())
for data in ["", "s\nx\nq\n", "s\n1\nx\nq\n", "s\n1\n1\nx\nq\n", "a\nq\n", "s\n-2147483648\n1\nq\n"]:
    r = subprocess.run([exe], input=data, text=True, capture_output=True, timeout=5)
    assert r.returncode == 0, r.stderr
for text in ["", "0 "*80, "0 "*80+"x", "0 "*80+"10", "1 "*81, "0 "*82]:
    with tempfile.TemporaryDirectory() as tmp:
        fixture = pathlib.Path(tmp)/"bad.txt"
        fixture.write_text(text)
        r = subprocess.run([exe, str(fixture)],input="",text=True,capture_output=True,timeout=5)
        assert r.returncode == 1, repr(text)
r=subprocess.run([exe,"missing-file"],capture_output=True,timeout=5)
assert r.returncode == 1
print("CLI EOF, nonnumeric input, missing/malformed/conflicting puzzle PASS")
