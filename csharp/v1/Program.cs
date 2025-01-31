using v1;

if (args.Length < 3)
{
    Console.WriteLine("Usage: <parts file> <masterParts file> <results file>");
    return;
}

var sourceData = SourceData.Load(args[0], args[1]);
var processor = new Processor(sourceData);

var parts = sourceData.Parts;
var results = new string[parts.Length];
var matchCount = 0;

for (int i = 0; i < parts.Length; i++)
{
    var match = processor.FindMatch(parts[i].Code);
    if (match is not null)
    {
        matchCount++;
        results[i] = $"{parts[i].Code};{match.Code}";   
    }
    else
    {
        results[i] = $"{parts[i].Code};";
    }
}

File.WriteAllLines(args[2], results);
Console.WriteLine(matchCount);
