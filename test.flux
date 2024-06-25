include Flux.Math

Auto res = 100;

Func<Auto> Factorial(Auto N)
{
    Return (N < 1 || N == 1) ? N : N * Factorial(N - 1);
}

Func<Auto> Summation(Auto ...)
{
    Auto sum;
    
    For i in ... {
        sum = sum + i;
    }

    Return sum;
}

Func<Auto> doFunc()
{
    If (res == 0) {
        Return Abs(LCM(Factorial(Summation(1, 2, 3, 4, 5)), Factorial(Summation(1, 2, 3, 4, 5))));
    }
    res = res - 1;
    Return doFunc();
}

doFunc() + 0;