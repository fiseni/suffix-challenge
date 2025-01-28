using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;

namespace v2;

/* Fati Iseni
 * Strings in .NET even though are objects, are treated in special way compared to other objects.
 * Some of the fields (e.g. _stringLength) are mapped directly onto the fields in an EE StringObject, so no overhead for them.
 * The minimal exclusive size starts with 24 bytes (as any other object). Empty strings and strings with single char are included in this size.
 * Then each additional char requires additional 2 bytes.
 * 
 * Memory<char> boxed will have minimum size of 32 bytes (an object ref included in the 24 bytes, and additional 2 int fields, total 32 bytes).
 * So, the size of the boxed Memory<char> will match the strings of 5 chars.
 * 
 * For the our use-case, the codes are mostly longer than 5 chars, so we have net win if we use Memory<char>.
 * Most importantly, the whole file content is loaded into a contiguous char[] memory block.
 */

[DebuggerDisplay("{System.Text.Encoding.ASCII.GetString(Code.ToArray())}")]
public sealed class Part(ReadOnlyMemory<byte> code, int index)
{
    public ReadOnlyMemory<byte> Code = code;
    public int Index = index; // Index to the original records. Also used for stable sorting.
}

public sealed class SourceData
{
    public ReadOnlyMemory<byte>[] PartsOriginal;
    public ReadOnlyMemory<byte>[] MasterPartsOriginal;

    public Part[] PartsAsc;
    public Part[] MasterPartsAsc;
    public Part[] MasterPartsAscNh;

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

            var trimmedRecord = Utils.GetTrimmedMemory(block, blockIndex, stringLength);
            partsOriginal[partsIndex] = trimmedRecord;

            var upperRecord = Utils.GetUpperMemory(trimmedRecord, blockUpper, blockUpperIndex);
            partsAsc[partsIndex] = new Part(upperRecord, partsIndex);
            blockUpperIndex += upperRecord.Length;

            partsIndex++;
            blockIndex = i + 1;
        }

        Array.Sort(partsAsc, StableComparer());
        PartsAsc = partsAsc;
        PartsOriginal = partsOriginal;
    }

    [MemberNotNull(nameof(MasterPartsOriginal), nameof(MasterPartsAsc), nameof(MasterPartsAscNh))]
    public void LoadMasterParts(string masterPartsFile)
    {
        var block = File.ReadAllBytes(masterPartsFile);
        var blockUpper = new byte[block.Length];
        var blockUpperNh = new byte[block.Length];
        var lineCount = Utils.GetLineCount(block);

        var mpOriginal = new ReadOnlyMemory<byte>[lineCount];
        var mpAsc = new Part[lineCount];
        var mpAscNh = new Part[lineCount];

        var mpIndex = 0;
        var mpNhIndex = 0;
        var blockIndex = 0;
        var blockUpperIndex = 0;
        var blockUpperNhIndex = 0;

        for (int i = 0; i < block.Length; i++)
        {
            // Also handle records that do not end with a newline
            if (block[i] != Constants.LF && i != block.Length - 1) continue;
            if (block[i] != Constants.LF) i++;

            var stringLength = i > 0 && block[i - 1] == Constants.CR
                ? i - blockIndex - 1
                : i - blockIndex;

            var trimmedRecord = Utils.GetTrimmedMemory(block, blockIndex, stringLength);
            if (trimmedRecord.Length >= Constants.MIN_STRING_LENGTH)
            {
                mpOriginal[mpIndex] = trimmedRecord;

                var upperRecord = Utils.GetUpperMemory(trimmedRecord, blockUpper, blockUpperIndex);
                mpAsc[mpIndex] = new Part(upperRecord, mpIndex);
                blockUpperIndex += upperRecord.Length;

                if (Utils.ContainsDash(upperRecord))
                {
                    var noDashRecord = Utils.GetNoDashMemory(upperRecord, blockUpperNh, blockUpperNhIndex);
                    mpAscNh[mpNhIndex] = new Part(noDashRecord, mpIndex);
                    blockUpperNhIndex += noDashRecord.Length;
                    mpNhIndex++;
                }

                mpIndex++;
            }
            blockIndex = i + 1;
        }


        MasterPartsOriginal = mpOriginal;

        Array.Resize(ref mpAsc, mpIndex);
        Array.Resize(ref mpAscNh, mpNhIndex);
        Array.Sort(mpAsc, StableComparer());
        Array.Sort(mpAscNh, StableComparer());
        MasterPartsAsc = mpAsc;
        MasterPartsAscNh = mpAscNh;

        //MasterPartsAsc = mpAsc.Take(mpIndex).OrderBy(x => x.Code.Length).ToArray();
        //MasterPartsAscNh = mpAscNh.Take(mpNhIndex).OrderBy(x => x.Code.Length).ToArray();
    }

    // We need a stable sort and Array.Sort is not (OrderBy is stable).
    // So we need to compare Index if the lengths are equal.
    private static Comparison<Part> StableComparer()
        => (x, y) => x.Code.Length == y.Code.Length ? x.Index.CompareTo(y.Index) : x.Code.Length.CompareTo(y.Code.Length);
}
