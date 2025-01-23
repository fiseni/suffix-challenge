namespace version1;

public class Part
{
    public string Code { get; }

    public Part(string code)
    {
        Code = code.Trim();
    }
}


public class MasterPart
{
    public string Code { get; }
    public string? CodeNoHyphens { get; }

    public MasterPart(string code)
    {
        Code = code.Trim();
        CodeNoHyphens = code.Contains('-') ? Code.Replace("-", "") : null;
    }
}

public class SourceData
{
    public Part[] Parts { get; }
    public MasterPart[] MasterParts { get; }

    public SourceData(Part[] parts, MasterPart[] masterParts)
    {
        Parts = parts;
        MasterParts = masterParts;
    }

    public static SourceData Load(string partsFilePath, string masterPartsFilePath)
    {
        var partLines = File.ReadAllLines(partsFilePath);
        var masterPartLines = File.ReadAllLines(masterPartsFilePath);

        var parts = partLines
            .Select(x => new Part(x))
            .ToArray();

        var masterParts = masterPartLines
            .Select(x => new MasterPart(x))
            .Where(x => x.Code.Length > 2)
            .OrderBy(x => x.Code.Length)
            .ToArray();

        return new SourceData(parts, masterParts);
    }
}
