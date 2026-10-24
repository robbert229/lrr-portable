# DAT global reverse engineering

`globals.csv` is an address-level work ledger generated from the complete
Ghidra pseudocode corpus in `../decompiles/full/LEGORR.full.c`. It records each
DAT-labeled address referenced by at least one exported function, the number of
references, the functions that reference it, and finding files that mention
the address. It also maps addresses through the PE section table to a file
offset and initial bytes when file-backed; virtual zero-fill is reported
separately. Regenerate it with:

```sh
python3 tools/dat_inventory.py
```

Pass `--binary /path/to/LegoRR.exe` to select another executable, or pass an
output CSV path as the first argument.

The `finding_mentions` column is only a navigation aid. A mention does not mean
the global's role is fully understood. `review_status`, `interpretation`, and
`confidence` are manual coverage fields maintained in `annotations.csv` and
joined into the generated inventory. Record evidence-backed roles in findings;
distinguish observed reads/writes from inferences and unresolved cases.

This ledger's scope is the DAT labels referenced in the checked-in decompile,
not every data symbol in the PE image. The decompile and the executable under
`~/source/lrr-portable/reference/LegoRR.exe` have matching SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
