using System.Text;

namespace v2;

public static class App
{
    public static void Run(string[] args)
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
        Console.WriteLine(matchCount);
    }
}
