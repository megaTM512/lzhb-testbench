import subprocess
import os
import csv
import sys

def run_analysis(input_dir_name:str):
  base_dir = os.path.dirname(os.path.abspath(__file__))
  input_dir = os.path.join(base_dir, input_dir_name)
  files = os.listdir(input_dir)
  files.sort()
  for filename in files:
    if filename.endswith(".lzcp"):
      filepath = os.path.join(input_dir, filename)
      #command = ["./lzhb-testbench", "-i", filepath, "-r", "100000", "-b", "512", "-s", "100000"]
      command = ["./lzhb-testbench", "-i", filepath, "-r", "1", "-b", "1", "-s", "1"]
      subprocess.run(command)
      
if __name__ == "__main__":
  print("Running analyses...")
  """   run_analysis("res/c1")
  run_analysis("res/c2")
  run_analysis("res/c3")
  run_analysis("res/c4")
  run_analysis("res/kpp3")
  run_analysis("res/lzlmocc")
  run_analysis("res/lzmaxocc")
  run_analysis("res/lzhb3")
  run_analysis("res/lzhb3sumh") """
  run_analysis("kkp3all")