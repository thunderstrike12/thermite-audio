import os
import json
import subprocess

# Directory containing your codebase (change this to your actual directory)
CODEBASE_DIR = "."

# File extensions to format
FILE_EXTENSIONS = (".h", ".hpp", ".c", ".cpp", ".cc", ".cxx")

# Directories or paths to ignore
IGNORE_PATHS = {
    "extern",
    "build"
}

# File to store the last-modified timestamps
CACHE_FILE = ".format_cache.json"


def load_cache():
    """Load the cached file modification times from disk."""
    if os.path.exists(CACHE_FILE):
        with open(CACHE_FILE, "r") as f:
            return json.load(f)
    return {}


def save_cache(cache):
    """Save the file modification times to disk."""
    with open(CACHE_FILE, "w") as f:
        json.dump(cache, f, indent=2)


def should_ignore_file(file_path):
    """Check if the file should be ignored based on IGNORE_PATHS."""
    for ignore_path in IGNORE_PATHS:
        if ignore_path in file_path:
            return True
    return False


def find_files(directory, extensions):
    """Recursively find all files with the given extensions, ignoring specified paths."""
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
        return True
    except subprocess.CalledProcessError as e:
        print(f"Failed to format {file_path}: {e}")
        return False


def main():
    cache = load_cache()
    updated_cache = {}
    formatted_count = 0
    skipped_count = 0

    for file_path in find_files(CODEBASE_DIR, FILE_EXTENSIONS):
        # Normalize path so cache keys are consistent across runs
        norm_path = os.path.normpath(file_path)
        last_modified = os.path.getmtime(norm_path)

        if cache.get(norm_path) == last_modified:
            skipped_count += 1
            updated_cache[norm_path] = last_modified  # keep it in the cache
            continue

        success = run_clang_format(norm_path)

        if success:
            # Record the mtime AFTER formatting so the next run skips it
            updated_cache[norm_path] = os.path.getmtime(norm_path)
            formatted_count += 1
        else:
            # On failure, don't cache — retry next time
            updated_cache[norm_path] = cache.get(norm_path)

    save_cache(updated_cache)
    print(f"\nFormatting complete! {formatted_count} formatted, {skipped_count} skipped.")


if __name__ == "__main__":
    main()