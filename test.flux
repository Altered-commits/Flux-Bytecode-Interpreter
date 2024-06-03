include Flux.MathDefs.flux

Float radius = 5.0,
      circumference = 0.0,
      area = Cast<Float>(FALSE);

If (radius <= 10) {
    circumference = 2 * M_PI * radius;
}
Else {
    area = M_PI * radius * radius;
}

circumference;