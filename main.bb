final fmt_generator(specs) = {
  fmt = new{
    specs |= str;  // ensure str type for specs, make them
    \call(x) = {return x[this.specs];}
  }
  return fmt;
}

print(1.2345 | fmt_generator(".3f"));