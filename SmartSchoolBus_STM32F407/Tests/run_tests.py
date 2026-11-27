"""Run from any folder with Python 3 and native gcc in PATH. No board/SD needed."""
import os, pathlib, subprocess, tempfile, zlib, shutil
root=pathlib.Path(__file__).resolve().parents[1]
os.chdir(root)
def call(*args, cwd=None): subprocess.run(args, cwd=cwd, check=True)
with tempfile.TemporaryDirectory(prefix="host-check-",dir=".") as tmp:
    tmp=pathlib.Path(tmp).relative_to(root)
    source=pathlib.Path("Core/Src/main.c").read_text()
    for header in ("board.h","FreeRTOS.h","task.h","queue.h"):
        source=source.replace('#include "'+header+'"', '')
    (tmp/"ui_under_test.c").write_text(source)
    ui=tmp/"ui.exe";storage=tmp/"storage.exe"
    call("gcc","-std=c11","-ICore/Inc","-I"+str(tmp),"Tests/ui_test.c","Core/Src/trip.c","Core/Src/calendar.c","-o",str(ui))
    call(str(ui.resolve()))
    call("gcc","-std=c11","-Wall","-Wextra","-Werror","-ITests","-ICore/Inc","Tests/storage_test.c","Core/Src/trip.c","Core/Src/journal.c","Core/Src/roster.c","-o",str(storage))
    storage=str(storage.resolve())
    def run(folder,mode): call(storage,mode,cwd=folder)
    fresh=tmp/"fresh";fresh.mkdir();run(fresh,"init")
    log=(fresh/"TRIP_LOG.CSV").read_text()
    assert "MANUAL_EXIT" in log and "2026-11-26 12:30:00,HS01" in log
    for line in log.splitlines()[1:]:
        body,crc=line.rsplit(",",1);assert int(crc,16)==zlib.crc32(body.encode())
    # Cannot remove a student from an unfinished trip, including a recorded exit.
    roster=fresh/"STUDENTS.CSV"
    roster.write_text("id,name\nHS01,AN\n")
    run(fresh,"badroster")
    roster.write_text("id,name\nHS02,BINH\nHS01,AN NEW\nHS03,CHI\n")
    run(fresh,"resume");run(fresh,"reset")
    for contents in ("id,name\nHS01,AN\nHS01,BINH\n", "id,name\nHS33,AN\n", "id,name\nHS01,ABCDEFGHIJK\n", "id,name\n", "id,name\nHS01,An\n"):
        roster.write_text(contents);run(fresh,"badroster")
    roster.write_bytes(b"\xef\xbb\xbfid,name\r\nHS32,STUDENT 32\r\nHS01,AN\r\n")
    run(fresh,"reset")
    original=(fresh/"TRIP_LOG.CSV").read_bytes()
    (fresh/"TRIP_LOG.CSV").write_bytes(original[:-4]);run(fresh,"corrupt")
    (fresh/"TRIP_LOG.CSV").write_bytes(original.replace(b"MANUAL_EXIT",b"MANUAL_EXIt"));run(fresh,"corrupt")
    legacy=tmp/"legacy";legacy.mkdir()
    body="1,1,0,REGISTER,0,1234ABCD,0"
    (legacy/"TRIP_LOG.CSV").write_bytes(("seq,uptime_ms,trip,event,slot,uid,onboard,crc32\n"+body+",%08X\n"%zlib.crc32(body.encode())).encode())
    run(legacy,"legacy");run(legacy,"legacyresume")
    assert (legacy/"TRIP_LOG.CSV").read_text().count("datetime,student_id")==1
print("ALL HOST TESTS PASSED. Hardware RTC, display and SD power-loss tests remain separate.")

