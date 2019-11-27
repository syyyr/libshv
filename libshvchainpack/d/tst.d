import std.stdio;

void main()
{
    // Read standard input 4KB at a time
    foreach (ubyte[] buffer; stdin.byChunk(4096))
    {
        writeln(buffer);
    }
}