using System.Diagnostics.CodeAnalysis;

namespace v2;

public sealed class Processor
{
    private readonly byte[] _buffer = new byte[Constants.MAX_STRING_LENGTH];
    private readonly Context _ctx;

    public Processor(SourceData data)
    {
        var ctx = _ctx = new Context(data);

        CreateTablesInParallel(ctx, data.MasterPartsAsc, CreateSuffixTableForMasterParts);
        CreateTablesInParallel(ctx, data.MasterPartsNhAsc, CreateSuffixTableForMasterPartsNh);
        CreateTablesInParallel(ctx, data.PartsAsc, CreateTableForParts);
    }

    public ReadOnlyMemory<byte>? FindMatch(ReadOnlyMemory<byte> partCode)
    {
        if (partCode.Length < Constants.MIN_STRING_LENGTH) return null;

        var ctx = _ctx;
        var data = ctx.Data;

        var partCodeUpper = Utils.GetUpperRecord(partCode, _buffer);

        // Don't extract it to a method, it slows down for 5-10 ms (it's unable to inline for AOT).

        var mpIndexByMpSuffix = ctx.MpSuffixTablesAlt[partCodeUpper.Length];
        if (mpIndexByMpSuffix.HasValue && mpIndexByMpSuffix.Value.TryGetValue(partCodeUpper, out var mpIndex))
        {
            return data.MasterPartsOriginal[mpIndex];
        }

        var mpIndexByMpNhSuffix = ctx.MpNhSuffixTablesAlt[partCodeUpper.Length];
        if (mpIndexByMpNhSuffix.HasValue && mpIndexByMpNhSuffix.Value.TryGetValue(partCodeUpper, out mpIndex))
        {
            return data.MasterPartsOriginal[mpIndex];
        }

        var mpIndexByPart = ctx.PartTablesAlt[partCodeUpper.Length];
        if (mpIndexByPart.HasValue && mpIndexByPart.Value.TryGetValue(partCodeUpper, out mpIndex))
        {
            return data.MasterPartsOriginal[mpIndex];
        }

        return null;
    }

    private static void CreateTablesInParallel(Context ctx, Part[] parts, Func<Context, int?[], Action<int>> func)
    {
        var startIndexesByLength = new int?[Constants.MAX_STRING_LENGTH];
        for (var i = 0; i < parts.Length; i++)
        {
            var length = parts[i].Code.Length;
            if (startIndexesByLength[length] is null)
                startIndexesByLength[length] = i;
        }
        var maxLength = BackwardFill(startIndexesByLength);

        Parallel.For(Constants.MIN_STRING_LENGTH - 1, maxLength + 1, func(ctx, startIndexesByLength));
    }

    private static Action<int> CreateSuffixTableForMasterParts(Context ctx, int?[] startIndexesByLength) => length =>
    {
        // We will use the (MIN_STRING_LENGTH - 1) thread to fill the master parts dictionary.
        if (length == Constants.MIN_STRING_LENGTH - 1)
        {
            var masterPartsAsc = ctx.Data.MasterPartsAsc;
            var tempDict = new Dictionary<ReadOnlyMemory<byte>, int>(masterPartsAsc.Length, Context.Comparer);
            for (var i = 0; i < masterPartsAsc.Length; i++)
            {
                var mp = masterPartsAsc[i];
                tempDict.TryAdd(mp.Code, mp.Index);
            }
            ctx.MpDictionary = tempDict;
            ctx.MpDictionaryAlt = tempDict.GetAlternateLookup<ReadOnlySpan<byte>>();
            return;
        }

        var startIndex = startIndexesByLength[length];
        if (startIndex is not null)
        {
            var masterPartsAsc = ctx.Data.MasterPartsAsc;
            var tempDict = new Dictionary<ReadOnlyMemory<byte>, int>(masterPartsAsc.Length - startIndex.Value, Context.Comparer);
            for (var i = startIndex.Value; i < masterPartsAsc.Length; i++)
            {
                var mp = masterPartsAsc[i];
                var suffix = mp.Code[^length..];
                tempDict.TryAdd(suffix, mp.Index);
            }
            ctx.MpSuffixTables[length] = tempDict;
            ctx.MpSuffixTablesAlt[length] = tempDict.GetAlternateLookup<ReadOnlySpan<byte>>();
        }
    };

