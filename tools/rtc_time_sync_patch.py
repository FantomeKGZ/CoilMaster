from pathlib import Path
import textwrap

workflow = Path('.github/workflows/rtc-time-sync-hardening.yml').read_text(encoding='utf-8')
start_marker = "          python3 - <<'PY'\n"
end_marker = "\n          PY\n"
start = workflow.index(start_marker) + len(start_marker)
end = workflow.index(end_marker, start)
code = textwrap.dedent(workflow[start:end])
exec(compile(code, 'rtc-time-sync-hardening', 'exec'), {'__name__': '__main__'})
