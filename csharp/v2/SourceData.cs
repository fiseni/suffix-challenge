using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;

namespace v2;

/* Fati Iseni
 * To be cache friendly, it's crucial we store the content in a contiguous memory block.
 * - File.ReadAllLines - it's convenient, but strings might/will be dispersed in the memory.
 * - File.ReadAllText - maps the whole content to a single string, so all chars are contiguous. 
 *   But, we know the content is ASCII, and the upper byte is a waste. It might lead to cache line evictions more often.
 * - File.ReadAllBytes - maps the whole content to a single byte[], so all bytes are contiguous.
 * 
 * We'll have a single byte[] memory block and use Memory<byte> to reference individual records.
 * Memory<byte> once boxed will have minimum size of 32 bytes (an _object field included in the 24 bytes, and additional 2 int fields, total 32 bytes).
 * The overhead of the string object is 24 bytes. They're treated specially in .NET, and some of the fields (e.g. _stringLength) are stored in the object header itself.
 * So, we have additional 8 bytes overhead, but it's worth it considering the content is contiguous.
 * 
 * If we define the Part as a struct, we'll avoid Memory<byte> boxing here. Then even the references will be stored contiguously.
 * But, sorting the array then becomes expensive, and the overall performance was slower.
 */

[DebuggerDisplay("{System.Text.Encoding.ASCII.GetString(Code.ToArray())}")]
public sealed class Part(ReadOnlyMemory<byte> code, int index)
{
    public ReadOnlyMemory<byte> Code = code;
    public int Index = index;                           // Index to the original records. Also used for stable sorting.
}

public sealed class SourceData
{
    public ReadOnlyMemory<byte>[] PartsOriginal;        // Original parts records, trimmed
    public ReadOnlyMemory<byte>[] MasterPartsOriginal;  // Original master parts records, trimmed

    public Part[] PartsAsc;                             // Sorted parts records, uppercased
    public Part[] MasterPartsAsc;                       // Sorted master parts records, uppercased
    public Part[] MasterPartsNhAsc;                     // Sorted master parts records, uppercased, without hyphens (Nh = no hyphens)

    public SourceData(string partsFile, string masterPartsFile)
    {
        LoadParts(partsFile);
        LoadMasterParts(masterPartsFile);
    }

    [MemberNotNull(nameof(PartsOriginal), nameof(PartsAsc))]
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

    [MemberNotNull(nameof(MasterPartsOriginal), nameof(MasterPartsAsc), nameof(MasterPartsNhAsc))]
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
        Array.Sort(mpAsc, StableComparer());
        Array.Sort(mpNhAsc, StableComparer());
        MasterPartsAsc = mpAsc;
        MasterPartsNhAsc = mpNhAsc;

        //MasterPartsAsc = mpAsc.Take(mpIndex).OrderBy(x => x.Code.Length).ToArray();
        //MasterPartsNhAsc = mpNhAsc.Take(mpNhIndex).OrderBy(x => x.Code.Length).ToArray();
    }

    // We need a stable sort and Array.Sort is not (OrderBy is stable).
    // So we need to compare Index if the lengths are equal.
    private static Comparison<Part> StableComparer()
        => (x, y) => x.Code.Length == y.Code.Length ? x.Index.CompareTo(y.Index) : x.Code.Length.CompareTo(y.Code.Length);
}
