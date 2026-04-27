import matplotlib.pyplot as plt
import re

def parse_data(filename):
    sizes = []
    binary_cold = []
    binary_warm = []
    find_cold = []
    find_warm = []
    simd_quad_cold = []
    simd_quad_warm = []
    simd_binary_cold = []
    simd_binary_warm = []

    with open(filename, 'r') as f:
        lines = f.readlines()

    current_size = None
    for line in lines:
        if line.startswith('arrays:'):
            match = re.search(r'size per array: (\d+)', line)
            if match:
                current_size = int(match.group(1))
                sizes.append(current_size)
        elif line.strip().startswith('binary_search'):
            if 'cold' in line:
                match = re.search(r'cold :\s+([\d.]+) ns', line)
                if match:
                    binary_cold.append(float(match.group(1)))
            elif 'warm' in line:
                match = re.search(r'warm :\s+([\d.]+) ns', line)
                if match:
                    binary_warm.append(float(match.group(1)))
        elif line.strip().startswith('find'):
            if 'cold' in line:
                match = re.search(r'cold :\s+([\d.]+) ns', line)
                if match:
                    find_cold.append(float(match.group(1)))
            elif 'warm' in line:
                match = re.search(r'warm :\s+([\d.]+) ns', line)
                if match:
                    find_warm.append(float(match.group(1)))
        elif line.strip().startswith('simd_quad'):
            if 'cold' in line:
                match = re.search(r'cold :\s+([\d.]+) ns', line)
                if match:
                    simd_quad_cold.append(float(match.group(1)))
            elif 'warm' in line:
                match = re.search(r'warm :\s+([\d.]+) ns', line)
                if match:
                    simd_quad_warm.append(float(match.group(1)))
        elif line.strip().startswith('simd_binary'):
            if 'cold' in line:
                match = re.search(r'cold :\s+([\d.]+) ns', line)
                if match:
                    simd_binary_cold.append(float(match.group(1)))
            elif 'warm' in line:
                match = re.search(r'warm :\s+([\d.]+) ns', line)
                if match:
                    simd_binary_warm.append(float(match.group(1)))

    return sizes, binary_cold, binary_warm, find_cold, find_warm, simd_quad_cold, simd_quad_warm, simd_binary_cold, simd_binary_warm

def plot_comparison(sizes, binary_cold, binary_warm, find_cold, find_warm, suffix):
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

    # Cold cache
    ax1.plot(sizes, binary_cold, label='binary_search', marker='o', color='blue')
    ax1.plot(sizes, find_cold, label='find', marker='s', color='orange')
    ax1.set_xlabel('Size per array')
    ax1.set_ylabel('Time (ns)')
    ax1.set_title('Cold Cache Performance')
    ax1.legend(frameon=False)
    ax1.grid(True)
    ax1.spines['top'].set_visible(False)
    ax1.spines['right'].set_visible(False)

    # Warm cache
    ax2.plot(sizes, binary_warm, label='binary_search', marker='o', color='blue')
    ax2.plot(sizes, find_warm, label='find', marker='s', color='orange')
    ax2.set_xlabel('Size per array')
    ax2.set_ylabel('Time (ns)')
    ax2.set_title('Warm Cache Performance')
    ax2.legend(frameon=False)
    ax2.grid(True)
    ax2.spines['top'].set_visible(False)
    ax2.spines['right'].set_visible(False)

    # Set same y-axis scale
    all_times = binary_cold + binary_warm + find_cold + find_warm
    ymin, ymax = min(all_times), max(all_times)
    ax1.set_ylim(ymin, ymax)
    ax2.set_ylim(ymin, ymax)

    plt.tight_layout()
    plt.savefig(f'comparison_plot_{suffix}.png', dpi=300)

