#!/usr/bin/env python3
import re
import sys
import yaml
from collections import defaultdict
from html import escape
 
# -----------------------------------
def load_rules(path="rules.yml"):
    with open(path, "r") as f:
        return yaml.safe_load(f)
 
# -----------------------------------
TIMESTAMP_AT_START = re.compile(
    r"""
    ^\s*(
        \[\d{4}-\d{2}-\d{2}[ T]\d{2}:\d{2}:\d{2}(?:\.\d+)?\]? |
        \d{4}-\d{2}-\d{2}[ T]\d{2}:\d{2}:\d{2}(?:\.\d+)? |
        \d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(?:\.\d+)?Z? |
        \d{2}:\d{2}:\d{2}[.,]\d+ |
        [A-Z][a-z]{2}\s+\d{1,2}\s+\d{2}:\d{2}:\d{2}
    )
    """,
    re.VERBOSE
)
 
def has_timestamp_at_start(line):
    return bool(TIMESTAMP_AT_START.match(line))
 
# -----------------------------------
def detect_level(line):
    for lvl in ["ERROR", "WARN", "INFO", "DEBUG", "TRACE"]:
        if re.search(rf"\b{lvl}\b", line):
            return lvl
    return "UNKNOWN"
 
def matches_any(patterns, text):
    return any(re.search(p, text, re.IGNORECASE) for p in patterns)
 
# -----------------------------------
def analyze(log_file, rules):
    noisy = defaultdict(list)
    sensitive = []
    severity_violations = []
 
    in_public_api = False
 
    with open(log_file, "r", errors="ignore") as f:
        for ln, line in enumerate(f, 1):
 
            #  PROCESS ONLY TIMESTAMPED LOGS
            if not has_timestamp_at_start(line):
                continue
 
            level = detect_level(line)
 
            # Detect public API context
            if matches_any(rules["public_api_patterns"], line):
                in_public_api = True
 
            # Noisy logs: internal logs during public API execution
            if in_public_api:
                if matches_any(rules["internal_log_patterns"], line):
                    if level in rules["noisy_log_levels"]:
                        noisy["internal"].append((ln, line.strip()))
 
            # Sensitive / PII detection
            if matches_any(rules["sensitive_patterns"], line):
                sensitive.append((ln, line.strip()))
 
            # Failure severity enforcement
            if matches_any(rules["failure_keywords"], line):
                if level not in rules["required_severity_on_failure"]:
                    severity_violations.append((ln, line.strip()))
 
            # End of request heuristic
            if "request completed" in line.lower():
                in_public_api = False
 
    return noisy, sensitive, severity_violations
 
# -----------------------------------
def generate_html(noisy, sensitive, severity, out="/tmp/noisy_log_report.html"):
    with open(out, "w", encoding="utf-8") as f:
        f.write("""
<html><head><title>Log Quality Report</title>
<style>
body{font-family:Arial;}
table{border-collapse:collapse;width:100%;}
th,td{border:1px solid #ccc;padding:6px;}
th{background:#f0f0f0;}
</style>
</head><body>
<h2>Noisy Logs</h2>
<table>
<tr><th>Type</th><th>Count</th><th>Samples</th></tr>
""")
        for k, logs in noisy.items():
            sample = "<br>".join(escape(l[1]) for l in logs[:5])
            f.write(f"<tr><td>{k}</td><td>{len(logs)}</td><td>{sample}</td></tr>")
 
        f.write("""
</table>
<h2>Sensitive / PII Logs</h2>
<table><tr><th>Line</th><th>Log</th></tr>
""")
        for ln, l in sensitive:
            f.write(f"<tr><td>{ln}</td><td>{escape(l)}</td></tr>")
 
        f.write("""
</table>
<h2>Severity Violations</h2>
<table><tr><th>Line</th><th>Log</th></tr>
""")
        for ln, l in severity:
            f.write(f"<tr><td>{ln}</td><td>{escape(l)}</td></tr>")
 
        f.write("</table></body></html>")
 
    print(f"Report generated: {out}")
 
# -----------------------------------
if __name__ == "__main__":
 
    if len(sys.argv) < 2:
        print("Usage: python3 analyzer.py <log_file>")
        sys.exit(1)
 
    log_file = sys.argv[1]
 
    rules = load_rules()
    noisy, sensitive, severity = analyze(log_file, rules)
    generate_html(noisy, sensitive, severity)
