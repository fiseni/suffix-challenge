# Suffix Challenge

This challenge is inspired by this [article](https://fiseni.com/posts/the-journey-to-630x-faster-batch-job/). In this challenge, we'll slightly modify the original requirements. To make it easier to benchmark implementations across various languages we'll measure the wall time (including reading and writing to disk).

## Challenge Description

The repository includes two files `parts.txt` and `master-parts.txt` that contain part codes. The challenge is, as fast as possible, to find matches for `parts` in `masterParts`.

The following arguments will be passed to the application:
- First argument - the path to `parts.txt`. It contains ~80K lines.
- Second argument - the path to `master-parts.txt`. It contains ~1.5M lines.
- Third argument - the path to `results.txt`.

The challenge is for each record in `parts` to find a match in `masterParts` according to these rules:
- Find the `masterPart` where the current `part` is a suffix to a `masterPart`.
- If not found, find the `masterPart` where the current `part` is a suffix to a `masterPart` but ignoring the hyphens in it.
- If not found, apply the opposite logic, find the `masterPart` where `masterPart` is a suffix to the current `part`.
- If current `part` is less than 3 characters (trimmed) do not find match.
- Ignore `masterParts` with less than 3 characters (trimmed).
- The matches should be case-insensitive.
- Find the best matches; an equal string, then a string with +1 length, and so on.
- If there is more than one match with the same length, return the one that appears the first in the file.

Input files:
- Each line in the file represents a single record. Lines are terminated either by LF or CRLF.
- Records contain only ASCII characters.
- Records have a length of less than 50 characters.
- Records may contain leading/trailing white spaces, and should be trimmed before matching.

Output file:
- It contains the results of the matching.
- It contains all `part` records in the original order.
- Each line consists of two records, the `part` and the matching `masterParts`, separated by `;`. 
- Both records should be trimmed but preserve the original casing.
- For `parts` with less than 3 characters or if no match is found, nothing should be written after `;`.

### Example

| Parts   | MasterParts | Results     | Notes                |
|---------|-------------|-------------|----------------------|
|   aBc   | wa-bc       | aBc;wwwabc  | Based on first rule  |
| qwerTYU |  wwwabc     | qwerTYU;tyu | Based on third rule  |
|  de     | yu          | de;         | No match             |
| ZXC     | aaaaabc     | ZXC;sssz-xC | Based on second rule |
|         |  sssz-xC    |             |                      |
|         | tyu         |             |                      |

## Acceptance criteria

The repository includes `expected.txt` file that contains the expected matches. The acceptance criteria is simple, the produced `results.txt` file should be identical with `expected.txt`.

## Benchmark Results

## Contributions

Everyone is welcome to participate in the challenge. You may use any language of your choice. Create a directory with the language name (if it doesn't exist) and sub-directory with your name (or anything). You may submit implementations for an existing language as well (create a sub-directory).
