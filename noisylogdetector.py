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
def starts_with_date_and_timestamp(line):
    """
    Matches log lines starting with any of the following timestamp patterns:
      - HH:MM:SS or HH:MM:SS.ssssss (e.g. 04:31:14 or 04:31:14.109764)
      - YYYY-MM-DD HH:MM:SS or YYYY-MM-DD HH:MM:SS.sss (e.g. 2024-11-11 04:31:14 or 2024-11-11 04:31:14.109)
      - Mon DD HH:MM:SS (e.g. Nov 11 04:31:14)
    Lines not matching these patterns at the start will be ignored.

    NOTE: If your log lines are not being reported, check:
      - The timestamp is at the very start of the line.
      - The timestamp matches one of the above formats.
      - If there are leading spaces, adjust the regex to allow them.
    """
    # This regex allows optional leading whitespace before the timestamp.
    return bool(re.match(
        r'^\s*(\d{2}:\d{2}:\d{2}(?:\.\d+)?|\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2}(?:\.\d+)?|'
        r'(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s+\d{1,2}\s+\d{2}:\d{2}:\d{2})',
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
    """
    Analyze a log file for noisy logging, sensitive data exposure, and
    incorrect severity usage based on the provided rules.
    Parameters
    ----------
    log_file : str or pathlib.Path
        Path to the log file to analyze. The file is opened in text mode
        with errors ignored to allow processing partially invalid encodings.
    rules : dict
        Configuration dictionary containing analysis rules. Expected keys:
        - "sensitive_patterns": list of regex patterns that match sensitive
          or PII data that must not appear in logs.
        - "failure_keywords": list of lowercase keywords that indicate a
          failure or error condition in a log line.
        - "noisy_log_levels": iterable of log levels (e.g. "INFO", "DEBUG")
          that are considered noisy.
        - "required_severity_on_failure": iterable of log levels (e.g.
          "ERROR", "WARN") that must be used when a failure keyword is
          present.
    Returns
    -------
    tuple
        A 3-tuple `(noisy_logs, sensitive_logs, severity_violations)` where
        each element is a list of dictionaries describing matching log lines.
        - noisy_logs: entries for logs emitted at noisy log levels.
        - sensitive_logs: entries where sensitive or PII data was detected,
          with similar structure ("line", "log", "reason").
        - severity_violations: entries where a failure keyword was found but
          the log level did not meet the required severity.
    """
    noisy_logs = []
    sensitive_logs = []
    severity_violations = []

    sensitive_res = compile_patterns(rules["sensitive_patterns"])
    failure_keywords = rules["failure_keywords"]

    # Read the full log file once so we can both:
    # - Scan line-by-line (existing behavior), and
    # - Apply sensitive patterns across multiple lines.
    with open(log_file, "r", errors="ignore") as f:
        full_text = f.read()
    # Build a mapping from character offsets to line numbers.
    line_starts = []
    current_line = 1
    line_starts.append((0, current_line))
    for idx, ch in enumerate(full_text):
        if ch == "\n":
            current_line += 1
            line_starts.append((idx + 1, current_line))
    def _offset_to_line(pos):
        """
        Map a character offset in full_text to a 1-based line number using
        the precomputed line_starts list.
        """
        line_no = 1
        for start, ln in line_starts:
            if start > pos:
                break
            line_no = ln
        return line_no
    # Split into lines for existing per-line analysis.
    lines = full_text.splitlines()
    # First, detect multi-line sensitive matches that would not be visible
    # when scanning individual lines.
    for r in sensitive_res:
        for match in r.finditer(full_text):
            matched_text = match.group(0)
            # Only treat as multi-line if the match actually spans lines.
            if "\n" not in matched_text and "\r" not in matched_text:
                continue
            start_pos = match.start()
            ln = _offset_to_line(start_pos)
            if 1 <= ln <= len(lines):
                line_text = lines[ln - 1]
            else:
                line_text = ""
            sensitive_logs.append({
                "line": ln,
                "log": line_text,
                "reason": "Sensitive / PII data detected (multi-line match)"
            })
    # Existing per-line analysis.
    for ln, line in enumerate(lines, 1):
        line = line.rstrip()
        if not starts_with_date_and_timestamp(line):
            continue
        level = detect_level(line)
        # Report all noisy log levels (DEBUG, TRACE, INFO) as noisy logs
        if level in rules["noisy_log_levels"]:
            noisy_logs.append({
                "line": ln,
                "log": line,
                "reason": f"Noisy log level: {level}"
            })
        # Sensitive logs (single-line or line-contained matches)
        for r in sensitive_res:
            if r.search(line):
                sensitive_logs.append({
                    "line": ln,
                    "log": line,
                    "reason": "Sensitive / PII data detected"
                })
                break
        # Severity enforcement
        if any(k in line.lower() for k in failure_keywords):
            if level not in rules["required_severity_on_failure"]:
                severity_violations.append({
                    "line": ln,
                    "log": line,
                    "reason": "Failure logged without ERROR/WARN"
                })

    return noisy_logs, sensitive_logs, severity_violations

# -----------------------------
def generate_html(noisy, sensitive, severity, output):
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
            f.write(f"<h2>{title}</h2>")
            f.write("<table>")
            f.write("<tr><th>Line</th><th>Reason</th><th>Log</th></tr>")
            if not rows:
                f.write('<tr><td colspan="3">No issues found in this section.</td></tr>')
            else:
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

# -----------------------------
if __name__ == "__main__":
    if len(sys.argv) < 3:
        print(
            "Usage: python3 noisylogdetector.py <log_file> <output.html>\n"
            "Note: requires rules.yml in the current working directory."
        )
        sys.exit(1)

    log_file = sys.argv[1]
    output = sys.argv[2]

    if not Path(log_file).exists():
        print(f"Log file not found: {log_file}")
        sys.exit(1)

    rules = load_rules()
    noisy, sensitive, severity = analyze(log_file, rules)
    generate_html(noisy, sensitive, severity, output)

