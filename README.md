# Huffman Command Encoder

This project implements a character-level Huffman encoder for Formula SAE command strings. It includes the original C implementation and a Python simulation script for case-sensitivity analysis.

## Project Structure
- `huffman_commands.c`: The core C implementation of the Huffman encoder.
- `compare_strings.py`: A Python script to compare Mixed Case vs. Lowercase Huffman encoding efficiency.

## How to Run

### 1. C Encoder
To compile and run the Huffman encoder in C:
```bash
gcc huffman_commands.c -o huffman_commands
./huffman_commands
```

### 2. Python Comparison Analysis
To run the analysis script that compares mixed-case vs. lowercase encoding:
```bash
python3 compare_strings.py
```

## Compression Benefits
Switching to a **lowercase only** character set reduces unique characters from 40 to 24, resulting in a **~15% reduction** in bit size and ensuring all commands fit safely within a 32-bit (4-byte) limit.
