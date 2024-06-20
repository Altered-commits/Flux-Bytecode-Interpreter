include Flux.Common.flux

//Common defines found in math libraries
define M_E        2.718281828459045
define M_LOG2E    1.4426950408889634
define M_LOG10E   0.4342944819032518
define M_LN2      0.6931471805599453
define M_LN10     2.302585092994046
define M_PI       3.141592653589793
define M_PI_2     1.5707963267948966
define M_PI_4     0.7853981633974483
define M_1_PI     0.3183098861837907
define M_2_PI     0.6366197723675814
define M_2_SQRTPI 1.1283791670955126
define M_SQRT2    1.4142135623730951
define M_SQRT1_2  0.7071067811865476

//Testing functions for making standard library
Func<Int> Pow(Int base, Int power)
{
    Return base ^ power;
}

Func<Int> Sqrt(Int base, Int root)
{
    Return Pow(base, -root);
}

Func<Int> Abs(Int value)
{
    Return value < 0 ? -value : value;
}

Func<Int> Factorial(Int N)
{
    Return (N < 1 || N == 1) ? N : N * Factorial(N - 1);
}

Func<Int> GCD(Int a, Int b)
{
    If (b == 0) {
        Return a;
    }
    Return GCD(b, a % b);
}

Func<Int> LCM(Int a, Int b)
{
    Return (a * b) / GCD(a, b);
}