namespace v2;

public static class Constants
{
    public const int MIN_STRING_LENGTH = 3;
    public const int MAX_STRING_LENGTH = 50;

    public const byte LF = 10;
    public const byte CR = 13;
    public const byte SPACE = 32;
    public const byte HYPHEN = 45;
    public const byte SEMICOLON = 59;
}

public static class Utils
{
    public static ReadOnlySpan<byte> GetUpperRecord(ReadOnlyMemory<byte> source, byte[] destination)
    {
        var sourceSpan = source.Span;
        for (int i = 0; i < sourceSpan.Length; i++)
        {
            destination[i] = (uint)(sourceSpan[i] - 97) <= 25 // 25 = (uint)(122 - 97)
                ? (byte)(sourceSpan[i] & 0x5F)
                : sourceSpan[i];
        }
        return destination.AsSpan()[..sourceSpan.Length];
    }

    public static ReadOnlyMemory<byte> GetUpperRecord(ReadOnlyMemory<byte> source, byte[] destination, int startIndex)
    {
        var sourceSpan = source.Span;
        var destinationSpan = destination.AsSpan(startIndex, sourceSpan.Length);

        for (int i = 0; i < sourceSpan.Length; i++)
        {
            destinationSpan[i] = (uint)(sourceSpan[i] - 97) <= 25 // 25 = (uint)(122 - 97)
                ? (byte)(sourceSpan[i] & 0x5F)
                : sourceSpan[i];
        }
        return new ReadOnlyMemory<byte>(destination, startIndex, destinationSpan.Length);
    }

    public static ReadOnlyMemory<byte> GetTrimmedRecord(byte[] source, int startIndex, int length)
    {
        var sourceSpan = source.AsSpan(startIndex, length);
        var start = ClampStart(sourceSpan);
        var end = ClampEnd(sourceSpan, start);
        return new ReadOnlyMemory<byte>(source, startIndex + start, end);

        static int ClampStart(ReadOnlySpan<byte> span)
        {
            int start = 0;
            for (; start < span.Length; start++)
                if (span[start] != Constants.SPACE) break;
            return start;
        }

        // Initially, start==len==0. If ClampStart trims all, start==len
        static int ClampEnd(ReadOnlySpan<byte> span, int start)
        {
            int end = span.Length - 1;
            for (; end >= start; end--)
                if (span[end] != Constants.SPACE) break;
            return end - start + 1;
        }
    }

    public static ReadOnlyMemory<byte> GetNoHyphensRecord(ReadOnlyMemory<byte> source, byte[] destination, int startIndex)
    {
        var sourceSpan = source.Span;
        var destinationSpan = destination.AsSpan(startIndex, sourceSpan.Length);

        var j = 0;
        for (int i = 0; i < sourceSpan.Length; i++)
        {
            if (sourceSpan[i] != Constants.HYPHEN)
            {
                destinationSpan[j++] = sourceSpan[i];
            }
        }
        return new ReadOnlyMemory<byte>(destination, startIndex, j);
    }

    public static int GetLineCount(ReadOnlySpan<byte> span)
    {
        var lines = 0;
        for (int i = 0; i < span.Length; i++)
        {
            if (span[i] == Constants.LF)
                lines++;
        }
        // Handle the case where the last line might not end with a newline
        if (span.Length > 1 && span[^1] != Constants.LF)
            lines++;

        return lines;
    }
}
