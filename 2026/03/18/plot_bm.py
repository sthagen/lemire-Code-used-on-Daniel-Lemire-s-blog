import matplotlib.pyplot as plt
import re

plt.rcParams.update({'font.size': 20})

def parse_file(filename):
    sizes = []
    bm_values = []
    with open(filename, 'r') as f:
        for line in f:
            # Match lines like: 1000                                               :  0.438 ns   2.28 Gv/s   0.00 bm
            match = re.match(r'(\d+)\s+:\s+[\d.]+\s+ns\s+[\d.]+\s+Gv/s\s+([\d.]+)\s+bm', line.strip())
            if match:
                size = int(match.group(1))
                bm = float(match.group(2))
                sizes.append(size)
                bm_values.append(bm)
    return sizes, bm_values

# Parse the files
m4_sizes, m4_bm = parse_file('m4.txt')
intel_sizes, intel_bm = parse_file('intel.txt')
amd_sizes, amd_bm = parse_file('amd.txt')

# Plot
plt.figure(figsize=(10, 6))
ax = plt.gca()
ax.spines['top'].set_visible(False)
ax.spines['right'].set_visible(False)
plt.plot(amd_sizes, amd_bm, label='AMD Zen 5', marker='^')
plt.plot(m4_sizes, m4_bm, label='Apple M4 Max', marker='o')
plt.plot(intel_sizes, intel_bm, label='Intel Emerald Rapids', marker='s')

plt.xlabel('Input Size')
plt.ylabel('Branch Mispredictions per Value (bm)')
plt.legend(frameon=False)
plt.xscale('log')  # Since sizes increase exponentially
plt.tight_layout()
plt.savefig('branch_mispredictions.png')
