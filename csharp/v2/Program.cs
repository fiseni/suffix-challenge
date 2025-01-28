using System.Diagnostics;
using v2;

var stopwatch = new Stopwatch();
stopwatch.Start();

#if DEBUG
var testArgs = new string[] { "../../../../../data/parts.txt", "../../../../../data/master-parts.txt", "results.txt" };
//var testArgs = new string[] { "../../../../../data/test-parts.txt", "../../../../../data/test-master-parts.txt", "results.txt" };
App.Run(testArgs);
#else
    App.Run(args);
#endif

stopwatch.Stop();
Console.WriteLine($"Wall time: {stopwatch.ElapsedMilliseconds:n0}");
