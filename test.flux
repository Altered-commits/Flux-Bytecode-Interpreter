Int res = 998; //Max recursion limit for now

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
        Return Summation(10, 20, 30, 40);
    }
    res = res - 1;
    Return doFunc();
}


doFunc() + 0;