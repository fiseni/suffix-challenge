using System.Diagnostics.CodeAnalysis;
using System.Runtime.CompilerServices;
using System.Text;

namespace v2;

public class Processor
{
    private class Context
    {
        public required SourceData Data { get; init; }
        public required Dictionary<ReadOnlyMemory<byte>, int>[] MpSuffixTables { get; init; }
        public required Dictionary<ReadOnlyMemory<byte>, int>[] MpNhSuffixTables { get; init; }
        public required Dictionary<ReadOnlyMemory<byte>, List<int>>[] PartSuffixTables { get; init; }
        public required Dictionary<ReadOnlyMemory<byte>, int> State { get; init; }
    }

    private static readonly MemoryComparer _memoryComparer = new();
    private readonly Context _ctx;

    public Processor(SourceData data)
    {
        _ctx = BuildContext(data);
    }

    public ReadOnlyMemory<byte>? FindMatch(ReadOnlyMemory<byte> partCode)
    {
        if (partCode.Length < Constants.MIN_STRING_LENGTH) return null;

        Span<byte> buffer = stackalloc byte[partCode.Length];
        var upper = Utils.GetUpperMemory(partCode, buffer.ToArray(), 0);

        if (_ctx.State.TryGetValue(upper, out var mpIndex))
        {
            return _ctx.Data.MasterPartsOriginal[mpIndex];
        }
        return null;
    }

    private static Context BuildContext(SourceData data)
    {
        var ctx = new Context
        {
            Data = data,
            MpSuffixTables = new Dictionary<ReadOnlyMemory<byte>, int>[Constants.MAX_STRING_LENGTH],
            MpNhSuffixTables = new Dictionary<ReadOnlyMemory<byte>, int>[Constants.MAX_STRING_LENGTH],
            PartSuffixTables = new Dictionary<ReadOnlyMemory<byte>, List<int>>[Constants.MAX_STRING_LENGTH],
            State = new Dictionary<ReadOnlyMemory<byte>, int>(data.PartsAsc.Length, _memoryComparer)
        };

        CreateSuffixTables(ctx, data.MasterPartsAsc, CreateSuffixTableForMasterParts);
        CreateSuffixTables(ctx, data.MasterPartsAscNh, CreateSuffixTableForMasterPartsNh);
        CreateSuffixTables(ctx, data.PartsAsc, CreateSuffixTableForParts);

        for (var i = 0; i < data.PartsAsc.Length; i++)
        {
            var code = data.PartsAsc.Span[i].Code;

            var mpIndexBySuffix = ctx.MpSuffixTables[code.Length];
            if (mpIndexBySuffix is not null && mpIndexBySuffix.TryGetValue(code, out var mpIndex))
            {
                ctx.State.TryAdd(code, mpIndex);
                continue;
            }

            var mpNhIndexBySuffix = ctx.MpNhSuffixTables[code.Length];
            if (mpNhIndexBySuffix is not null && mpNhIndexBySuffix.TryGetValue(code, out mpIndex))
            {
                ctx.State.TryAdd(code, mpIndex);
            }
        }

        for (var i = data.MasterPartsAsc.Length - 1; i >= 0; i--)
        {
            var masterPart = data.MasterPartsAsc.Span[i];

            var partsIndicesBySuffix = ctx.PartSuffixTables[masterPart.Code.Length];
            var originalPartIndices = partsIndicesBySuffix?.GetValueOrDefault(masterPart.Code);
            if (originalPartIndices is not null)
            {
                for (var j = originalPartIndices.Count - 1; j >= 0; j--)
                {
                    var index = originalPartIndices[j];
                    ctx.State.TryAdd(data.PartsAsc.Span[index].Code, masterPart.Index);
                }
            }
        }

        return ctx;
    }

    private static void CreateSuffixTables(Context ctx, ReadOnlyMemory<Part> parts, Func<Context, int?[], Action<int>> func)
    {
        var startIndexByLength = new int?[Constants.MAX_STRING_LENGTH];
        for (var i = 0; i < Constants.MAX_STRING_LENGTH; i++)
        {
            startIndexByLength[i] = null;
        }
        for (var i = 0; i < parts.Length; i++)
        {
            var length = parts.Span[i].Code.Length;
            if (startIndexByLength[length] is null)
                startIndexByLength[length] = i;
        }
        BackwardFill(startIndexByLength);

        Parallel.For(Constants.MIN_STRING_LENGTH, Constants.MAX_STRING_LENGTH, func(ctx, startIndexByLength));

    }

