using v2;

#if DEBUG
var debugArgs = new string[] { "../../../../../data/parts.txt", "../../../../../data/master-parts.txt", "results.txt" };
Run(debugArgs);
return;
#endif

GC.TryStartNoGCRegion(1000 * 1024 * 1024);
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
    var resultBuilder = new ResultBuilder(args[2], partsOriginal.Length);

    for (int i = 0; i < partsOriginal.Length; i++)
    {
        var partOriginal = partsOriginal[i];
        var match = processor.FindMatch(partOriginal);
        resultBuilder.AddMatch(partOriginal, match);
    }

    resultBuilder.PrintMatchCount();
    resultBuilder.WriteToFile();
}
