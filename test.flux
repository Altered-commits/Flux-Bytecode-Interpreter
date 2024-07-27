include Flux.IO
include Flux.Time

Func<Void> Fibonnaci(Int n)
{
    Int a, b = 1, c;
    Print(c);
    
    For i in 2..(n + 1) {
        c = a + b;
        a = b;
        b = c;

        Print(a);
    }
}

Auto start = GetTimeNs();

Fibonnaci(20);

Auto end   = GetTimeNs();

Print((end - start) / 1000);