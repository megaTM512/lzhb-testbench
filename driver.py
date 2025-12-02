import subprocess
import os
import csv
import sys

def run_analysis():
  base_dir = os.path.dirname(os.path.abspath(__file__))
  input_dir = os.path.join(base_dir, "input")
  files = os.listdir(input_dir)
  files.sort()
  for filename in files:
    if filename.endswith(".lzcp"):
      filepath = os.path.join(input_dir, filename)
      command = ["./lzhb-testbench", "-i", filepath]
      subprocess.run(command)
      
if __name__ == "__main__":
  run_analysis()