#!/usr/bin/env python3
import re
import yaml
from collections import defaultdict
 
# -----------------------------
def load_rules(path="rules.yml"):
    with open(path) as f:
        return yaml.safe_load(f)
 
# -----------------------------
def extract_api_name(line):
    m = re.search(r'API[=:]\s*([a-zA-Z0-9_]+)', line)
    return m.group(1) if m else None
 
def extract_log_level(line):
    for lvl in ["ERROR", "WARN", "INFO", "DEBUG", "TRACE"]:
        if lvl in line:
            return lvl
    return "UNKNOWN"
 
# -----------------------------
def is_internal_api(line, internal_patterns):
    return any(p in line.lower() for p in internal_patterns)
 
def contains_failure(line, failure_keywords):
    return any(k in line.lower() for k in failure_keywords)
 
def contains_sensitive(line, patterns):
    return any(re.search(p, line, re.IGNORECASE) for p in patterns)
 
# -----------------------------
def analyze_logs(log_lines, rules):
    noisy = defaultdict(list)
    security_issues = []
    severity_issues = []
 
    for file, line in log_lines:
        api = extract_api_name(line)
        level = extract_log_level(line)
 
        # ---- Noisy log detection
        if api and is_internal_api(line, rules["internal_api_patterns"]):
            if level in rules["noisy_log_levels"]:
                noisy[api].append((file, line))
 
        # ---- Security detection
        if contains_sensitive(line, rules["sensitive_patterns"]):
            security_issues.append((file, line))
 
        # ---- Severity enforcement
        if api and contains_failure(line, rules["failure_keywords"]):
            if level not in rules["required_severity_on_failure"]:
                severity_issues.append((file, line))
 
    return noisy, security_issues, severity_issues
