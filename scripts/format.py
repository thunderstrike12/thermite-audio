import os
import subprocess
#asked deepseek to create this script
# Directory containing your codebase (change this to your actual directory)
CODEBASE_DIR = "."

# File extensions to format
FILE_EXTENSIONS = (".h", ".hpp", ".c", ".cpp", ".cc", ".cxx")

# Directories or paths to ignore
IGNORE_PATHS = {
    "extern",  # Ignore the "external" directory
    "build"
}

def should_ignore_file(file_path):
    """Check if the file should be ignored based on the IGNORE_PATHS."""
    for ignore_path in IGNORE_PATHS:
        if ignore_path in file_path:
            return True
    return False

def find_files(directory, extensions):
    """Recursively find all files with the given extensions in the directory, ignoring specified paths."""
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(extensions):
                file_path = os.path.join(root, file)
                if not should_ignore_file(file_path):
                    yield file_path

def run_clang_format(file_path):
    """Run clang-format on the specified file."""
    try:
        subprocess.run(["clang-format", "-i", file_path], check=True)
        print(f"Formatted: {file_path}")
    except subprocess.CalledProcessError as e:
        print(f"Failed to format {file_path}: {e}")

def main():
    # Find all C/C++ files in the codebase, ignoring specified paths
    files_to_format = find_files(CODEBASE_DIR, FILE_EXTENSIONS)

    # Run clang-format on each file
    for file_path in files_to_format:
        run_clang_format(file_path)

    print("Formatting complete!")

if __name__ == "__main__":
    main()