def plot_binary_vs_simd(sizes, binary_cold, binary_warm, simd_cold, simd_warm, suffix):
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

    # Cold cache
    ax1.plot(sizes, binary_cold, label='binary_search', marker='o', color='blue')
    ax1.plot(sizes, simd_cold, label='simd_quad', marker='s', color='red')
    ax1.set_xlabel('Size per array')
    ax1.set_ylabel('Time (ns)')
    ax1.set_title('Cold Cache: binary_search vs simd_quad')
    ax1.legend(frameon=False)
    ax1.grid(True)
    ax1.spines['top'].set_visible(False)
    ax1.spines['right'].set_visible(False)

    # Warm cache
    ax2.plot(sizes, binary_warm, label='binary_search', marker='o', color='blue')
    ax2.plot(sizes, simd_warm, label='simd_quad', marker='s', color='red')
    ax2.set_xlabel('Size per array')
    ax2.set_ylabel('Time (ns)')
    ax2.set_title('Warm Cache: binary_search vs simd_quad')
    ax2.legend(frameon=False)
    ax2.grid(True)
    ax2.spines['top'].set_visible(False)
    ax2.spines['right'].set_visible(False)

    # Set same y-axis scale
    all_times = binary_cold + binary_warm + simd_cold + simd_warm
    ymin, ymax = min(all_times), max(all_times)
    ax1.set_ylim(ymin, ymax)
    ax2.set_ylim(ymin, ymax)

    plt.tight_layout()
    plt.savefig(f'binary_vs_simd_{suffix}.png', dpi=300)

def plot_three_comparison(sizes, binary_cold, binary_warm, quad_cold, quad_warm, bin_cold, bin_warm, suffix):
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

    # Cold cache
    ax1.plot(sizes, binary_cold, label='binary_search', marker='o', color='blue')
    ax1.plot(sizes, quad_cold, label='SIMD Quad', marker='^', color='green')
    ax1.plot(sizes, bin_cold, label='SIMD binary', marker='v', color='purple')
    ax1.set_xlabel('Size per array')
    ax1.set_ylabel('Time (ns)')
    ax1.set_title('Cold Cache: binary_search vs SIMD Quad vs SIMD binary')
    ax1.legend(frameon=False)
    ax1.grid(True)
    ax1.spines['top'].set_visible(False)
    ax1.spines['right'].set_visible(False)

    # Warm cache
    ax2.plot(sizes, binary_warm, label='binary_search', marker='o', color='blue')
    ax2.plot(sizes, quad_warm, label='SIMD Quad', marker='^', color='green')
    ax2.plot(sizes, bin_warm, label='SIMD binary', marker='v', color='purple')
    ax2.set_xlabel('Size per array')
    ax2.set_ylabel('Time (ns)')
    ax2.set_title('Warm Cache: binary_search vs SIMD Quad vs SIMD binary')
    ax2.legend(frameon=False)
    ax2.grid(True)
    ax2.spines['top'].set_visible(False)
    ax2.spines['right'].set_visible(False)

    # Set same y-axis scale
    all_times = binary_cold + binary_warm + quad_cold + quad_warm + bin_cold + bin_warm
    ymin, ymax = min(all_times), max(all_times)
    ax1.set_ylim(ymin, ymax)
    ax2.set_ylim(ymin, ymax)

    plt.tight_layout()
    plt.savefig(f'three_way_comparison_{suffix}.png', dpi=300)

