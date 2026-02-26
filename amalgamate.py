import os
import re

def get_files(directory, ext):
    file_list = []
    for root, _, files in os.walk(directory):
        for f in files:
            if f.endswith(ext):
                file_list.append(os.path.join(root, f))
    return file_list

def resolve_includes(filepath, root_dir, processed_files, processed_includes, output_lines):
    # Absolute path handling to prevent duplicate inclusions
    abs_path = os.path.abspath(filepath)
    if abs_path in processed_files:
        return
    processed_files.add(abs_path)

    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            lines = f.readlines()
    except Exception as e:
        print(f"Error reading {filepath}: {e}")
        return

    output_lines.append(f"// --- Begin: {os.path.relpath(filepath, root_dir)} ---\n")

    for line in lines:
        line_clean = line.strip()
        
        # Handle internal #include "vertel/..."
        if line_clean.startswith('#include "vertel/') or line_clean.startswith('#include "nlohmann/'):
            # Extract the included path
            match = re.search(r'#include\s+"([^"]+)"', line_clean)
            if match:
                include_path = match.group(1)
                
                # Special handling for nlohmann/json.hpp (we don't inline it if it's external,
                # but if it exists in third_party, we should inline it)
                if include_path == "nlohmann/json.hpp":
                    full_include_path = os.path.join(root_dir, "third_party", "nlohmann", "json.hpp")
                    if os.path.exists(full_include_path):
                        resolve_includes(full_include_path, root_dir, processed_files, processed_includes, output_lines)
                    else:
                        # Otherwise leave it as a regular include (hoping user has it)
                        if line_clean not in processed_includes:
                            processed_includes.add(line_clean)
                            output_lines.append(line)
                    continue

                # Normal vertel includes
                full_include_path = os.path.join(root_dir, "include", include_path)
                if os.path.exists(full_include_path):
                    # Recursively resolve and inline this header
                    resolve_includes(full_include_path, root_dir, processed_files, processed_includes, output_lines)
                else:
                    output_lines.append(f"// WARNING: Could not find internal include: {include_path}\n")
            continue

        # Handle #pragma once
        elif line_clean == "#pragma once":
            continue
            
        # Handle system includes #include <...>
        elif line_clean.startswith('#include <'):
            if line_clean not in processed_includes:
                processed_includes.add(line_clean)
                output_lines.append(line)
        else:
            output_lines.append(line)
            
    output_lines.append(f"// --- End: {os.path.relpath(filepath, root_dir)} ---\n\n")

def amalgamate(project_root, output_file):
    include_dir = os.path.join(project_root, "include")
    src_dirs = [
        os.path.join(project_root, "core", "src"),
        os.path.join(project_root, "adapters", "src"),
        os.path.join(project_root, "runtime", "src"),
        os.path.join(project_root, "platform", "src")
    ]

    # We need a primary entry point to start the header resolution so that dependencies are
    # resolved in the correct order (bottom-up).
    # A safe bet is to find all headers and start resolving them. Since `resolve_includes` 
    # protects against multiple inclusions and resolves dependencies first, the order in which 
    # we feed the initial headers doesn't matter as much, but we still ensure they are all hit.
    header_files = get_files(include_dir, ".hpp")
    
    source_files = []
    for d in src_dirs:
        source_files.extend(get_files(d, ".cpp"))

    processed_files = set()
    processed_includes = set()
    
    output_lines = [
        "// ============================================================================\n",
        "// VerTel Amalgamated Single-Header\n",
        "// Generated to provide drop-in integration.\n",
        "//\n",
        "// USAGE:\n",
        "// In exactly *ONE* source file (.cpp), define VERTEL_IMPLEMENTATION before\n",
        "// including this file to compile the implementation:\n",
        "//   #define VERTEL_IMPLEMENTATION\n",
        "//   #include \"vertel.hpp\"\n",
        "//\n",
        "// In all other files, just #include \"vertel.hpp\"\n",
        "// ============================================================================\n\n",
        "#ifndef VERTEL_HPP\n",
        "#define VERTEL_HPP\n\n"
    ]

    # PASS 1: Headers (Declarations - Resolved Topologically)
    output_lines.append("// " + "="*60 + "\n")
    output_lines.append("//  HEADER DECLARATIONS\n")
    output_lines.append("// " + "="*60 + "\n\n")

    # Prevent Windows.h from defining min and max macros
    output_lines.append("#ifdef max\n")
    output_lines.append("#undef max\n")
    output_lines.append("#endif\n")
    output_lines.append("#ifdef min\n")
    output_lines.append("#undef min\n")
    output_lines.append("#endif\n")
    output_lines.append("#ifndef NOMINMAX\n")
    output_lines.append("#define NOMINMAX\n")
    output_lines.append("#endif\n\n")

    for h in header_files:
        resolve_includes(h, project_root, processed_files, processed_includes, output_lines)

    output_lines.append("\n#endif // VERTEL_HPP\n\n")

    # PASS 2: Source Files (Implementations)
    output_lines.append("#ifdef VERTEL_IMPLEMENTATION\n\n")
    output_lines.append("// " + "="*60 + "\n")
    output_lines.append("//  IMPLEMENTATION\n")
    output_lines.append("// " + "="*60 + "\n\n")

    output_lines.append("#ifndef VERTEL_HAS_LIBCURL\n")
    output_lines.append("#define VERTEL_HAS_LIBCURL 0\n")
    output_lines.append("#endif\n\n")

    for s in source_files:
        resolve_includes(s, project_root, processed_files, processed_includes, output_lines)

    output_lines.append("\n#endif // VERTEL_IMPLEMENTATION\n")

    with open(output_file, 'w', encoding='utf-8') as f:
        f.writelines(output_lines)

    print(f"Amalgamation complete! Processed {len(processed_files)} files into {output_file}")

if __name__ == "__main__":
    import pathlib
    project_root = str(pathlib.Path.cwd())
    output_file = os.path.join(project_root, "vertel.hpp")
    amalgamate(project_root, output_file)
