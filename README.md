# Suffix Matching Challenge

This is inspired by the following [article](https://fiseni.com/posts/the-journey-to-630x-faster-batch-job/). But, in this challenge, we'll slightly modify the original requirements. To make it easier to benchmark implementations across various languages we'll measure the wall time (including reading and writing to disk).

## Challenge Description

The repository includes two files, `parts.txt` and `master-parts.txt`, that contain part codes. The goal is to find matches for `parts` in `masterParts` as fast as possible.

Input files:
- Each line in the file represents a single record. Lines are terminated either by LF or CRLF.
- Records contain only ASCII characters.
- Records have a length of less than 50 characters.
- Records may contain leading/trailing white spaces and should be trimmed before matching.

The challenge is for each record in `parts` to find a match in `masterParts` according to these rules:
- Find the `masterPart` where the current `part` is a suffix to a `masterPart`.
- If not found, find the `masterPart` where the current `part` is a suffix to a `masterPart` ignoring the hyphens `-` in `masterPart`.
- If not found, find the `masterPart` where `masterPart` is a suffix to the current `part`.
- If the current `part` is less than 3 characters (trimmed) do not find a match.
- Ignore `masterParts` with less than 3 characters (trimmed).
- Matches should be case-insensitive.
- Prioritize best matches:
  - Exact matches first.
  - Then matches with minimal length difference.
  - For ties, choose the first `masterPart` in the file.
  
Output:
- The app should print the number of matches.
- The app should create a `results.txt` file containing matching results:
  - Include all `parts` in the same order as the input.
  - Format: `<part>;<matching masterPart>`
  - For unmatched `parts`, include only `<part>;`.
  - Trim the records, but preserve the original casing.

### Example

| Parts   | MasterParts | Results     | Notes                |
|---------|-------------|-------------|----------------------|
|   aBc   | wa-bc       | aBc;wwwabc  | Based on first rule  |
| qwerTYU |  wwwabc     | qwerTYU;tyu | Based on third rule  |
|  de     | yu          | de;         | No match             |
| ZXC     | aaaaabc     | ZXC;sssz-xC | Based on second rule |
|         |  sssz-xC    |             |                      |
|         | tyu         |             |                      |
|         | xde         |             |                      |

### Acceptance criteria

- The number of matches for the given input files is 41241.
- The repository includes an `expected.txt` file that contains the expected matches. The produced `results.txt` file should be identical to `expected.txt`. 
- You should not make assumptions based on these specific input files (e.g. size, string length distribution, etc.).

## Contributions

Everyone is welcome to participate in the challenge. 
- You may use a language of your choice.
- Create a directory with the language name (if it doesn't exist) and a sub-directory with your name (or anything).
- Create a `build.sh`/`build.bat` script that builds the application.
- Create a `run.sh`/`run.bat` script that runs the application.

## Benchmark Results

| Command | Mean [s] | Min [s] | Max [s] | Relative |
|:---|---:|---:|---:|---:|
| `C#` | 1.555 ± 0.050 | 1.512 | 1.610 | 1.00 |
| `C# AOT` | 1.459 ± 0.006 | 1.452 | 1.464 | 1.00 |

