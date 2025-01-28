using ManagedObjectSize;
using System.Runtime.CompilerServices;
using System.Text;
using v2;

#if DEBUG
var testArgs = new string[] { "../../../../../data/parts.txt", "../../../../../data/master-parts.txt", "results.txt" };
//var testArgs = new string[] { "../../../testParts.txt", "../../../testMasterParts.txt", "results.txt" };
    Run(testArgs);
#else
    Run(args);
#endif

//var data = new SourceData("../../../testParts.txt", "../../../testParts.txt");

{ }

return;

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




//var data = new SourceData("../../../testParts.txt", "masterparts.txt");

//var data = new SourceData("../../../parts.txt", "masterparts.txt");
//var parts1 = data.PartsAsc.Select(x => Encoding.ASCII.GetString(x.Code.ToArray())).ToArray();
//File.WriteAllLines("parts1", parts1);

//var data2 = File.ReadAllLines("../../../parts.txt")
//    .Select(x => x.Trim())
//    .OrderBy(x => x.Length)
//    .ToArray();
//File.WriteAllLines("parts2", data2);


//Console.WriteLine();
//var x1 = string.Empty;
//var x2 = "a";
//var x3 = "ab";
var x4 = "abcde";

//SizeHelper.Print(x1);
//SizeHelper.Print(x2);
//SizeHelper.Print(x3);
//SizeHelper.Print(x4);

//var y1 = x1.AsMemory();
//var y2 = x2.AsMemory();
//var y3 = x3.AsMemory();
var y4 = x4.AsMemory();

//var z1 = Encoding.ASCII.GetString(y4.ToArray());
//var bytes = Encoding.ASCII.GetBytes(x4);
//var bytesSpan = bytes.AsSpan();

//var bytesMemory = bytes.AsMemory();

//var qwe = y4.ToString();

//SizeHelper.Print(y1);
//SizeHelper.Print(y2);
//SizeHelper.Print(y3);
//SizeHelper.Print(y4);

public static class SizeHelper
{
    public static void Print(object obj, [CallerArgumentExpression(nameof(obj))] string caller = "")
    {
        Console.WriteLine("");
        Console.WriteLine(caller);
        Console.WriteLine($"Exclusive: {ObjectSize.GetObjectExclusiveSize(obj):N0}");
        Console.WriteLine($"Inclusive: {ObjectSize.GetObjectInclusiveSize(obj):N0}");
    }

    public static void Print(object obj, IEnumerable<object> deductSizes, [CallerArgumentExpression(nameof(obj))] string caller = "")
    {
        var deductSize = deductSizes.Sum(x => ObjectSize.GetObjectInclusiveSize(x));

        Console.WriteLine("");
        Console.WriteLine(caller);
        Console.WriteLine($"Exclusive: {ObjectSize.GetObjectExclusiveSize(obj):N0}");
        Console.WriteLine($"Inclusive: {ObjectSize.GetObjectInclusiveSize(obj) - deductSize:N0}");
    }
}