def plot_quad_vs_binary(sizes, quad_cold, quad_warm, binary_cold, binary_warm, suffix):
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

    # Cold cache
    ax1.plot(sizes, quad_cold, label='SIMD Quad', marker='o', color='red')
    ax1.plot(sizes, binary_cold, label='SIMD binary', marker='s', color='purple')
    ax1.set_xlabel('Size per array')
    ax1.set_ylabel('Time (ns)')
    ax1.set_title('Cold Cache: SIMD Quad vs SIMD binary')
    ax1.legend(frameon=False)
    ax1.grid(True)
    ax1.spines['top'].set_visible(False)
    ax1.spines['right'].set_visible(False)

    # Warm cache
    ax2.plot(sizes, quad_warm, label='SIMD Quad', marker='o', color='red')
    ax2.plot(sizes, binary_warm, label='SIMD binary', marker='s', color='purple')
    ax2.set_xlabel('Size per array')
    ax2.set_ylabel('Time (ns)')
    ax2.set_title('Warm Cache: SIMD Quad vs SIMD binary')
    ax2.legend(frameon=False)
    ax2.grid(True)
    ax2.spines['top'].set_visible(False)
    ax2.spines['right'].set_visible(False)

    # Set same y-axis scale
    all_times = quad_cold + quad_warm + binary_cold + binary_warm
    ymin, ymax = min(all_times), max(all_times)
    ax1.set_ylim(ymin, ymax)
    ax2.set_ylim(ymin, ymax)

    plt.tight_layout()
    plt.savefig(f'quad_vs_binary_{suffix}.png', dpi=300)

def plot_four_comparison(sizes, binary_cold, binary_warm, find_cold, find_warm, quad_cold, quad_warm, bin_cold, bin_warm, suffix):
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

    # Cold cache
    ax1.plot(sizes, binary_cold, label='binary_search', marker='o', color='blue')
    ax1.plot(sizes, find_cold, label='find', marker='s', color='orange')
    ax1.plot(sizes, quad_cold, label='SIMD Quad', marker='^', color='green')
    ax1.plot(sizes, bin_cold, label='SIMD binary', marker='v', color='purple')
    ax1.set_xlabel('Size per array')
    ax1.set_ylabel('Time (ns)')
    ax1.set_title('Cold Cache: All Algorithms')
    ax1.legend(frameon=False)
    ax1.grid(True)
    ax1.spines['top'].set_visible(False)
    ax1.spines['right'].set_visible(False)

    # Warm cache
    ax2.plot(sizes, binary_warm, label='binary_search', marker='o', color='blue')
    ax2.plot(sizes, find_warm, label='find', marker='s', color='orange')
    ax2.plot(sizes, quad_warm, label='SIMD Quad', marker='^', color='green')
    ax2.plot(sizes, bin_warm, label='SIMD binary', marker='v', color='purple')
    ax2.set_xlabel('Size per array')
    ax2.set_ylabel('Time (ns)')
    ax2.set_title('Warm Cache: All Algorithms')
    ax2.legend(frameon=False)
    ax2.grid(True)
    ax2.spines['top'].set_visible(False)
    ax2.spines['right'].set_visible(False)

    # Set same y-axis scale
    all_times = binary_cold + binary_warm + find_cold + find_warm + quad_cold + quad_warm + bin_cold + bin_warm
    ymin, ymax = min(all_times), max(all_times)
    ax1.set_ylim(ymin, ymax)
    ax2.set_ylim(ymin, ymax)

    plt.tight_layout()
    plt.savefig(f'four_way_comparison_{suffix}.png', dpi=300)

if __name__ == '__main__':
    for filename, suffix in [('applem4.txt', 'apple'), ('intel.txt', 'intel')]:
        sizes, binary_cold, binary_warm, find_cold, find_warm, simd_quad_cold, simd_quad_warm, simd_binary_cold, simd_binary_warm = parse_data(filename)
        plot_comparison(sizes, binary_cold, binary_warm, find_cold, find_warm, suffix)
        plot_binary_vs_simd(sizes, binary_cold, binary_warm, simd_quad_cold, simd_quad_warm, suffix)
        plot_three_comparison(sizes, binary_cold, binary_warm, simd_quad_cold, simd_quad_warm, simd_binary_cold, simd_binary_warm, suffix)
        plot_quad_vs_binary(sizes, simd_quad_cold, simd_quad_warm, simd_binary_cold, simd_binary_warm, suffix)
        plot_four_comparison(sizes, binary_cold, binary_warm, find_cold, find_warm, simd_quad_cold, simd_quad_warm, simd_binary_cold, simd_binary_warm, suffix)