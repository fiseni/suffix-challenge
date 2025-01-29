using v2;

sealed class ResultBuilder
{
    private readonly string _resultFile;
    private readonly byte[] _block;
    private int _blockIndex = 0;
    private int _matchCount = 0;

    public ResultBuilder(string resultFile, int lineCount)
    {
        // Two record per line. Each record is max 49 chars + CR + LC + separator
        _block = new byte[(Constants.MAX_STRING_LENGTH * 2 + 3) * lineCount];
        _resultFile = resultFile;
    }

    public void AddMatch(ReadOnlyMemory<byte> partCode, ReadOnlyMemory<byte>? match)
    {
        partCode.Span.CopyTo(_block.AsSpan(_blockIndex));
        _block[_blockIndex + partCode.Length] = Constants.SEMICOLON;
        _blockIndex += partCode.Length + 1;

        if (match is not null)
        {
            match.Value.Span.CopyTo(_block.AsSpan(_blockIndex));
            _blockIndex += match.Value.Length;
            _matchCount++;
        }

        _block[_blockIndex++] = Constants.CR;
        _block[_blockIndex++] = Constants.LF;
    }

    public void PrintMatchCount()
    {
        Console.WriteLine(_matchCount);
    }

    public void WriteToFile()
    {
        File.WriteAllBytes(_resultFile, _block.AsSpan(0, _blockIndex));
    }
}
