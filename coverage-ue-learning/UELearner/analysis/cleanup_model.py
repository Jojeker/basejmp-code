#! /usr/bin/env python

import networkx as nx
import sys
from networkx.drawing.nx_pydot import read_dot, write_dot

def transform_dot_file(input_dot_file, output_dot_file):
    # Read DOT file into a NetworkX graph
    graph = read_dot(input_dot_file)

    # Create a dictionary to store labels for each state
    label_dict = {}

    # Iterate over all edges and merge different inputs going to same outputs
    for edge in graph.edges(data=True):
        source = edge[0]
        target = edge[1]
        label = edge[2].get('label', '').replace('"','')

        # Otherwise we miss the `__start`
        if label.find("/") != -1:
            io = label.split("/")
            input = io[0].strip()
            output = io[1].strip()

            if source not in label_dict:
                label_dict[source] = {}
            if target not in label_dict[source]:
                label_dict[source][target] = {}
            if output not in label_dict[source][target]:
                label_dict[source][target][output] = list()

            label_dict[source][target][output].append(input)
        
    print(label_dict)

    # Create a new graph for the modified self-loops
    modified_graph = nx.MultiDiGraph()

    # Add consolidated self-loops with combined labels to the new graph
    for source, targets in label_dict.items():
        for target, output_and_inputs in targets.items():
            for output, inputs in output_and_inputs.items():
                combined_label = "{" + ", ".join(inputs) + "} / " + output
                print(f"adding from {source} => {target} [{combined_label}]")
                modified_graph.add_edge(source, target, label=combined_label)

    # Write the modified graph to the output DOT file
    write_dot(modified_graph, output_dot_file)

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python script.py <source.dot> <dest.dot>")
        sys.exit(1)
    input_dot_file = sys.argv[1]
    output_dot_file = sys.argv[2]

    transform_dot_file(input_dot_file, output_dot_file)
