using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;
using System.Text;

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
public class Part(ReadOnlyMemory<byte> code, int index)
{
    public ReadOnlyMemory<byte> Code = code;
    public int Index = index;
}

public class SourceData
{
    public ReadOnlyMemory<byte>[] PartsOriginal;
    public ReadOnlyMemory<byte>[] MasterPartsOriginal;

    public ReadOnlyMemory<Part> PartsAsc;
    public ReadOnlyMemory<Part> MasterPartsAsc;
    public ReadOnlyMemory<Part> MasterPartsAscNh;

    private static readonly StableSortComparer _stableSortComparer = new();

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
        var stringStartIndex = 0;
        var blockUpperIndex = 0;

        for (var i = 0; i < block.Length; i++)
        {
            // Also handle records that do not end with a newline
            if (block[i] != Constants.LF && i != block.Length - 1) continue;

            var length = i > 0 && block[i - 1] == Constants.CR
                ? i - stringStartIndex - 1
                : i - stringStartIndex;

            var trimmedRecord = Utils.GetTrimmedMemory(block, stringStartIndex, length);
            partsOriginal[partsIndex] = trimmedRecord;

            var upperRecord = Utils.GetUpperMemory(trimmedRecord, blockUpper, blockUpperIndex);
            partsAsc[partsIndex] = new Part(upperRecord, partsIndex);
            blockUpperIndex += upperRecord.Length;

            partsIndex++;
            stringStartIndex = i + 1;
        }

        Array.Sort(partsAsc, _stableSortComparer);

        PartsOriginal = partsOriginal;
        PartsAsc = new ReadOnlyMemory<Part>(partsAsc, 0, partsIndex);
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
        var stringStartIndex = 0;
        var blockUpperIndex = 0;
        var blockUpperNhIndex = 0;

        for (int i = 0; i < block.Length; i++)
        {
            // Also handle records that do not end with a newline
            if (block[i] != Constants.LF && i != block.Length - 1) continue;

            var length = i > 0 && block[i - 1] == Constants.CR
                ? i - stringStartIndex - 1
                : i - stringStartIndex;

            var trimmedRecord = Utils.GetTrimmedMemory(block, stringStartIndex, length);
            if (trimmedRecord.Length >= Constants.MIN_STRING_LENGTH)
            {
                mpOriginal[mpIndex] = trimmedRecord;

                var upperRecord = Utils.GetUpperMemory(trimmedRecord, blockUpper, blockUpperIndex);
                mpAsc[mpIndex] = new Part(upperRecord, mpIndex);
                blockUpperIndex += upperRecord.Length;

                if (Utils.ContainsDash(upperRecord))
                {
                    var noDashRecord = Utils.GetNoDashRecord(upperRecord, blockUpperNh, blockUpperNhIndex);
                    mpAscNh[mpNhIndex] = new Part(noDashRecord, mpIndex);
                    blockUpperNhIndex += noDashRecord.Length;
                    mpNhIndex++;
                }

                mpIndex++;
            }
            stringStartIndex = i + 1;
        }

        Array.Sort(mpAsc, 0, mpIndex, _stableSortComparer);
        Array.Sort(mpAscNh, 0, mpNhIndex, _stableSortComparer);

        MasterPartsOriginal = mpOriginal;
        MasterPartsAsc = new ReadOnlyMemory<Part>(mpAsc, 0, mpIndex);
        MasterPartsAscNh = new ReadOnlyMemory<Part>(mpAscNh, 0, mpNhIndex);
    }

    // We need a stable sort and Array.Sort is not (OrderBy is stable).
    // So we need to compare Index if the lengths are equal.
    private class StableSortComparer : IComparer<Part>
    {
        public int Compare(Part? x, Part? y)
            => x!.Code.Length == y!.Code.Length ? x.Index.CompareTo(y.Index) : x.Code.Length.CompareTo(y.Code.Length);

        public int Compare2(Part? x, Part? y)
        {
            return x!.Code.Length < y!.Code.Length ? -1
                : x.Code.Length > y.Code.Length ? 1
                : x.Index < y.Index ? -1 : x.Index > y.Index ? 1 : 0;
        }
    }
}
