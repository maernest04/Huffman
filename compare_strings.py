
import math
from collections import Counter

commands = [
    "Pltog", "Plstat", "Plmode", "Pltarg", "Plkwlm", "Plinit", "Pltqcm", "Plclmp",
    "LcKp",  "lcKi",   "lcKd",   "lcpid",  "lcSRT",  "lcLcTog", "lcCSR",
    "lcCVD", "lcTVD",  "lcLTq",  "lcITq",
    "lck",   "lcMTq",  "lcPTq",  "lcUF",   "lcmode", "lcSt",   "lcPh",
    "efTog", "efEBk", "efLpCt", "efCOk",
    "efTS_s", "efTC_s", "efESk", "efESs",
    "efLEk", "efTLk", "efFLp",
    "rgRgTog", "rgMd", "rgApTq", "rgBTN", "rgRTq",
    "rgTLD", "rgTZPD", "rgPBM", "rgPAC",
    "rgPdMu", "rgTk"
]

comments = [
    "Power limit toggle",
    "Power limit status",
    "Power limit mode",
    "Power limit target",
    "Power limit kW limit",
    "Power limit init",
    "Power limit torque command",
    "Power limit clamp",
    "Launch control Kp",
    "Launch control Ki",
    "Launch control Kd",
    "Launch control PID",
    "Launch control slip ratio target",
    "Launch control LC toggle",
    "Launch control current slip ratio",
    "Launch control current velocity difference",
    "Launch control target velocity difference",
    "Launch control LC torque command",
    "Launch control initial torque",
    "Launch control k",
    "Launch control max torque",
    "Launch control previous torque",
    "Launch control use filter",
    "Launch control mode",
    "Launch control state",
    "Launch control phase",
    "Efficiency efficiency toggle",
    "Efficiency energy budget kWh (efEBk)",
    "Efficiency lap counter",
    "Efficiency carry over energy kWh (efCOk)",
    "Efficiency time eff in straights (s)",
    "Efficiency time eff in corners (s)",
    "Efficiency energy spent in corners kWh (efESk)",
    "Efficiency energy spent in straights kWh (efESs)",
    "Efficiency lap energy spent kWh (efLEk)",
    "Efficiency total lap distance km (efTLk)",
    "Efficiency finished lap",
    "Regen regen toggle",
    "Regen mode",
    "Regen APPS torque",
    "Regen BPS torque Nm",
    "Regen regen torque command",
    "Regen torque limit D Nm",
    "Regen torque at zero pedal D Nm",
    "Regen percent BPS for max regen",
    "Regen percent APPS for coasting",
    "Regen pad mu",
    "Regen tick"
]

def huffman_simulate(strings):
    all_chars = "".join(strings)
    counts = Counter(all_chars)
    # Simple relative length estimate: log2(1/prob)
    total = sum(counts.values())
    codes = {char: -math.log2(count/total) for char, count in counts.items()}
    # This is an estimate, true Huffman is integer bits. But for comparison it's okay.
    # To be precise, let's just count unique chars for now or do a real Huffman.
    unique_chars = len(counts)
    return counts, unique_chars

def get_huffman_lengths(strings):
    # Basic huffman length calculation
    import heapq
    all_chars = "".join(strings)
    freq = Counter(all_chars)
    if not freq: return {}
    
    heap = [[f, [c, ""]] for c, f in freq.items()]
    heapq.heapify(heap)
    while len(heap) > 1:
        lo = heapq.heappop(heap)
        hi = heapq.heappop(heap)
        for pair in lo[1:]:
            pair[1] = '0' + pair[1]
        for pair in hi[1:]:
            pair[1] = '1' + pair[1]
        heapq.heappush(heap, [lo[0] + hi[0]] + lo[1:] + hi[1:])
    
    res = sorted(heapq.heappop(heap)[1:], key=lambda p: (len(p[-1]), p[0]))
    return {char: len(code) for char, code in res}

def calculate_total_bits(strings, lengths_dict):
    total = 0
    for s in strings:
        for char in s:
            total += lengths_dict[char]
    return total

def get_bits(s, lengths_dict):
    return sum(lengths_dict[char] for char in s)

# Case 1: Mixed Case
mixed_lengths = get_huffman_lengths(commands)
# Case 2: Lowercase Only
lower_commands = [c.lower() for c in commands]
lower_lengths = get_huffman_lengths(lower_commands)

print(f"| {'Full Name (Original)':<55} | {'Shortened (Mixed)':<18} | {'Bits':<6} | {'Shortened (Lower)':<18} | {'Bits':<6} |")
print(f"|{'-'*57}|{'-'*20}|{'-'*8}|{'-'*20}|{'-'*8}|")

for i in range(len(commands)):
    full = comments[i]
    mixed = commands[i]
    lower = lower_commands[i]
    m_bits = get_bits(mixed, mixed_lengths)
    l_bits = get_bits(lower, lower_lengths)
    print(f"| {full:<55} | {mixed:<18} | {m_bits:<6} | {lower:<18} | {l_bits:<6} |")

print("\n--- Summary ---")
m_total = sum(get_bits(c, mixed_lengths) for c in commands)
l_total = sum(get_bits(c, lower_lengths) for c in lower_commands)
m_unique = len(Counter("".join(commands)))
l_unique = len(Counter("".join(lower_commands)))

print(f"Mixed Case Unique Chars: {m_unique}")
print(f"Lower Case Unique Chars: {l_unique}")
print(f"Mixed Case Total Bits: {m_total}")
print(f"Lower Case Total Bits: {l_total}")

m_bits_list = [get_bits(c, mixed_lengths) for c in commands]
l_bits_list = [get_bits(c, lower_lengths) for c in lower_commands]

print(f"\nMixed Case: Min Bits = {min(m_bits_list)}, Max Bits = {max(m_bits_list)}, Average = {m_total/len(commands):.2f}")
print(f"Lower Case: Min Bits = {min(l_bits_list)}, Max Bits = {max(l_bits_list)}, Average = {l_total/len(commands):.2f}")