    private static Action<int> CreateSuffixTableForMasterParts(Context ctx, int?[] startIndexesByLength)
    {
        return suffixLength =>
        {
            var startIndex = startIndexesByLength[suffixLength];
            if (startIndex is not null)
            {
                var masterPartsAsc = ctx.Data.MasterPartsAsc;
                var tempDict = new Dictionary<ReadOnlyMemory<byte>, int>(masterPartsAsc.Length - startIndex.Value, _memoryComparer);
                for (var i = startIndex.Value; i < masterPartsAsc.Length; i++)
                {
                    var mp = masterPartsAsc.Span[i];
                    var suffix = mp.Code[^suffixLength..];
                    tempDict.TryAdd(suffix, mp.Index);
                }
                ctx.MpSuffixTables[suffixLength] = tempDict;
            }
        };
    }

    private static Action<int> CreateSuffixTableForMasterPartsNh(Context ctx, int?[] startIndexesByLength)
    {
        return suffixLength =>
        {
            var startIndex = startIndexesByLength[suffixLength];
            if (startIndex is not null)
            {
                var masterPartsAscNh = ctx.Data.MasterPartsAscNh;
                var tempDict = new Dictionary<ReadOnlyMemory<byte>, int>(masterPartsAscNh.Length - startIndex.Value, _memoryComparer);
                for (var i = startIndex.Value; i < masterPartsAscNh.Length; i++)
                {
                    var mp = masterPartsAscNh.Span[i];
                    var suffix = masterPartsAscNh.Span[i].Code[^suffixLength..];
                    tempDict.TryAdd(suffix, mp.Index);
                }
                ctx.MpNhSuffixTables[suffixLength] = tempDict;
            }
        };
    }

    private static Action<int> CreateSuffixTableForParts(Context ctx, int?[] startIndexesByLength)
    {
        return suffixLength =>
        {
            var startIndex = startIndexesByLength[suffixLength];
            if (startIndex is not null)
            {
                var partsAsc = ctx.Data.PartsAsc;
                var tempDict = new Dictionary<ReadOnlyMemory<byte>, List<int>>(partsAsc.Length - startIndex.Value, _memoryComparer);
                for (var i = startIndex.Value; i < partsAsc.Length; i++)
                {
                    var suffix = partsAsc.Span[i].Code[^suffixLength..];
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
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static void BackwardFill(int?[] array)
    {
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
            }
        }
    }
}

public class MemoryComparer : IEqualityComparer<ReadOnlyMemory<byte>>, IAlternateEqualityComparer<ReadOnlySpan<byte>, ReadOnlyMemory<byte>>
{
    // In our case we will never add using ReadOnlySpan<byte>
    public ReadOnlyMemory<byte> Create(ReadOnlySpan<byte> alternate)
        => throw new NotImplementedException();

    public bool Equals(ReadOnlyMemory<byte> x, ReadOnlyMemory<byte> y)
    {
        if (x.Length != y.Length)
        {
            return false;
        }

        for (int i = 0; i < x.Length; i++)
        {
            if (x.Span[i] != y.Span[i])
            {
                return false;
            }
        }

        return true;
    }

    public bool Equals(ReadOnlySpan<byte> alternate, ReadOnlyMemory<byte> other)
    {
        if (alternate.Length != other.Span.Length)
        {
            return false;
        }

        for (int i = 0; i < alternate.Length; i++)
        {
            if (alternate[i] != other.Span[i])
            {
                return false;
            }
        }

        return true;
    }

    public int GetHashCode([DisallowNull] ReadOnlyMemory<byte> obj)
    {
        int hash = 16777619;
        for (int i = 0; i < obj.Length; i++)
        {
            hash = (hash * 31) ^ obj.Span[i];
        }
        return hash;
    }

    public int GetHashCode(ReadOnlySpan<byte> alternate)
    {
        int hash = 16777619;
        for (int i = 0; i < alternate.Length; i++)
        {
            hash = (hash * 31) ^ alternate[i];
        }
        return hash;
    }
}
