#! /usr/bin/env python

import json
import gzip
import matplotlib.pyplot as plt
import argparse
import fnmatch

# zcat in python
def load_json_data(json_file):
    with gzip.open(json_file, 'rt', encoding='utf-8') as f:
        data = json.load(f)
    return data

# Extract the name and execution count from gcov
# Calls "jq '.files[].functions[] | { name: .demangled_name, executed: .execution_count}'
# mealy_gcov.json" in a more elaborate way
def extract_function_coverage(data):
    functions_data = []
    for file in data.get("files", []):
        for func in file.get("functions", []):
            # Extracting function name and execution count
            functions_data.append({
                "name": func.get("demangled_name", "Unknown"),
                "executed": func.get("execution_count", 0)
            })
    return functions_data


# Visualize the extracted coverage data
def plot_coverage(functions_data, file):
    # Extract function names and execution counts
    function_names = [func['name'] for func in functions_data]
    execution_counts = [func['executed'] for func in functions_data]
    
    # Create a bar chart of function coverage
    plt.figure(figsize=(10, 6))
    plt.barh(function_names, execution_counts, color='skyblue')
    plt.xlabel('Execution Count')
    plt.title('Function Coverage')
    plt.title(f'Statistics for compilation unit "{file}"')
    plt.tight_layout()
    plt.show()


# def filter_functions(functions_data, filter_names):
#     # Ignore empty filter
#     if filter_names != []:
#         # Filter only the functions whose name matches one in the filter list
#         return [func for func in functions_data if func['name'] in filter_names]
#     return functions_data


def filter_functions(functions_data, patterns):
    # Empty filter means we want all
    if not patterns:
        return functions_data  
    
    # Filter functions for any provided glob patterns
    filtered = []
    for func in functions_data:
        for pattern in patterns:
            if fnmatch.fnmatch(func['name'], pattern):
                filtered.append(func)
                break  # No need to check other patterns once matched
    return filtered


# Main execution
if __name__ == '__main__':
    parser = argparse.ArgumentParser(prog='gcov_analyzer', 
                                     description='Visualize the coverage information of a file')
    parser.add_argument("-f", "--file", required=True)
    parser.add_argument('-t', '--functions', type=str, nargs='*', default=[], 
                        help="List of function names to visualize") 

    args = parser.parse_args()

    # Extract coverage data from the JSON file using jq
    functions_data = load_json_data(args.file)
    extracted_cov = extract_function_coverage(functions_data)
    filtered_cov = filter_functions(extracted_cov, args.functions)

    # Prepare the filename
    compilation_unit_name = args.file.split('.')[0] + ".o"


    # Plot the data
    plot_coverage(filtered_cov, compilation_unit_name)

