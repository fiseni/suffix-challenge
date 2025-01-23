using System.Runtime.CompilerServices;

namespace v1;

public class Processor
{
    private const int MIN_STRING_LENGTH = 3;
    private const int MAX_STRING_LENGTH = 50;

    private readonly Dictionary<string, MasterPart?> _masterPartsByPartCode;

    public Processor(SourceData sourceData)
    {
        var masterPartsInfo = new MasterPartsInfo(sourceData.MasterParts);
        var partsInfo = new PartsInfo(sourceData.Parts);

        _masterPartsByPartCode = BuildDictionary(masterPartsInfo, partsInfo);
    }

    public MasterPart? FindMatch(string partCode)
    {
        var match = _masterPartsByPartCode.GetValueOrDefault(partCode);
        return match;
    }

    private static Dictionary<string, MasterPart?> BuildDictionary(MasterPartsInfo masterPartsInfo, PartsInfo partsInfo)
    {
        var masterPartsByPartCode = new Dictionary<string, MasterPart?>(partsInfo.Parts.Length, StringComparer.OrdinalIgnoreCase);

        for (var i = 0; i < partsInfo.Parts.Length; i++)
        {
            var partCode = partsInfo.Parts[i].Code;

            var masterPartsBySuffix = masterPartsInfo.SuffixesByLength[partCode.Length];
            var match = masterPartsBySuffix?.GetValueOrDefault(partCode);
            if (match is not null)
            {
                masterPartsByPartCode.TryAdd(partCode, match);
                continue;
            }

            masterPartsBySuffix = masterPartsInfo.SuffixesByNoHyphensLength[partCode.Length];
            match = masterPartsBySuffix?.GetValueOrDefault(partCode);
            if (match is not null)
            {
                masterPartsByPartCode.TryAdd(partCode, match);
            }
        }

        for (var i = masterPartsInfo.MasterParts.Length - 1; i >= 0; i--)
        {
            var masterPart = masterPartsInfo.MasterParts[i];

            var partsBySuffix = partsInfo.SuffixesByLength[masterPart.Code.Length];
            var originalPartIndices = partsBySuffix?.GetValueOrDefault(masterPart.Code);
            if (originalPartIndices is not null)
            {
                for (var j = originalPartIndices.Count - 1; j >= 0; j--)
                {
                    var index = originalPartIndices[j];
                    masterPartsByPartCode.TryAdd(partsInfo.Parts[index].Code, masterPart);
                }
            }
        }

        return masterPartsByPartCode;
    }

    private sealed class MasterPartsInfo
    {
        public MasterPart[] MasterParts { get; }
        public MasterPart[] MasterPartsNoHyphens { get; }
        public Dictionary<string, MasterPart>?[] SuffixesByLength { get; }
        public Dictionary<string, MasterPart>?[] SuffixesByNoHyphensLength { get; }

        public MasterPartsInfo(MasterPart[] masterParts)
        {
            MasterParts = masterParts;

            MasterPartsNoHyphens = masterParts
                .Where(x => x.CodeNoHyphens is not null && x.Code.Length > 2)
                .OrderBy(x => x.CodeNoHyphens!.Length)
                .ToArray();

            SuffixesByLength = new Dictionary<string, MasterPart>?[MAX_STRING_LENGTH];
            SuffixesByNoHyphensLength = new Dictionary<string, MasterPart>?[MAX_STRING_LENGTH];

            BuildSuffixDictionaries(SuffixesByLength, MasterParts, false);
            BuildSuffixDictionaries(SuffixesByNoHyphensLength, MasterPartsNoHyphens, true);
        }

        private static void BuildSuffixDictionaries(Dictionary<string, MasterPart>?[] suffixesByLength, MasterPart[] masterParts, bool useNoHyphen)
        {
            // Create and populate start indices.
            var startIndexesByLength = new int?[MAX_STRING_LENGTH];
            for (var i = 0; i < MAX_STRING_LENGTH; i++)
            {
                startIndexesByLength[i] = null;
            }
            for (var i = 0; i < masterParts.Length; i++)
            {
                var length = useNoHyphen
                    ? masterParts[i].CodeNoHyphens!.Length
                    : masterParts[i].Code.Length;

                if (startIndexesByLength[length] is null)
                    startIndexesByLength[length] = i;
            }
            BackwardFill(startIndexesByLength);

            // Create and populate suffix dictionaries.
            Parallel.For(MIN_STRING_LENGTH, MAX_STRING_LENGTH, length =>
            {
                var startIndex = startIndexesByLength[length];
                if (startIndex is not null)
                {
                    var tempDictionary = new Dictionary<string, MasterPart>(masterParts.Length - startIndex.Value, StringComparer.OrdinalIgnoreCase);
                    var altLookup = tempDictionary.GetAlternateLookup<ReadOnlySpan<char>>();
                    for (var i = startIndex.Value; i < masterParts.Length; i++)
                    {
                        var suffix = useNoHyphen
                            ? masterParts[i].CodeNoHyphens!.AsSpan()[^length..]
                            : masterParts[i].Code.AsSpan()[^length..];
                        altLookup.TryAdd(suffix, masterParts[i]);
                    }
                    suffixesByLength[length] = tempDictionary;
                }
            });
        }
    }

    private sealed class PartsInfo
    {
        public Part[] Parts { get; }
        public Dictionary<string, List<int>>?[] SuffixesByLength { get; }

        public PartsInfo(Part[] parts)
        {
            Parts = parts
                .Where(x => x.Code.Length > 2)
                .OrderBy(x => x.Code.Length)
                .ToArray();

            SuffixesByLength = new Dictionary<string, List<int>>?[MAX_STRING_LENGTH];
            BuildSuffixDictionaries(SuffixesByLength, Parts);
        }

        private static void BuildSuffixDictionaries(Dictionary<string, List<int>>?[] suffixesByLength, Part[] parts)
        {
            // Create and populate start indices.
            var startIndexByLength = new int?[MAX_STRING_LENGTH];
            for (var i = 0; i < MAX_STRING_LENGTH; i++)
            {
                startIndexByLength[i] = null;
            }
            for (var i = 0; i < parts.Length; i++)
            {
                var length = parts[i].Code.Length;
                if (startIndexByLength[length] is null)
                    startIndexByLength[length] = i;
            }
            BackwardFill(startIndexByLength);

            // Create and populate suffix dictionaries.
            Parallel.For(MIN_STRING_LENGTH, MAX_STRING_LENGTH, length =>
            {
                var startIndex = startIndexByLength[length];
                if (startIndex is not null)
                {
                    var tempDictionary = new Dictionary<string, List<int>>(parts.Length - startIndex.Value, StringComparer.OrdinalIgnoreCase);
                    var altLookup = tempDictionary.GetAlternateLookup<ReadOnlySpan<char>>();
                    for (var i = startIndex.Value; i < parts.Length; i++)
                    {
                        var suffix = parts[i].Code.AsSpan()[^length..];
                        if (altLookup.TryGetValue(suffix, out var originalPartIndices))
                        {
                            originalPartIndices.Add(i);
                        }
                        else
                        {
                            altLookup.TryAdd(suffix, [i]);
                        }
                    }
                    suffixesByLength[length] = tempDictionary;
                }
            });
        }
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static void BackwardFill(int?[] array)
    {
        var temp = array[MAX_STRING_LENGTH - 1];
        for (var i = MAX_STRING_LENGTH - 1; i >= 0; i--)
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
