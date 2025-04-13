import configparser
import random
import sys
import os

def generate_random_hypergraph(config_file):
    """
    Generates a random hypergraph based on parameters in a config file.

    Args:
        config_file (str): Path to the configuration file.
    """
    config = configparser.ConfigParser()
    if not os.path.exists(config_file):
        print(f"Error: Configuration file '{config_file}' not found.")
        sys.exit(1)

    try:
        config.read(config_file)
        params = config['Hypergraph']

        num_vertices = int(params['num_vertices'])
        num_hyperedges = int(params['num_hyperedges'])
        min_edge_size = int(params['min_edge_size'])
        max_edge_size = int(params['max_edge_size'])
        output_file = params['output_file']

        # Basic validation
        if num_vertices <= 0 or num_hyperedges <= 0 or min_edge_size <= 0:
            print("Error: Number of vertices, hyperedges, and min_edge_size must be positive.")
            sys.exit(1)
        if min_edge_size > max_edge_size:
            print("Error: min_edge_size cannot be greater than max_edge_size.")
            sys.exit(1)
        if max_edge_size > num_vertices:
            print("Warning: max_edge_size is greater than num_vertices. Setting max_edge_size to num_vertices.")
            max_edge_size = num_vertices

    except KeyError as e:
        print(f"Error: Missing configuration parameter: {e}")
        sys.exit(1)
    except ValueError as e:
        print(f"Error: Invalid numerical value in configuration: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"Error reading configuration file: {e}")
        sys.exit(1)

    print("Generating hypergraph with the following parameters:")
    print(f"  Vertices: {num_vertices}")
    print(f"  Hyperedges: {num_hyperedges}")
    print(f"  Edge Size Range: [{min_edge_size}, {max_edge_size}]")
    print(f"  Output File: {output_file}")

    vertices = list(range(num_vertices))
    hyperedges = []

    for _ in range(num_hyperedges):
        # Determine the size of the current hyperedge
        current_edge_size = random.randint(min_edge_size, max_edge_size)
        # Sample vertices without replacement
        hyperedge = random.sample(vertices, current_edge_size)
        hyperedges.append(hyperedge)

    # Write to output file
    try:
        with open(output_file, 'w') as f:
            for edge in hyperedges:
                # Convert vertex IDs to strings and join with spaces
                f.write(' '.join(map(str, edge)) + '\n')
        print(f"Successfully generated hypergraph and saved to '{output_file}'")
    except IOError as e:
        print(f"Error writing to output file '{output_file}': {e}")
        sys.exit(1)

if __name__ == "__main__":

    config_path = '/root/ReachCompress/config.ini'
    generate_random_hypergraph(config_path)
