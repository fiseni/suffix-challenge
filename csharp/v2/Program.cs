using System.Text;
using v2;

Run(args);

static void Run(string[] args)
{
    if (args.Length < 3)
    {
        Console.WriteLine("Usage: <parts file> <masterParts file> <results file>");
        return;
    }

    var sourceData = new SourceData(args[0], args[1]);
    var processor = new Processor(sourceData);

    var partsOriginal = sourceData.PartsOriginal;
    var results = new string[partsOriginal.Length];
    var matchCount = 0;

    for (int i = 0; i < partsOriginal.Length; i++)
    {
        var match = processor.FindMatch(partsOriginal[i]);
        if (match is not null)
        {
            matchCount++;
            results[i] = $"{Encoding.ASCII.GetString(partsOriginal[i].ToArray())};{Encoding.ASCII.GetString(match.Value.ToArray())}";
        }
        else
        {
            results[i] = $"{Encoding.ASCII.GetString(partsOriginal[i].ToArray())};";
        }
    }

    File.WriteAllLines(args[2], results);
    Console.WriteLine(matchCount);
}
