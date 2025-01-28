using System.Text;
using v2;

#if DEBUG
var testArgs = new string[] { "../../../../../data/parts.txt", "../../../../../data/master-parts.txt", "results.txt" };
//var testArgs = new string[] { "../../../../../data/test-parts.txt", "../../../../../data/test-master-parts.txt", "results.txt" };
Run(testArgs);
#else
    Run(args);
#endif

static void Run(string[] args)
{
    if (args.Length < 3)
    {
        Console.WriteLine("Usage: <parts file> <masterParts file> <results file>");
        return;
    }

    var sourceData = new SourceData(args[0], args[1]);
    var processor = new Processor(sourceData);

    var parts = sourceData.PartsOriginal;
    var results = new string[parts.Length];
    var matchCount = 0;

    for (int i = 0; i < parts.Length; i++)
    {
        var match = processor.FindMatch(parts[i]);
        if (match is not null)
        {
            matchCount++;
            results[i] = $"{Encoding.ASCII.GetString(parts[i].ToArray())};{Encoding.ASCII.GetString(match.Value.ToArray())}";
        }
        else
        {
            results[i] = $"{Encoding.ASCII.GetString(parts[i].ToArray())};";
        }
    }

    File.WriteAllLines(args[2], results);
    Console.Write(matchCount);
}
