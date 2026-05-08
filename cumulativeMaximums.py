import matplotlib.pyplot as plt
import numpy as np

# Read data from file
runs = []
current_run = []

path = r"C:\Users\risti\OneDrive\Radna površina\SantaTracker2\maksimumi.txt"

with open(path, "r") as f:
    for line in f:
        line = line.strip()
        if line == "":
            if current_run:
                runs.append(current_run)
                current_run = []
        else:
            current_run.append(float(line))
    if current_run:
        runs.append(current_run)

runs_array = np.array(runs)

average_run = np.mean(runs_array, axis=0)

plt.figure(figsize=(10, 6))
for i, run in enumerate(runs_array):
    iterations = np.arange(1, len(run)+1)
    plt.plot(np.log10(iterations), np.log10(run), alpha=0.5)
plt.plot(np.log10(np.arange(1, len(average_run)+1)), np.log10(average_run), color='red', linewidth=2, label="Srednja vrednost")
plt.xlabel("log10(iteration)")
plt.ylabel("log10(f)")
plt.title("Cumulative maximums (log-log)")
plt.grid(True, which="both", linestyle="--", linewidth=0.5)
plt.legend()
plt.show()