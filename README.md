# LZHB-Testbench

## Usage

### Running the program

Make with:

```bash
make
```

Then simply

```bash
./lzhb-testbench -i {INPUT_FILE}
```

### Arguments

```bash
-i {INPUT_FILE}   # The input file. Must follow the lzcp format from lzhb.
-o {OUTPUT_FILE}  # The output for the test results. Currently does nothing.
-r {NUMBER}       # The number of repeats for the "random access" test.
-b {NUMBER}       # Batch size.
-v                # If the decoded phrases and the decoded text should be printed.
```

### Limitations

Currently, only the output of [https://github.com/dscalgo/lzhb](lzhb) can be used for now, specifically using the -a flag to generate phrases with including the last character.
