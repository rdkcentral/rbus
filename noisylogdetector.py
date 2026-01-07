#!/usr/bin/env python3
import re
import sys
import yaml
from collections import defaultdict
from html import escape
 
# -------------------------------
def load_rules(path="rules.yml"):
    with open(path, "r") as f:
        rules = yaml.safe_load(f)
    # Compile regex patterns for speed
    rules["sensitive_patterns_compiled"] = [re.compile(p) for p in rules.get("sensitive_patterns", [])]
    rules["public_api_patterns_compiled"] = [re.compile(p) for p in rules.get("public_api_patterns", [])]
    rules["internal_log_patterns_compiled"] = [re.compile(p, re.IGNORECASE) for p in rules.get("internal_log_patterns", [])]
    rules["failure_keywords_compiled"] = [re.compile(p, re.IGNORECASE) for p in rules.get("failure_keywords", [])]
    return rules
 
# -------------------------------
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
 
def starts_with_timestamp(line):
    return bool(TIMESTAMP_AT_START.match(line))
# -------------------------------
def detect_level(line):
    for lvl in ["ERROR", "WARN", "INFO", "DEBUG", "TRACE"]:
        if re.search(rf"\b{lvl}\b", line):
            return lvl
    return "UNKNOWN"
 
def matches_any_compiled(compiled_patterns, text):
    return any(p.search(text) for p in compiled_patterns)
 
# -------------------------------
def analyze(log_file, rules):
    noisy = []
    sensitive = []
    severity_violations = []
 
    with open(log_file, "r", errors="ignore") as f:
        for ln, line in enumerate(f, 1):
            line = line.rstrip()
            if not line or not starts_with_timestamp(line):
                continue  # skip lines without timestamp at start

            level = detect_level(line)

            # Determine if this line is a public API log (rbus_*)
            is_public_api = matches_any_compiled(rules["public_api_patterns_compiled"], line)

            # Noisy logs: any log not matching rbus_* and with a noisy level
            if not is_public_api and level in rules["noisy_log_levels"]:
                noisy.append({
                    "line": ln,
                    "log": line,
                    "rule": "NOISY_INTERNAL_API_LOG",
                    "reason": "Non-rbus_* log printed at noisy level"
                })

            # Sensitive / PII detection
            if matches_any_compiled(rules["sensitive_patterns_compiled"], line):
                sensitive.append({
                    "line": ln,
                    "log": line,
                    "rule": "SENSITIVE_PII_LOG",
                    "reason": "Potential sensitive information detected"
                })

            # Failure severity enforcement
            if matches_any_compiled(rules["failure_keywords_compiled"], line):
                if level not in rules["required_severity_on_failure"]:
                    severity_violations.append({
                        "line": ln,
                        "log": line,
                        "rule": "SEVERITY_VIOLATION",
                        "reason": f"Expected severity {rules['required_severity_on_failure']}, got {level}"
                    })

    return noisy, sensitive, severity_violations
# -------------------------------
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
<tr><th>Type</th><th>Count</th><th>Reason</th><th>Samples</th></tr>
""")
        for k, logs in noisy.items():
            sample = "<br>".join(escape(l[1]) for l in logs[:5])
            reasons = "<br>".join(escape(l[2]) for l in logs[:5])
            f.write(f"<tr><td>{k}</td><td>{len(logs)}</td><td>{reasons}</td><td>{sample}</td></tr>")
 
        f.write("""
</table>
<h2>Sensitive / PII Logs</h2>
<table><tr><th>Line</th><th>Reason</th><th>Log</th></tr>
""")
        for ln, l, reason in sensitive:
            f.write(f"<tr><td>{ln}</td><td>{escape(reason)}</td><td>{escape(l)}</td></tr>")
 
        f.write("""
</table>
<h2>Severity Violations</h2>
<table><tr><th>Line</th><th>Reason</th><th>Log</th></tr>
""")
        for ln, l, reason in severity:
            f.write(f"<tr><td>{ln}</td><td>{escape(reason)}</td><td>{escape(l)}</td></tr>")
 
        f.write("</table></body></html>")
 
    print(f"Report generated: {out}")
 
# -------------------------------
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 analyzer.py <log_file>")
        sys.exit(1)
 
    log_file = sys.argv[1]
    rules = load_rules()
    noisy, sensitive, severity = analyze(log_file, rules)
    generate_html(noisy, sensitive, severity)
 
