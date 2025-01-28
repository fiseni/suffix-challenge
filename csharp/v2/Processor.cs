using System.Diagnostics.CodeAnalysis;

namespace v2;

public sealed class Processor
{
    private readonly byte[] _buffer = new byte[Constants.MAX_STRING_LENGTH];
    private readonly Context _ctx;

    public Processor(SourceData data)
    {
        var ctx = _ctx = new Context(data);

        CreateSuffixTables(ctx, data.MasterPartsAsc, CreateSuffixTableForMasterParts);
        CreateSuffixTables(ctx, data.MasterPartsAscNh, CreateSuffixTableForMasterPartsNh);
        CreateSuffixTables(ctx, data.PartsAsc, CreateSuffixTableForParts);

        for (var i = data.MasterPartsAsc.Length - 1; i >= 0; i--)
        {
            var masterPart = data.MasterPartsAsc[i];
            var partsIndicesBySuffix = ctx.PartSuffixTables[masterPart.Code.Length];
            var partIndices = partsIndicesBySuffix?.GetValueOrDefault(masterPart.Code);
            if (partIndices is not null)
            {
                var partsAsc = data.PartsAsc;
                for (var j = partIndices.Count - 1; j >= 0; j--)
                {
                    var partIndex = partIndices[j];
                    ctx.PartsLookup.TryAdd(partsAsc[partIndex].Code, masterPart.Index);
                }
            }
        }
    }

    public ReadOnlyMemory<byte>? FindMatch(ReadOnlyMemory<byte> partCode)
    {
        if (partCode.Length < Constants.MIN_STRING_LENGTH) return null;

        var ctx = _ctx;
        var data = ctx.Data;

        var partCodeUpper = Utils.GetUpperSpan(partCode, _buffer.AsSpan()[..partCode.Length]);

        var mpIndexBySuffix = ctx.MpSuffixTablesAlt[partCodeUpper.Length];
        if (mpIndexBySuffix.HasValue && mpIndexBySuffix.Value.TryGetValue(partCodeUpper, out var mpIndex))
        {
            return data.MasterPartsOriginal[mpIndex];
        }

        mpIndexBySuffix = ctx.MpNhSuffixTablesAlt[partCodeUpper.Length];
        if (mpIndexBySuffix.HasValue && mpIndexBySuffix.Value.TryGetValue(partCodeUpper, out mpIndex))
        {
            return data.MasterPartsOriginal[mpIndex];
        }

        if (ctx.PartsLookupAlt.TryGetValue(partCodeUpper, out mpIndex))
        {
            return data.MasterPartsOriginal[mpIndex];
        }

        return null;
    }

    private static void CreateSuffixTables(Context ctx, Part[] parts, Func<Context, int?[], Action<int>> func)
    {
        var startIndexByLength = new int?[Constants.MAX_STRING_LENGTH];
        for (var i = 0; i < parts.Length; i++)
        {
            var length = parts[i].Code.Length;
            if (startIndexByLength[length] is null)
                startIndexByLength[length] = i;
        }
        var longestLength = BackwardFill(startIndexByLength);

        Parallel.For(Constants.MIN_STRING_LENGTH, longestLength + 1, func(ctx, startIndexByLength));
    }

    private static Action<int> CreateSuffixTableForMasterParts(Context ctx, int?[] startIndexesByLength) => suffixLength =>
    {
        var startIndex = startIndexesByLength[suffixLength];
        if (startIndex is not null)
        {
            var masterPartsAsc = ctx.Data.MasterPartsAsc;
            var tempDict = new Dictionary<ReadOnlyMemory<byte>, int>(masterPartsAsc.Length - startIndex.Value, Context.Comparer);
            for (var i = startIndex.Value; i < masterPartsAsc.Length; i++)
            {
                var mp = masterPartsAsc[i];
                var suffix = mp.Code[^suffixLength..];
                tempDict.TryAdd(suffix, mp.Index);
            }
            ctx.MpSuffixTables[suffixLength] = tempDict;
            ctx.MpSuffixTablesAlt[suffixLength] = tempDict.GetAlternateLookup<ReadOnlySpan<byte>>();
        }
    };

    private static Action<int> CreateSuffixTableForMasterPartsNh(Context ctx, int?[] startIndexesByLength) => suffixLength =>
    {
        var startIndex = startIndexesByLength[suffixLength];
        if (startIndex is not null)
        {
            var masterPartsAscNh = ctx.Data.MasterPartsAscNh;
            var tempDict = new Dictionary<ReadOnlyMemory<byte>, int>(masterPartsAscNh.Length - startIndex.Value, Context.Comparer);
            for (var i = startIndex.Value; i < masterPartsAscNh.Length; i++)
            {
                var mp = masterPartsAscNh[i];
                var suffix = masterPartsAscNh[i].Code[^suffixLength..];
                tempDict.TryAdd(suffix, mp.Index);
            }
            ctx.MpNhSuffixTables[suffixLength] = tempDict;
            ctx.MpNhSuffixTablesAlt[suffixLength] = tempDict.GetAlternateLookup<ReadOnlySpan<byte>>();
        }
    };

