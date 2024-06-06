include Flux.CommonDefs.flux

Int num = 4;
Int isPrime = TRUE;

If (num <= 1) {
    isPrime = FALSE;
} Elif (num == 2) {
    isPrime = TRUE;
} Else {
    For i in 2..(num / 2)+1 {
        If (num % i == 0) {
            isPrime = FALSE;
            Break;
        }
    }
}

isPrime;