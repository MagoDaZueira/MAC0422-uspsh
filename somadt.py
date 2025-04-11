import sys

def sum_third_column(filename):
    total = 0.0
    with open(filename, 'r') as file:
        for line_num, line in enumerate(file, start=1):
            parts = line.strip().split()
            if len(parts) < 3:
                print(f"Skipping line {line_num}: not enough columns")
                continue
            try:
                value = float(parts[2])  # index 2 = 3rd column
                total += value
            except ValueError:
                print(f"Skipping line {line_num}: could not convert '{parts[2]}' to float")
    print(f"Total sum of the 3rd column: {total}")

# Example usage:
# sum_third_column('back.txt')
sum_third_column(sys.argv[1])
