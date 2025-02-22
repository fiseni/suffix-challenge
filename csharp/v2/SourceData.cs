﻿using System.Diagnostics;

namespace v2;

/* Fati Iseni
 * To be cache friendly, it's crucial we store the content in a contiguous memory block.
 * - File.ReadAllLines - it's convenient, but strings might/will be dispersed in the memory.
 * - File.ReadAllText - it maps the whole content to a single string, so all chars are contiguous. 
 *   But, we know the content is ASCII, and the upper byte is a waste (char is 2 bytes in .NET). It might lead to cache line evictions more often.
 * - File.ReadAllBytes - maps the whole content to a single byte[], so all bytes are contiguous.
 * 
 * We'll have a single byte[] memory block and use Memory<byte> to reference individual records.
 * Memory<byte> is 16 bytes struct. If boxed will have minimum size of 32 bytes (an _object field included in the 24 bytes, and additional 2 int fields, total 32 bytes).
 * The overhead of the string object is 24 bytes. They're treated specially in .NET, and some of the fields (e.g. _stringLength) are stored in the object header itself.
 * There is no Memory<byte> boxing here, but there might be once used as a dictionary key in Processor. In case of strings, they are immutable, so there will be a new string instance for each key anyway.
 * Overall, we'll save some memory and have better cache locality.
 * 
 * If we define the Part as a struct, we'll avoid instantiating Part objects in SourceData. Then even the references will be stored contiguously.
 * But, sorting the array then becomes expensive, and the overall performance was slower.
 */

[DebuggerDisplay("{System.Text.Encoding.ASCII.GetString(Code.ToArray())}")]
public sealed class Part(ReadOnlyMemory<byte> code, int index)
{
    public ReadOnlyMemory<byte> Code = code;
    public int Index = index;                                   // Index to the original records. Also used for stable sorting.
}

public sealed class SourceData
{
    public ReadOnlyMemory<byte>[] PartsOriginal = null!;        // Original parts records, trimmed
    public ReadOnlyMemory<byte>[] MasterPartsOriginal = null!;  // Original master parts records, trimmed

    public Part[] PartsAsc = null!;                             // Sorted parts records, uppercased
    public Part[] MasterPartsAsc = null!;                       // Sorted master parts records, uppercased
    public Part[] MasterPartsNhAsc = null!;                     // Sorted master parts records, uppercased, without hyphens (Nh = no hyphens)

    public SourceData(string partsFile, string masterPartsFile)
    {
        Parallel.Invoke(
            () => LoadParts(partsFile),
            () => LoadMasterParts(masterPartsFile)
        );
    }

    public void LoadParts(string partsFile)
    {
        var block = File.ReadAllBytes(partsFile);
        var blockUpper = new byte[block.Length];
        var lineCount = Utils.GetLineCount(block);

        var partsOriginal = new ReadOnlyMemory<byte>[lineCount];
        var partsAsc = new Part[lineCount];

        var partsIndex = 0;
        var blockIndex = 0;
        var blockUpperIndex = 0;

        for (var i = 0; i < block.Length; i++)
        {
            // Also handle records that do not end with a newline
            if (block[i] != Constants.LF && i != block.Length - 1) continue;
            if (block[i] != Constants.LF) i++;

            var stringLength = i > 0 && block[i - 1] == Constants.CR
                ? i - blockIndex - 1
                : i - blockIndex;

            var trimmedRecord = Utils.GetTrimmedRecord(block, blockIndex, stringLength);
            partsOriginal[partsIndex] = trimmedRecord;

            var upperRecord = Utils.GetUpperRecord(trimmedRecord, blockUpper, blockUpperIndex);
            partsAsc[partsIndex] = new Part(upperRecord, partsIndex);
            blockUpperIndex += upperRecord.Length;

            partsIndex++;
            blockIndex = i + 1;
        }

        Array.Sort(partsAsc, StableComparer());
        PartsAsc = partsAsc;
        PartsOriginal = partsOriginal;
    }

    public void LoadMasterParts(string masterPartsFile)
    {
        var block = File.ReadAllBytes(masterPartsFile);
        var blockUpper = new byte[block.Length];
        var blockUpperNh = new byte[block.Length];
        var lineCount = Utils.GetLineCount(block);

        var mpOriginal = new ReadOnlyMemory<byte>[lineCount];
        var mpAsc = new Part[lineCount];
        var mpNhAsc = new Part[lineCount];

        var mpIndex = 0;
        var mpNhIndex = 0;
        var blockIndex = 0;
        var blockUpperIndex = 0;
        var blockUpperNhIndex = 0;
        var containsHyphen = false;

        for (int i = 0; i < block.Length; i++)
        {
            if (block[i] == Constants.HYPHEN) containsHyphen = true;

            // Also handle records that do not end with a newline
            if (block[i] != Constants.LF && i != block.Length - 1) continue;
            if (block[i] != Constants.LF) i++;

            var stringLength = i > 0 && block[i - 1] == Constants.CR
                ? i - blockIndex - 1
                : i - blockIndex;

            var trimmedRecord = Utils.GetTrimmedRecord(block, blockIndex, stringLength);
            if (trimmedRecord.Length >= Constants.MIN_STRING_LENGTH)
            {
                mpOriginal[mpIndex] = trimmedRecord;

                var upperRecord = Utils.GetUpperRecord(trimmedRecord, blockUpper, blockUpperIndex);
                mpAsc[mpIndex] = new Part(upperRecord, mpIndex);
                blockUpperIndex += upperRecord.Length;

                if (containsHyphen)
                {
                    var noHyphensRecord = Utils.GetNoHyphensRecord(upperRecord, blockUpperNh, blockUpperNhIndex);
                    mpNhAsc[mpNhIndex] = new Part(noHyphensRecord, mpIndex);
                    blockUpperNhIndex += noHyphensRecord.Length;
                    mpNhIndex++;
                    containsHyphen = false;
                }

                mpIndex++;
            }
            blockIndex = i + 1;
        }

        MasterPartsOriginal = mpOriginal;

        Array.Resize(ref mpAsc, mpIndex);
        Array.Resize(ref mpNhAsc, mpNhIndex);

        Parallel.Invoke(
            () => Array.Sort(mpAsc, StableComparer()),
            () => Array.Sort(mpNhAsc, StableComparer())
        );

        MasterPartsAsc = mpAsc;
        MasterPartsNhAsc = mpNhAsc;
    }

    // We need a stable sort and Array.Sort is not (OrderBy is stable).
    // So we need to compare Index if the lengths are equal.
    private static Comparison<Part> StableComparer()
        => (x, y) => x.Code.Length == y.Code.Length ? x.Index.CompareTo(y.Index) : x.Code.Length.CompareTo(y.Code.Length);
}