    private static Action<int> CreateSuffixTableForMasterPartsNh(Context ctx, int?[] startIndexesByLength) => length =>
    {
        if (length == Constants.MIN_STRING_LENGTH - 1) return;
        var startIndex = startIndexesByLength[length];
        if (startIndex is not null)
        {
            var masterPartsNhAsc = ctx.Data.MasterPartsNhAsc;
            var tempDict = new Dictionary<ReadOnlyMemory<byte>, int>(masterPartsNhAsc.Length - startIndex.Value, Context.Comparer);
            for (var i = startIndex.Value; i < masterPartsNhAsc.Length; i++)
            {
                var mp = masterPartsNhAsc[i];
                var suffix = mp.Code[^length..];
                tempDict.TryAdd(suffix, mp.Index);
            }
            ctx.MpNhSuffixTables[length] = tempDict;
            ctx.MpNhSuffixTablesAlt[length] = tempDict.GetAlternateLookup<ReadOnlySpan<byte>>();
        }
    };

    private static Action<int> CreateTableForParts(Context ctx, int?[] startIndexesByLength) => length =>
    {
        if (length == Constants.MIN_STRING_LENGTH - 1) return;
        var startIndex = startIndexesByLength[length];
        if (startIndex is not null)
        {
            var partsAsc = ctx.Data.PartsAsc;
            var tempDict = new Dictionary<ReadOnlyMemory<byte>, int>(partsAsc.Length - startIndex.Value, Context.Comparer);
            for (var i = startIndex.Value; i < partsAsc.Length; i++)
            {
                var partCode = partsAsc[i].Code;
                if (partCode.Length > length) break;

                for (int suffixLength = partCode.Length - 1; suffixLength >= Constants.MIN_STRING_LENGTH; suffixLength--)
                {
                    var suffix = partCode.Span[^suffixLength..];
                    if (ctx.MpDictionaryAlt.TryGetValue(suffix, out var mpIndex))
                    {
                        tempDict.TryAdd(partCode, mpIndex);
                        break;
                    }
                }
            }
            ctx.PartTables[length] = tempDict;
            ctx.PartTablesAlt[length] = tempDict.GetAlternateLookup<ReadOnlySpan<byte>>();
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

    private sealed class Context(SourceData data)
    {
        public static readonly ReadOnlyMemoryByteComparer Comparer = new();

        public SourceData Data = data;

        public Dictionary<ReadOnlyMemory<byte>, int>? MpDictionary;
        public Dictionary<ReadOnlyMemory<byte>, int>.AlternateLookup<ReadOnlySpan<byte>> MpDictionaryAlt;

        public Dictionary<ReadOnlyMemory<byte>, int>[] PartTables = new Dictionary<ReadOnlyMemory<byte>, int>[Constants.MAX_STRING_LENGTH];
        public Dictionary<ReadOnlyMemory<byte>, int>.AlternateLookup<ReadOnlySpan<byte>>?[] PartTablesAlt =
            new Dictionary<ReadOnlyMemory<byte>, int>.AlternateLookup<ReadOnlySpan<byte>>?[Constants.MAX_STRING_LENGTH];

        public Dictionary<ReadOnlyMemory<byte>, int>[] MpSuffixTables = new Dictionary<ReadOnlyMemory<byte>, int>[Constants.MAX_STRING_LENGTH];
        public Dictionary<ReadOnlyMemory<byte>, int>.AlternateLookup<ReadOnlySpan<byte>>?[] MpSuffixTablesAlt =
            new Dictionary<ReadOnlyMemory<byte>, int>.AlternateLookup<ReadOnlySpan<byte>>?[Constants.MAX_STRING_LENGTH];

        public Dictionary<ReadOnlyMemory<byte>, int>[] MpNhSuffixTables = new Dictionary<ReadOnlyMemory<byte>, int>[Constants.MAX_STRING_LENGTH];
        public Dictionary<ReadOnlyMemory<byte>, int>.AlternateLookup<ReadOnlySpan<byte>>?[] MpNhSuffixTablesAlt =
            new Dictionary<ReadOnlyMemory<byte>, int>.AlternateLookup<ReadOnlySpan<byte>>?[Constants.MAX_STRING_LENGTH];
    }

    private sealed class ReadOnlyMemoryByteComparer :
        IEqualityComparer<ReadOnlyMemory<byte>>,
        IAlternateEqualityComparer<ReadOnlySpan<byte>, ReadOnlyMemory<byte>>
    {
        // In our case we will never add using ReadOnlySpan<byte>
        public ReadOnlyMemory<byte> Create(ReadOnlySpan<byte> alternate)
            => throw new NotImplementedException();

        public bool Equals(ReadOnlyMemory<byte> x, ReadOnlyMemory<byte> y)
            => x.Equals(y) || x.Span.SequenceEqual(y.Span);

        public bool Equals(ReadOnlySpan<byte> alternate, ReadOnlyMemory<byte> other)
        {
            var otherSpan = other.Span;
            return alternate == otherSpan || alternate.SequenceEqual(otherSpan);
        }

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
