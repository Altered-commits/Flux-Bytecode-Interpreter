include Flux.Common.flux
include Flux.Math.flux

Int j = -1;

Func<Int> Fibonacci(Int N)
{
    If (N == 1 || N == 0) {
        Return N;
    }
    Return Fibonacci(N - 1) + Fibonacci(N - 2);
}

Func<Int> Min(Int a, Int b)
{
    Return a < b ? a : b;
}

Func<Int> Max(Int a, Int b)
{
    Return a > b ? a : b;
}

Func<Int> doFunc(Int a, Int b)
{
    For i in a..b {
        If(LCM(i, b - a) > 1000) {
            Return i;
        }
    }
    Return -10;
}

Any res = doFunc(0, 4);

If (res is Int && res == -10) {
    j = 1;
}

j;