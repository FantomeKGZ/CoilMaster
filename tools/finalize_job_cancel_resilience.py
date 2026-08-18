from pathlib import Path

path = Path("firmware/esp32/src/main.cpp")
text = path.read_text(encoding="utf-8")
old = '''    if (activeJobLinkage.linked)\n    {\n        webServer.send(409, "application/json", "{\\"error\\":\\"linked_job_cannot_be_cancelled_here\\"}");\n        return;\n    }\n'''
if text.count(old) != 1:
    raise SystemExit(f"expected linked cancellation guard exactly once, found {text.count(old)}")
text = text.replace(old, '''    // Linked repair/motor/spool metadata is immutable history, not a reason to\n    // trap a delivery that never reached physical START. Cancelling here only\n    // closes the no-run job state; it does not delete snapshots or write off wire.\n''', 1)
path.write_text(text, encoding="utf-8")
print("linked no-run cancellation enabled")
