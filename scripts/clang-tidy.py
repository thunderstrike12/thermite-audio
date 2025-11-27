import subprocess
import pathlib
import time
import argparse
from collections import defaultdict
import re

# Define the file extensions that clang-tidy should check
EXTENSION = ['.c', '.cpp', '.cc', '.h', '.hpp']
# Define your clang-tidy command and options
COMMAND = 'clang-tidy'
PROFILE_ENABLE = "--enable-check-profile"
COMPILE_DATABASE = 'build/Debug-Editor'
OPTIONS = '--config-file=.clang-tidy'
# Default directories to check
DEFAULT_DIRECTORIES = ['engine', 'projects']

def glob_recursive_files(path, extensions):
    ret = []
    path_obj = pathlib.Path(path)
    for file_path in path_obj.rglob("*"):
        if (file_path.is_file() 
            and file_path.suffix in extensions 
            and file_path.name != "pch.hpp"):
            ret.append(file_path)
    return ret

def parse_clang_tidy_output(output):
    """Parse clang-tidy output to extract meaningful errors/warnings"""
    issues = []
    lines = output.split('\n')
    
    for line in lines:
        # Match lines with file:line:col: severity: message
        match = re.match(r'^(.+?):(\d+):(\d+):\s+(warning|error|note):\s+(.+)$', line)
        if match:
            file_path, line_num, col, severity, message = match.groups()
            issues.append({
                'file': file_path,
                'line': line_num,
                'col': col,
                'severity': severity,
                'message': message
            })
    
    return issues

def format_output(issues, summary=True):
    """Format issues into a readable report"""
    if not issues:
        return "No issues found!\n"
    
    # Group by file
    by_file = defaultdict(list)
    severity_count = defaultdict(int)
    
    for issue in issues:
        by_file[issue['file']].append(issue)
        severity_count[issue['severity']] += 1
    
    output = []
    output.append("=" * 80)
    output.append("CLANG-TIDY REPORT")
    output.append("=" * 80)
    output.append("")
    
    # Print issues by file
    for file_path in sorted(by_file.keys()):
        file_issues = by_file[file_path]
        output.append(f"\n{file_path}")
        output.append("-" * 80)
        
        for issue in file_issues:
            severity_marker = "⚠️ " if issue['severity'] == 'warning' else "❌"
            output.append(f"  {severity_marker} Line {issue['line']}:{issue['col']} - {issue['severity'].upper()}")
            output.append(f"     {issue['message']}")
            output.append("")
    
    # Summary
    if summary:
        output.append("\n" + "=" * 80)
        output.append("SUMMARY")
        output.append("=" * 80)
        output.append(f"Total files checked: {len(by_file)}")
        output.append(f"Errors: {severity_count.get('error', 0)}")
        output.append(f"Warnings: {severity_count.get('warning', 0)}")
        output.append("")
    
    return "\n".join(output)

def run_clang_tidy_on_files(files, output_file=None, quiet=False):
    command = [COMMAND, '-p=' + COMPILE_DATABASE, OPTIONS, '--quiet'] + files
    
    if not quiet:
        print(f"Running clang-tidy on {len(files)} files...")
        print("This may take a while...\n")
    
    try:
        result = subprocess.run(
            command,
            capture_output=True,
            text=True,
            check=False
        )
        
        # Combine stdout and stderr
        full_output = result.stdout + result.stderr
        
        # Parse output
        issues = parse_clang_tidy_output(full_output)
        
        # Format output
        formatted = format_output(issues)
        
        # Print to console
        if not quiet:
            print(formatted)
        
        # Save to file if requested
        if output_file:
            with open(output_file, 'w', encoding='utf-8') as f:
                f.write(formatted)
                f.write("\n\n" + "=" * 80 + "\n")
                f.write("RAW OUTPUT\n")
                f.write("=" * 80 + "\n")
                f.write(full_output)
            
            if not quiet:
                print(f"\nFull report saved to: {output_file}")
        
        return len([i for i in issues if i['severity'] == 'error'])
        
    except subprocess.CalledProcessError as e:
        print(f"Error running clang-tidy: {e}")
        return -1

def main():
    parser = argparse.ArgumentParser(description='Tidy directories of source files')
    parser.add_argument('-d', '--directory', help="Directories to Tidy (default: engine, projects)", type=str, nargs='*')
    parser.add_argument('-f', '--files', help="Files to Tidy", type=str, nargs='*')
    parser.add_argument('-o', '--output', help='Output file for results (default: tidy_results/report.txt)', type=str, default='tidy_results/report.txt')
    parser.add_argument('-q', '--quiet', help='Suppress console output', action='store_true')
    parser.add_argument('-p', '--profile', help='Specify output directory to check profile', type=str)
    
    args = parser.parse_args()
    
    start_time = time.time()
    
    files = []
    # Use default directories if none specified
    directories_to_check = args.directory if args.directory else DEFAULT_DIRECTORIES
    
    for directory in directories_to_check:
        files += glob_recursive_files(directory, EXTENSION)
    
    if args.files:
        files += args.files
    
    if not files:
        print("No files found to check!")
        return
    
    if not args.quiet:
        print(f"Checking directories: {', '.join(directories_to_check)}")
    
    # Ensure output directory exists
    if args.output:
        pathlib.Path(args.output).parent.mkdir(parents=True, exist_ok=True)
    
    error_count = run_clang_tidy_on_files(files, args.output, args.quiet)
    
    end_time = time.time()
    elapsed_time = end_time - start_time
    
    if not args.quiet:
        print(f"\nClang-tidy took {elapsed_time:.2f} seconds")
        print(f"Average of {(elapsed_time / len(files)):.2f}s per file")
        
        if error_count > 0:
            print(f"\n⚠️  Found {error_count} error(s) that need attention")

if __name__ == "__main__":
    main()