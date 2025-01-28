using System.Diagnostics.CodeAnalysis;

namespace v2;

public class ReadOnlyMemoryByteComparer : IEqualityComparer<ReadOnlyMemory<byte>>, IAlternateEqualityComparer<ReadOnlySpan<byte>, ReadOnlyMemory<byte>>
{
    // In our case we will never add using ReadOnlySpan<byte>
    public ReadOnlyMemory<byte> Create(ReadOnlySpan<byte> alternate)
        => throw new NotImplementedException();

    public bool Equals(ReadOnlyMemory<byte> x, ReadOnlyMemory<byte> y)
    {
        if (x.Length != y.Length)
        {
            return false;
        }

        for (int i = 0; i < x.Length; i++)
        {
            if (x.Span[i] != y.Span[i])
            {
                return false;
            }
        }

        return true;
    }

    public bool Equals(ReadOnlySpan<byte> alternate, ReadOnlyMemory<byte> other)
    {
        if (alternate.Length != other.Span.Length)
        {
            return false;
        }

        for (int i = 0; i < alternate.Length; i++)
        {
            if (alternate[i] != other.Span[i])
            {
                return false;
            }
        }

        return true;
    }

    public int GetHashCode([DisallowNull] ReadOnlyMemory<byte> obj)
    {
        int hash = 16777619;
        for (int i = 0; i < obj.Length; i++)
        {
            hash = (hash * 31) ^ obj.Span[i];
        }
        return hash;
    }

    public int GetHashCode(ReadOnlySpan<byte> alternate)
    {
        int hash = 16777619;
        for (int i = 0; i < alternate.Length; i++)
        {
            hash = (hash * 31) ^ alternate[i];
        }
        return hash;
    }
}
