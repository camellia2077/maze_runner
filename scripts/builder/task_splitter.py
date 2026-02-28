# builder/task_splitter.py
"""
Split clang-tidy output into task_XXX.log files for static analysis.

Features:
- Sorts tasks by size (smaller first)
- Sub-splits large tasks (>50 lines) by header file
- Adds a summary header with files and warning types
"""

import re
from collections import defaultdict
from pathlib import Path
from typing import Dict, List, Optional, Tuple

TASK_START_PATTERN = re.compile(
    r"^\[(\d+)/(\d+)\]\s+\[\d+/CHECK\]\s+Analyzing:\s+(.+)$"
)

WARNING_LINE_PATTERN = re.compile(
    r"^([A-Za-z]:)?([^:]+):(\d+):(\d+):\s+warning:\s+(.+)\s+\[([^\]]+)\]"
)

LARGE_TASK_THRESHOLD = 50


def _extract_warnings(lines: List[str]) -> List[Dict]:
    warnings = []
    for line in lines:
        match = WARNING_LINE_PATTERN.match(line.strip())
        if match:
            drive = match.group(1) or ""
            file_path = drive + match.group(2)
            warnings.append({
                "file": file_path,
                "line": int(match.group(3)),
                "col": int(match.group(4)),
                "message": match.group(5),
                "check": match.group(6),
            })
    return warnings


def _generate_summary(warnings: List[Dict]) -> str:
    if not warnings:
        return ""

    file_counts: Dict[str, int] = defaultdict(int)
    check_counts: Dict[str, int] = defaultdict(int)

    for warning in warnings:
        filename = Path(warning["file"]).name
        file_counts[filename] += 1
        check_counts[warning["check"]] += 1

    lines = ["=== SUMMARY ==="]
    file_parts = [f"{name}({count})" for name, count in sorted(file_counts.items(), key=lambda item: -item[1])]
    lines.append(f"Files: {', '.join(file_parts)}")

    check_parts = [f"{name}({count})" for name, count in sorted(check_counts.items(), key=lambda item: -item[1])]
    lines.append(f"Types: {', '.join(check_parts)}")

    lines.append("=" * 15)
    lines.append("")

    return "\n".join(lines)


def _group_by_header(lines: List[str], warnings: List[Dict]) -> Dict[str, Tuple[List[str], List[Dict]]]:
    header_groups: Dict[str, List[str]] = defaultdict(list)
    header_warnings: Dict[str, List[Dict]] = defaultdict(list)

    task_start_line = None
    for line in lines:
        if TASK_START_PATTERN.match(line.strip()):
            task_start_line = line
            break

    for warning in warnings:
        header_name = Path(warning["file"]).name
        header_warnings[header_name].append(warning)

    for line in lines:
        stripped = line.strip()
        if not stripped or TASK_START_PATTERN.match(stripped):
            continue

        matched_header = None
        for header in header_warnings.keys():
            if header in line:
                matched_header = header
                break

        if matched_header:
            header_groups[matched_header].append(line)
        elif "warning:" in line.lower() or "note:" in line.lower():
            pass
        elif "Suppressed" in line or "Use -header-filter" in line:
            for header in header_groups:
                if line not in header_groups[header]:
                    header_groups[header].append(line)

    result = {}
    for header, header_warning_list in header_warnings.items():
        header_lines = header_groups.get(header, [])
        if task_start_line:
            header_lines = [task_start_line] + header_lines
        result[header] = (header_lines, header_warning_list)

    return result


def _create_task_content(
    source_file: str,
    lines: List[str],
    warnings: List[Dict],
    sub_header: Optional[str] = None,
) -> str:
    header = f"File: {source_file}"
    if sub_header:
        header += f" [Sub: {sub_header}]"
    header += "\n" + "=" * 60 + "\n"

    summary = _generate_summary(warnings)
    content = header + summary + "".join(lines)
    return content


def split_tidy_logs(log_lines: List[str], output_dir: Path) -> int:
    output_dir.mkdir(parents=True, exist_ok=True)

    tasks: Dict[int, Dict] = {}
    current_task: Optional[int] = None
    current_file: Optional[str] = None
    current_lines: List[str] = []

    for line in log_lines:
        match = TASK_START_PATTERN.match(line.strip())

        if match:
            if current_task is not None:
                tasks[current_task] = {
                    "file_path": current_file,
                    "lines": current_lines,
                }

            current_task = int(match.group(1))
            current_file = match.group(3).strip()
            current_lines = [line]
        elif current_task is not None:
            current_lines.append(line)

    if current_task is not None:
        tasks[current_task] = {
            "file_path": current_file,
            "lines": current_lines,
        }

    final_tasks = []

    for _, task_data in tasks.items():
        lines = task_data["lines"]
        warnings = _extract_warnings(lines)

        if not warnings:
            continue

        line_count = len(lines)
        source_file = task_data["file_path"]
        unique_headers = set(Path(warning["file"]).name for warning in warnings)

        if line_count > LARGE_TASK_THRESHOLD and len(unique_headers) > 1:
            header_groups = _group_by_header(lines, warnings)

            for header_name, (header_lines, header_warnings) in header_groups.items():
                if header_warnings:
                    content = _create_task_content(source_file, header_lines, header_warnings, header_name)
                    final_tasks.append({
                        "source_file": source_file,
                        "sub_header": header_name,
                        "content": content,
                        "size": len(content),
                        "warning_count": len(header_warnings),
                    })
        else:
            content = _create_task_content(source_file, lines, warnings)
            final_tasks.append({
                "source_file": source_file,
                "sub_header": None,
                "content": content,
                "size": len(content),
                "warning_count": len(warnings),
            })

    final_tasks.sort(key=lambda item: item["size"])

    created_count = 0
    for idx, task_data in enumerate(final_tasks, start=1):
        task_id = f"{idx:03d}"
        task_file = output_dir / f"task_{task_id}.log"
        task_file.write_text(task_data["content"], encoding="utf-8")
        created_count += 1

    return created_count
