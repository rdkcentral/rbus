#!/usr/bin/env python3
import os
import re
import tarfile
import yaml
from collections import defaultdict, Counter
import html
 
# -----------------------------
# CONFIG
# -----------------------------
NOISY_THRESHOLD = 10
LOG_EXT_REGEX = re.compile(r'.*\.(log|txt|out|bak|sh)(\.\d+)?$')
 
# -----------------------------
# Load rules
# -----------------------------
def load_rules(path="rules.yml"):
    with open(path) as f:
        return yaml.safe_load(f)
 
# -----------------------------
# Utilities
# -----------------------------
def extract_logs(input_path):
    logs = []  # (file, line)
 
    def read_file(fpath, content):
        for line in content.splitlines():
            if line.strip():
                logs.append((fpath, line.strip()))
 
    if tarfile.is_tarfile(input_path):
        with tarfile.open(input_path, "r:*") as tar:
            for m in tar.getmembers():
                if not m.isfile():
                    continue
                if not LOG_EXT_REGEX.match(m.name.lower()):
                    continue
                f = tar.extractfile(m)
                if not f:
                    continue
                content = f.read().decode("utf-8", "replace")
                read_file(m.name, content)
    else:
        with open('logs.txt') as f:
            for line in f:
            # Example: 05:57:05.793185 RBUS ERROR rbus_tokenchain.c:82 -- Thread-5676: ERROR: regNode NULL
            match = re.search(r'(\d{2}:\d{2}:\d{2}\.\d+) RBUS (\w+) ([\w\.]+):(\d+) -- [^:]+: (.+)', line)
            if match:
                timestamp, severity, file, line_num, message = match.groups()
                print(f"{severity}: {file}:{line_num} - {message}")
            else:
                pass
#with open(input_path, "rb") as f:
#           content = f.read().decode("utf-8", "replace")
#            read_file(os.path.basename(input_path), content)
 
    return logs
 
# -----------------------------
# Extraction helpers
# -----------------------------
def extract_api(line):
    m = re.search(r'\b(rbus_[a-zA-Z_]+|[a-zA-Z_]+_(set|get|update|create|delete))\b', line)
    return m.group(1) if m else "UNKNOWN_API"
 
def extract_level(line):
    u = line.upper()
    for lvl in ["ERROR", "WARN", "INFO", "DEBUG", "TRACE"]:
        if lvl in u:
            return lvl
    return "UNKNOWN"
 
# -----------------------------
# Normalization (IGNORE timestamps, tids, etc.)
# -----------------------------
REMOVE_PAT = re.compile(
    r"""
    \d{4}-\d{2}-\d{2}[ T]\d{2}:\d{2}:\d{2}(?:\.\d+)?|
    [A-Z][a-z]{2}\s+\d{1,2}\s+\d{2}:\d{2}:\d{2}|
    tid[:=]?\d+|pid[:=]?\d+|thread[:=]?\d+|
    \bline[:=]?\d+\b|
    \(\d+\)|\[\d+\]|
    0x[0-9a-f]+|[0-9a-f]{8,}
    """,
    re.IGNORECASE | re.VERBOSE,
)
WS = re.compile(r"\s+")
 
def normalize(line):
    s = REMOVE_PAT.sub(" ", line)
    s = WS.sub(" ", s).strip().lower()
    return s
 
# -----------------------------
# Analysis
# -----------------------------
def analyze(logs, rules):
    noisy_counter = defaultdict(list)
    sensitive_hits = []
    severity_issues = []
 
    for file, line in logs:
        norm = normalize(line)
        api = extract_api(line)
        level = extract_level(line)
        low = line.lower()
 
        # Noisy logs (internal APIs + low severity)
        if any(p in low for p in rules["internal_api_patterns"]):
            if level in rules["noisy_log_levels"]:
                noisy_counter[norm].append((file, line, api, level))
 
        # Sensitive detection
        for pat in rules["sensitive_patterns"]:
            if re.search(pat, line, re.IGNORECASE):
                sensitive_hits.append((file, line))
                break
 
        # Severity enforcement
        if any(k in low for k in rules["failure_keywords"]):
            if level not in rules["required_severity_on_failure"]:
                severity_issues.append((file, line, level))
 
    return noisy_counter, sensitive_hits, severity_issues
 
# -----------------------------
# HTML Report
# -----------------------------
def generate_html(noisy, sensitive, severity):
    rows = []
 
    for norm, entries in noisy.items():
        if len(entries) < NOISY_THRESHOLD:
            continue
 
        count = len(entries)
        locations = sorted(set(f for f, _, _, _ in entries))
        sample_lines = list(dict.fromkeys(l for _, l, _, _ in entries))[:5]
 
        rows.append((count, locations, sample_lines))
 
    rows.sort(reverse=True, key=lambda x: x[0])
 
    html_out = [
        "<html><head><style>",
        "body{font-family:Arial}",
        "table{border-collapse:collapse;width:100%}",
        "th,td{border:1px solid #ccc;padding:6px;font-size:13px;vertical-align:top}",
        "th{background:#f4f4f4}",
        "</style></head><body>",
        "<h1>Noisy Log Analysis Report</h1>",
        f"<p><b>Total Noisy Entries:</b> {len(rows)}</p>",
        "<table>",
        "<tr><th>Count</th><th>Locations</th><th>Log Lines (samples)</th></tr>"
    ]
 
    for count, locs, lines in rows:
        html_out.append(
            "<tr>"
            f"<td>{count}</td>"
            f"<td>{'<br>'.join(html.escape(l) for l in locs)}</td>"
            f"<td>{'<br><br>'.join(html.escape(l) for l in lines)}</td>"
            "</tr>"
        )
 
    html_out.append("</table>")
 
    # Sensitive
    html_out.append("<h2>Sensitive / PII Logs</h2><ul>")
    for f, l in sensitive[:50]:
        html_out.append(f"<li>{html.escape(f)}: {html.escape(l)}</li>")
    html_out.append("</ul>")
 
    # Severity
    html_out.append("<h2>Severity Violations</h2><ul>")
    for f, l, lvl in severity[:50]:
        html_out.append(f"<li>{html.escape(f)} [{lvl}] {html.escape(l)}</li>")
    html_out.append("</ul>")
 
    html_out.append("</body></html>")
    output_path = "/tmp/noisy_log_report.html"
    with open(output_path, "w", encoding="utf-8") as f:
        f.write("\n".join(html_out))
 
# -----------------------------
# MAIN
# -----------------------------
def main():
    import sys
    if len(sys.argv) != 2:
        print("Usage: python noisy_log_analyzer.py <log|tgz|tar>")
        sys.exit(1)
 
    rules = load_rules()
    logs = extract_logs(sys.argv[1])
 
    noisy, sensitive, severity = analyze(logs, rules)
    generate_html(noisy, sensitive, severity)
 
    print(f"Report generated: noisy_log_report.html")
 
if __name__ == "__main__":
    main()

