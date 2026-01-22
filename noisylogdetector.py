#!/usr/bin/env python3

import re
import sys
import yaml
from html import escape
from pathlib import Path

# -----------------------------
def load_rules(path="rules.yml"):
    try:
        with open(path, "r") as f:
            return yaml.safe_load(f)
    except FileNotFoundError:
        print(f"Rules file not found: {path}", file=sys.stderr)
        sys.exit(1)
    except PermissionError:
        print(f"Permission denied while reading rules file: {path}", file=sys.stderr)
        sys.exit(1)
    except yaml.YAMLError as e:
        print(f"Failed to parse YAML rules file '{path}': {e}", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Unexpected error while loading rules from '{path}': {e}", file=sys.stderr)
        sys.exit(1)

# -----------------------------
def validate_rules(rules):
    required_keys = [
        "public_api_patterns",
        "sensitive_patterns",
        "noisy_log_levels",
        "failure_keywords",
        "required_severity_on_failure"
    ]
    for key in required_keys:
        if key not in rules or not isinstance(rules[key], list):
            print(f"Rules file missing or invalid key: '{key}'", file=sys.stderr)
            sys.exit(1)
    # End patterns intentionally not checked: API context continues until next API call.

# -----------------------------
def starts_with_date_and_timestamp(line):
    """
    Matches:
    04:31:14.109764
    2024-11-11 04:31:14.109
    Nov 11 04:31:14
    """
    return bool(re.match(
        r'^\s*(\d{2}:\d{2}:\d{2}\.\d+|\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2}\.\d+|\w{3}\s+\d+\s+\d{2}:\d{2}:\d{2})',
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
def detect_public_api_context(lines, rules):
    public_api_res = compile_patterns(rules["public_api_patterns"])
    context = []
    in_public_api = False
    active_public_api = None
    for ln, line in lines:
        found_api = None
        for r in public_api_res:
            m = r.search(line)
            if m:
                found_api = m.group(0)
                break
        if found_api:
            in_public_api = True
            active_public_api = found_api
        elif not found_api and in_public_api:
            # Remain in API context until next API call
            pass
        else:
            in_public_api = False
            active_public_api = None
        context.append((ln, line, in_public_api, active_public_api))
    return context

def find_noisy_logs(context, rules):
    noisy_logs = []
    public_api_res = compile_patterns(rules["public_api_patterns"])
    for ln, line, in_public_api, active_public_api in context:
        level = detect_level(line)
        # A log is noisy if we're in a public API context and the line does NOT start a new API call
        is_api_start = any(r.search(line) for r in public_api_res)
        if in_public_api and not is_api_start:
            if level in rules["noisy_log_levels"]:
                noisy_logs.append({
                    "line": ln,
                    "level": level,
                    "log": line,
                    "reason": f"Internal log during public API execution ({active_public_api})"
                })
    return noisy_logs

def find_sensitive_logs(lines, rules):
    sensitive_res = compile_patterns(rules["sensitive_patterns"])
    sensitive_logs = []
    for ln, line in lines:
        for r in sensitive_res:
            if r.search(line):
                sensitive_logs.append({
                    "line": ln,
                    "log": line,
                    "reason": "Sensitive / PII data detected"
                })
                break
    return sensitive_logs

def find_severity_violations(lines, rules):
    failure_keywords = rules["failure_keywords"]
    severity_violations = []
    for ln, line in lines:
        level = detect_level(line)
        if any(k in line.lower() for k in failure_keywords):
            if level not in rules["required_severity_on_failure"]:
                severity_violations.append({
                    "line": ln,
                    "level": level,
                    "log": line,
                    "reason": "Failure logged without ERROR/WARN"
                })
    return severity_violations

def analyze(log_file, rules):
    lines = []
    try:
        with open(log_file, "r", errors="ignore") as f:
            for ln, line in enumerate(f, 1):
                line = line.rstrip()
                if not starts_with_date_and_timestamp(line):
                    continue
                lines.append((ln, line))
    except FileNotFoundError:
        print(f"Log file not found: {log_file}", file=sys.stderr)
        sys.exit(1)
    except PermissionError:
        print(f"Permission denied when reading log file: {log_file}", file=sys.stderr)
        sys.exit(1)
    except OSError as e:
        print(f"Error reading log file {log_file}: {e}", file=sys.stderr)
        sys.exit(1)
    context = detect_public_api_context(lines, rules)
    noisy_logs = find_noisy_logs(context, rules)
    sensitive_logs = find_sensitive_logs(lines, rules)
    severity_violations = find_severity_violations(lines, rules)
    return noisy_logs, sensitive_logs, severity_violations

# -----------------------------
def generate_html(noisy, sensitive, severity, output):
    try:
        with open(output, "w", encoding="utf-8") as f:
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
                if not rows:
                    f.write(f"<h2>{title}</h2>")
                    f.write("<p>No issues found.</p>")
                    return
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
        print(f"Report generated: {output}")
    except Exception as e:
        print(f"Failed to write HTML report to '{output}': {e}", file=sys.stderr)
        sys.exit(1)

# -----------------------------
if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python3 noisylogdetector.py <log_file> <output.html> (requires rules.yml in the current directory)")
        sys.exit(1)
    log_file = sys.argv[1]
    output = sys.argv[2]
    if not Path(log_file).exists():
        print(f"Log file not found: {log_file}")
        sys.exit(1)
    rules = load_rules()
    validate_rules(rules)
    noisy, sensitive, severity = analyze(log_file, rules)
    generate_html(noisy, sensitive, severity, output)