    private static Action<int> CreateSuffixTableForParts(Context ctx, int?[] startIndexesByLength) => suffixLength =>
    {
        var startIndex = startIndexesByLength[suffixLength];
        if (startIndex is not null)
        {
            var partsAsc = ctx.Data.PartsAsc;
            var tempDict = new Dictionary<ReadOnlyMemory<byte>, List<int>>(partsAsc.Length - startIndex.Value, Context.Comparer);
            for (var i = startIndex.Value; i < partsAsc.Length; i++)
            {
                var suffix = partsAsc[i].Code[^suffixLength..];
                if (tempDict.TryGetValue(suffix, out var originalPartIndices))
                {
                    originalPartIndices.Add(i);
                }
                else
                {
                    tempDict.TryAdd(suffix, [i]);
                }
            }
            ctx.PartSuffixTables[suffixLength] = tempDict;
        }
    };

    private static int BackwardFill(int?[] array)
    {
        int lastFilled = Constants.MAX_STRING_LENGTH - 1;
        var temp = array[Constants.MAX_STRING_LENGTH - 1];
        for (var i = Constants.MAX_STRING_LENGTH - 1; i >= 0; i--)
        {
            if (array[i] is null)
            {
                array[i] = temp;
            }
            else
            {
                temp = array[i];
                if (lastFilled == Constants.MAX_STRING_LENGTH - 1)
                {
                    lastFilled = i;
                }
            }
        }
        return lastFilled;
    }

    private sealed class Context
    {
        public static readonly ReadOnlyMemoryByteComparer Comparer = new();

        public SourceData Data;

        public Dictionary<ReadOnlyMemory<byte>, int> PartsLookup;
        public Dictionary<ReadOnlyMemory<byte>, int>.AlternateLookup<ReadOnlySpan<byte>> PartsLookupAlt;
        public Dictionary<ReadOnlyMemory<byte>, List<int>>[] PartSuffixTables = new Dictionary<ReadOnlyMemory<byte>, List<int>>[Constants.MAX_STRING_LENGTH];

        public Dictionary<ReadOnlyMemory<byte>, int>[] MpSuffixTables = new Dictionary<ReadOnlyMemory<byte>, int>[Constants.MAX_STRING_LENGTH];
        public Dictionary<ReadOnlyMemory<byte>, int>.AlternateLookup<ReadOnlySpan<byte>>?[] MpSuffixTablesAlt =
            new Dictionary<ReadOnlyMemory<byte>, int>.AlternateLookup<ReadOnlySpan<byte>>?[Constants.MAX_STRING_LENGTH];

        public Dictionary<ReadOnlyMemory<byte>, int>[] MpNhSuffixTables = new Dictionary<ReadOnlyMemory<byte>, int>[Constants.MAX_STRING_LENGTH];
        public Dictionary<ReadOnlyMemory<byte>, int>.AlternateLookup<ReadOnlySpan<byte>>?[] MpNhSuffixTablesAlt =
            new Dictionary<ReadOnlyMemory<byte>, int>.AlternateLookup<ReadOnlySpan<byte>>?[Constants.MAX_STRING_LENGTH];

        public Context(SourceData data)
        {
            Data = data;
            PartsLookup = new(data.PartsAsc.Length, Comparer);
            PartsLookupAlt = PartsLookup.GetAlternateLookup<ReadOnlySpan<byte>>();
        }
    }

    private sealed class ReadOnlyMemoryByteComparer :
        IEqualityComparer<ReadOnlyMemory<byte>>,
        IAlternateEqualityComparer<ReadOnlySpan<byte>, ReadOnlyMemory<byte>>
    {
        // In our case we will never add using ReadOnlySpan<byte>
        public ReadOnlyMemory<byte> Create(ReadOnlySpan<byte> alternate)
            => throw new NotImplementedException();

        public bool Equals(ReadOnlyMemory<byte> x, ReadOnlyMemory<byte> y)
            => x.Span.SequenceEqual(y.Span);

        public bool Equals(ReadOnlySpan<byte> alternate, ReadOnlyMemory<byte> other)
            => alternate.SequenceEqual(other.Span);

        public int GetHashCode([DisallowNull] ReadOnlyMemory<byte> obj)
            => GetHashCode(obj.Span);

        public int GetHashCode(ReadOnlySpan<byte> alternate)
        {
            int hash = 16777619;
            for (int i = 0; i < alternate.Length; i++)
            {
                hash = (hash * 31) + alternate[i];
            }
            return hash;
        }

        //// The FnvHash is performing slower overall for our use-case
        //public int GetHashCode(ReadOnlySpan<byte> alternate)
        //{
        //    unchecked
        //    {
        //        const int prime = 16777619;
        //        int hash = (int)2166136261;

        //        for (int i = 0; i < alternate.Length; i++)
        //            hash = (hash ^ alternate[i]) * prime;

        //        return hash;
        //    }
        //}
    }
}
