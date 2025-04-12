import graphviz
import argparse
import os

def visualize_dot_file(dot_filepath, output_format='png'):
    """
    Reads a DOT file and renders it to an image file.

    Args:
        dot_filepath (str): Path to the input DOT file.
        output_format (str): Output image format (e.g., 'png', 'svg', 'pdf').
    """
    if not os.path.exists(dot_filepath):
        print(f"Error: DOT file not found at {dot_filepath}")
        return

    try:
        # Create a Graphviz source object from the DOT file
        s = graphviz.Source.from_file(dot_filepath)

        # Determine the output filename (same base name as input, different extension)
        output_filename = os.path.splitext(dot_filepath)[0]

        # Render the graph to a file (e.g., 'output_tree.png')
        # The view=False argument prevents automatically opening the file.
        rendered_path = s.render(filename=output_filename, format=output_format, view=False, cleanup=True)
        print(f"Tree visualization saved to: {rendered_path}")

    except Exception as e:
        print(f"Error rendering DOT file: {e}")
        print("Please ensure Graphviz is installed and in your system's PATH.")
        print("You might need to install it using your package manager (e.g., 'sudo apt install graphviz' or 'brew install graphviz')")
        print("And the Python library: 'pip install graphviz'")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Visualize a tree structure from a DOT file.")
    parser.add_argument("dot_file", help="Path to the input DOT file.")
    parser.add_argument("-f", "--format", default="png", help="Output image format (e.g., png, svg, pdf). Default: png")

    args = parser.parse_args()

    visualize_dot_file(args.dot_file, args.format)

# Example usage from the command line:
# python scripts/visualize_tree.py path/to/your/tree_output.dot -f png