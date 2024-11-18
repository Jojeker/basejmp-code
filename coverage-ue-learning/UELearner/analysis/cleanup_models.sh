#!/bin/bash

# Directory containing the models
MODEL_DIR="../models"

for file in "$MODEL_DIR"/*.dot; do
    # Skip already processed files
    if [[ "$file" == *_cl.dot ]]; then
        continue
    fi

    base_name=$(basename "$file" .dot)

    # Get Operands
    input_file="$file"
    output_file="${MODEL_DIR}/${base_name}_cl.dot"
    pdf_file="${MODEL_DIR}/${base_name}_cl.pdf"

    # Run cleanup_model.py and generate PDF
    DOT_ID=1 ./cleanup_model.py "$input_file" "$output_file" &&
    dot -Tpdf "$output_file" > "$pdf_file"
done
