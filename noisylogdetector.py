#!/usr/bin/env python3
import re
import sys
import yaml
from collections import defaultdict
from html import escape
from pathlib import Path
 
# -----------------------------
def load_rules(path="rules.yml"):
    with open(path, "r") as f:
        return yaml.safe_load(f)
 
# -----------------------------
def starts_with_timestamp(line):
    """
    Matches:
    04:31:14.109764
    2024-11-11 04:31:14.109
    Nov 11 04:31:14
    """
    return bool(re.match(
        r'^(\d{2}:\d{2}:\d{2}\.\d+|\d{4}-\d{2}-\d{2}|\w{3}\s+\d+\s+\d{2}:\d{2}:\d{2})',
        line
    ))
 
def detect_level(line):
    for lvl in ("ERROR", "WARN", "INFO", "DEBUG", "TRACE"):
        if re.search(rf"\b{lvl}\b", line):
            return lvl
    return "UNKNOWN"
 
# -----------------------------
def compile_patterns(patterns):
    return [re.compile(p, re.IGNORECASE) for p in patterns]
 
# -----------------------------
def analyze(log_file, rules):
    noisy_logs = []
    sensitive_logs = []
    severity_violations = []
 
    public_api_res = compile_patterns(rules["public_api_patterns"])
    sensitive_res = compile_patterns(rules["sensitive_patterns"])
    failure_keywords = rules["failure_keywords"]
 
    in_public_api = False
    active_public_api = None
 
    with open(log_file, "r", errors="ignore") as f:
        for ln, line in enumerate(f, 1):
            line = line.rstrip()
 
            if not starts_with_timestamp(line):
                continue
 
            level = detect_level(line)
 
            # ----------------- Detect public API start
            for r in public_api_res:
                m = r.search(line)
                if m:
                    in_public_api = True
                    active_public_api = m.group(0)
                    break
 
            # ----------------- Noisy logs
            if in_public_api:
                if active_public_api not in line:
                    if level in rules["noisy_log_levels"]:
                        noisy_logs.append({
                            "line": ln,
                            "level": level,
                            "log": line,
                            "reason": f"Internal log during public API execution ({active_public_api})"
                        })
 
            # ----------------- Sensitive logs
            for r in sensitive_res:
                if r.search(line):
                    sensitive_logs.append({
                        "line": ln,
                        "log": line,
                        "reason": "Sensitive / PII data detected"
                    })
                    break
 
            # ----------------- Severity enforcement
            if any(k in line.lower() for k in failure_keywords):
                if level not in rules["required_severity_on_failure"]:
                    severity_violations.append({
                        "line": ln,
                        "level": level,
                        "log": line,
                        "reason": "Failure logged without ERROR/WARN"
                    })
 
 
    return noisy_logs, sensitive_logs, severity_violations
 
# -----------------------------
def generate_html(noisy, sensitive, severity, out="/tmp/noisy_log_report.html"):
    with open(out, "w", encoding="utf-8") as f:
        f.write("""
<html>
<head>
<title>Log Quality Report</title>
<style>
body { font-family: Arial; }
table { border-collapse: collapse; width: 100%; margin-bottom: 30px; }
th, td { border: 1px solid #ccc; padding: 6px; text-align: left; }
th { background: #f0f0f0; }
</style>
</head>
<body>
<h1>Log Quality Report</h1>
""")
 
        def write_section(title, rows):
            f.write(f"<h2>{title}</h2>")
            f.write("<table>")
            f.write("<tr><th>Line</th><th>Reason</th><th>Log</th></tr>")
            for r in rows:
                f.write(
                    f"<tr><td>{r['line']}</td>"
                    f"<td>{escape(r['reason'])}</td>"
                    f"<td>{escape(r['log'])}</td></tr>"
                )
            f.write("</table>")
 
        write_section("Noisy Logs", noisy)
        write_section("Sensitive / PII Logs", sensitive)
        write_section("Severity Violations", severity)
 
        f.write("</body></html>")
 
    print(f"✅ Report generated: {out}")
 
# -----------------------------
if __name__ == "__main__":
 
    if len(sys.argv) < 2:
        print("Usage: python3 noisy_log_detector.py <log_file>")
        sys.exit(1)
 
    log_file = sys.argv[1]
 
    if not Path(log_file).exists():
        print(f"Log file not found: {log_file}")
        sys.exit(1)
 
    rules = load_rules()
    noisy, sensitive, severity = analyze(log_file, rules)
    generate_html(noisy, sensitive, severity)
 
