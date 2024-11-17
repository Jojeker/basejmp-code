#! /usr/bin/env python

import re
from collections import defaultdict

# Parse logs and check for inconsistencies
def check_inconsistencies(logs):
    # Dictionary to store output mappings
    input_mapping = defaultdict(list)
    
    # Regular expression to parse log lines
    log_pattern = re.compile(r"I: (.*), O: (.*)")
    
    for line in logs:
        match = log_pattern.search(line)
        if match:
            # Get groups for Input and output sequences
            input_seq = match.group(1).strip().replace(" ", "")
            output_seq = match.group(2).strip().replace(" ", "")

            # Check for inconsistency:
            # Is a prefix of the word, we look at already mapped
            # differently?
            for i in range(1, len(input_seq) + 1):
                input_seq_trunc = input_seq[:i]
                output_seq_trunc = output_seq[:i]

                if input_seq_trunc in input_mapping:
                    for mapped_input in input_mapping[input_seq_trunc]:
                        if mapped_input != output_seq_trunc:
                            print(f"[!] Inconsistency: \t\t'{input_seq}'\t'{input_seq_trunc}'")
                            print(f"[!] Maps to\t\t\t'{output_seq}'\t'{output_seq_trunc}'")
                            print(f"[!] But '{input_seq_trunc}' ->\t\t\t'{mapped_input}'")
                            exit(-1)
                else:
                    input_mapping[input_seq_trunc].append(output_seq_trunc)

    print("Check complete. No inconsistency found!")


file = open("../logs/application.log", "rt")

check_inconsistencies(file.readlines())

