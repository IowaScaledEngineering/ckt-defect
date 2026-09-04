import argparse
import re
import sys

class TreeNode:
    def __init__(self, name):
        self.name = name
        self.children = []

def parse_menu_structure(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # Map variable names to menu display names
    # e.g., auto home = std::make_shared<MenuHome>("Home", ...);
    var_to_name = {}
    pattern_create = re.compile(r'auto\s+(\w+)\s*=\s*std::make_shared<[^>]+>\(\s*"([^"]+)"')
    
    for var_name, menu_name in pattern_create.findall(content):
        var_to_name[var_name] = menu_name

    # Build hierarchy via addChild relationships
    # e.g., mainSel->addChild(menuMilepostConfig);
    nodes = {var: TreeNode(name) for var, name in var_to_name.items()}
    has_parent = set()

    pattern_add = re.compile(r'(\w+)->addChild\((\w+)\);')
    for parent_var, child_var in pattern_add.findall(content):
        if parent_var in nodes and child_var in nodes:
            nodes[parent_var].children.append(nodes[child_var])
            has_parent.add(child_var)

    # Find the root node(s)
    root_candidates = [var for var in nodes if var not in has_parent]
    return [nodes[var] for var in root_candidates]

def build_ascii_tree(node, prefix="", is_last=True):
    lines = []
    connector = "└── " if is_last else "├── "
    lines.append(prefix + (connector if prefix else "") + node.name)
    
    child_prefix = prefix + ("    " if is_last else "│   ")
    for i, child in enumerate(node.children):
        is_child_last = (i == len(node.children) - 1)
        lines.extend(build_ascii_tree(child, child_prefix, is_child_last))
    
    return lines

def main():
    parser = argparse.ArgumentParser(description="Parse a C++ menu file and generate an ASCII tree structure.")
    parser.add_argument("-i", "--input", required=True, help="Path to the C++ source file to parse (required)")
    parser.add_argument("-o", "--output", help="Path to the output file (optional; prints to stdout if omitted)")

    args = parser.parse_args()

    try:
        roots = parse_menu_structure(args.input)
    except FileNotFoundError:
        print(f"Error: The file '{args.input}' was not found.", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Error parsing file: {e}", file=sys.stderr)
        sys.exit(1)

    tree_lines = []
    for root in roots:
        tree_lines.extend(build_ascii_tree(root))

    output_text = "\n".join(tree_lines)

    if args.output:
        try:
            with open(args.output, 'w', encoding='utf-8') as f:
                f.write(output_text + "\n")
            print(f"Tree structure successfully written to '{args.output}'")
        except Exception as e:
            print(f"Error writing to output file: {e}", file=sys.stderr)
            sys.exit(1)
    else:
        print(output_text)

if __name__ == "__main__":
    main()