Func<Auto> Summation(Auto ...)
{
    Auto result;
    For i in ... {
        If (i is Int) {
            result = result + i;
        }
        Elif (i is Float) {
            result = result + i + 0.5;
        }
    }

    Return result;
}

Summation(1, 2.5, 3, 4.5, 5, 6.5, 7, 8.5) + 0;