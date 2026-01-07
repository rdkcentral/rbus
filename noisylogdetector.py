#!/usr/bin/env python3
import re
import sys
import yaml
from collections import defaultdict
from html import escape
 
# -----------------------------------
def load_rules(path="rules.yml"):
    with open(path, "r") as f:
        rules = yaml.safe_load(f)
 
    # Compile sensitive patterns
    rules["sensitive_patterns_compiled"] = []
    for p in rules.get("sensitive_patterns", []):
        try:
            rules["sensitive_patterns_compiled"].append(re.compile(p, re.IGNORECASE))
        except re.error as e:
            print(f"Failed to compile pattern {p}: {e}")
 
    # Compile internal and public API patterns too (optional)
    rules["internal_log_patterns_compiled"] = [re.compile(p, re.IGNORECASE) for p in rules.get("internal_log_patterns", [])]
    rules["public_api_patterns_compiled"] = [re.compile(p, re.IGNORECASE) for p in rules.get("public_api_patterns", [])]
 
    return rules
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
 
def starts_with_timestamp(line):
    return bool(TIMESTAMP_AT_START.match(line))
# --------------------------------------
def detect_level(line):
    for lvl in ["ERROR", "WARN", "INFO", "DEBUG", "TRACE"]:
        if re.search(rf"\b{lvl}\b", line):
            return lvl
    return "UNKNOWN"

def matches_any_compiled(compiled_patterns, text):
    return any(p.search(text) for p in compiled_patterns)

def matches_any(patterns, text):
    for p in patterns:
        if isinstance(p, str):
            if re.search(p, text, re.IGNORECASE):
                return True
        elif isinstance(p, dict):
            for v in p.values():
                if re.search(str(v), text, re.IGNORECASE):
                    return True
    return False
# -----------------------------------
def analyze(log_file, rules):
    noisy = []
    sensitive = []
    severity_violations = []
 
    in_public_api = False
 
    with open(log_file, "r", errors="ignore") as f:
        for ln, line in enumerate(f, 1):
            line = line.rstrip()
            if not line or not starts_with_timestamp(line):
                continue  # skip lines without timestamp at start
 
            level = detect_level(line)
            # Sensitive / PII detection
if matches_any_compiled(rules["sensitive_patterns_compiled"], line):
    sensitive.append({
        "line": ln,
        "log": line,
        "rule": "SENSITIVE_PII_LOG",
        "reason": "Potential sensitive information detected"
    })
 
# Internal API noisy logs
if in_public_api and matches_any_compiled(rules["internal_log_patterns_compiled"], line):
# Public API detection
    if matches_any_compiled(rules["public_api_patterns_compiled"], line):
        in_public_api = True
 
            # Detect public API context
            if matches_any(rules["public_api_patterns"], line):
                in_public_api = True
 
            # Noisy logs: internal logs during public API execution
            if in_public_api and matches_any(rules["internal_log_patterns"], line):
                if level in rules["noisy_log_levels"]:
                    noisy.append({
                        "line": ln,
                        "log": line,
                        "rule": "NOISY_INTERNAL_API_LOG",
                        "reason": "Internal API log printed during public API execution"
                    })
 
            # Sensitive / PII detection
            if matches_any(rules["sensitive_patterns"], line):
                sensitive.append({
                    "line": ln,
                    "log": line,
                    "rule": "SENSITIVE_PII_LOG",
                    "reason": "Potential sensitive information detected"
                })
 
            # Failure severity enforcement
            if matches_any(rules["failure_keywords"], line):
                if level not in rules["required_severity_on_failure"]:
                    severity_violations.append({
                        "line": ln,
                        "log": line,
                        "rule": "SEVERITY_VIOLATION",
                        "reason": f"Expected severity {rules['required_severity_on_failure']}, got {level}"
                    })
 
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
th,td{border:1px solid #ccc;padding:6px;vertical-align:top;}
th{background:#f0f0f0;}
</style>
</head><body>
<h2>Noisy Logs</h2>
<table>
<tr><th>Line</th><th>Rule</th><th>Reason</th><th>Log</th></tr>
""")
        # Group by rule
        rule_map = defaultdict(list)
        for item in noisy:
            rule_map[item["rule"]].append(item)
 
        for rule, items in rule_map.items():
            # show up to 3 full lines per rule
            for sample_item in items[:3]:
                f.write(
                    f"<tr>"
                    f"<td>{sample_item['line']}</td>"
                    f"<td>{sample_item['rule']}</td>"
                    f"<td>{sample_item['reason']}</td>"
                    f"<td>{escape(sample_item['log'])}</td>"
                    f"</tr>"
                )
 
        f.write("""
</table>
<h2>Sensitive / PII Logs</h2>
<table><tr><th>Line</th><th>Rule</th><th>Reason</th><th>Log</th></tr>
""")
        for item in sensitive:
            f.write(
                f"<tr>"
                f"<td>{item['line']}</td>"
                f"<td>{item['rule']}</td>"
                f"<td>{item['reason']}</td>"
                f"<td>{escape(item['log'])}</td>"
                f"</tr>"
            )
 
        f.write("""
</table>
<h2>Severity Violations</h2>
<table><tr><th>Line</th><th>Rule</th><th>Reason</th><th>Log</th></tr>
""")
        for item in severity:
            f.write(
                f"<tr>"
                f"<td>{item['line']}</td>"
                f"<td>{item['rule']}</td>"
                f"<td>{item['reason']}</td>"
                f"<td>{escape(item['log'])}</td>"
                f"</tr>"
            )
 
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
 
