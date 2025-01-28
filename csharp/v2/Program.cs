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
    var resultBuilder = new ResultBuilder(args[2], partsOriginal.Length);

    for (int i = 0; i < partsOriginal.Length; i++)
    {
        var match = processor.FindMatch(partsOriginal[i]);
        resultBuilder.AddMatch(partsOriginal[i], match);
    }

    resultBuilder.PrintMatchCount();
    resultBuilder.WriteToFile();
}
