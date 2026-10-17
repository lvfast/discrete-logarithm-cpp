# Discrete Logarithm Cryptography in C++

An educational C++ lab implementing number theory and cryptographic operations associated with the discrete logarithm problem: primitive root checking, Diffie-Hellman key exchange, ElGamal decryption, and ElGamal signature verification.

Each program implements its own unsigned big-integer arithmetic using `std::vector<uint64_t>`, with 64-bit limbs and no external big-integer library. This repository preserves the original C++ source files unchanged and adds project organization and build documentation. It does not implement a discrete logarithm solver.

## Programs

| Executable | Purpose | Original task |
| --- | --- | --- |
| `primitive_root` | Check a candidate generator using supplied prime factors of `p - 1` | `project_02_01` |
| `diffie_hellman` | Compute two public values and a shared secret | `project_02_02` |
| `elgamal_decrypt` | Compute the public key value and decrypt a ciphertext pair | `project_02_03` |
| `elgamal_verify` | Check an ElGamal signature equation and component ranges | `project_02_04` |

Topics include limb-based arithmetic, carry and borrow handling, modular multiplication, modular exponentiation, and modular inversion using Fermat's theorem.

## Structure

```text
.
├── README.md
├── CMakeLists.txt
├── .gitignore
├── src/
│   ├── primitive_root/main.cpp
│   ├── diffie_hellman/main.cpp
│   ├── elgamal_decrypt/main.cpp
│   └── elgamal_verify/main.cpp
└── docs/
    └── 231274448_Report.pdf
```

The four executables are independent. Shared-looking arithmetic code remains in each original file.

## Build

Use a C++11-compatible compiler and run commands from the repository root.

### GCC / MinGW

Create the build directory with `New-Item -ItemType Directory -Force build` in PowerShell, or `mkdir -p build` in a POSIX shell, then run:

```sh
g++ -std=c++11 -O2 src/primitive_root/main.cpp -o build/primitive_root
g++ -std=c++11 -O2 src/diffie_hellman/main.cpp -o build/diffie_hellman
g++ -std=c++11 -O2 src/elgamal_decrypt/main.cpp -o build/elgamal_decrypt
g++ -std=c++11 -O2 src/elgamal_verify/main.cpp -o build/elgamal_verify
```

On Windows, GCC produces executables with the `.exe` extension.

### CMake (optional)

With CMake 3.16 or later and a compatible C++ toolchain installed:

```sh
cmake -S . -B build
cmake --build build --config Release
```

For MinGW, select its generator on the first configuration:

```sh
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

Executables are commonly in `build/` for single-configuration generators, or `build/Release/` for Visual Studio. The CMake configuration has not been executed in the preparation environment; direct GCC builds were used for verification.

## Input and output

All numeric inputs use **reversed hexadecimal digits**, including the factor count in the primitive-root program. This reverses individual hex characters, not bytes. For example, decimal 23 is conventional hex `17`, entered as `71`; decimal 19 is hex `13`, entered as `31`.

Inputs are whitespace-separated; the examples use one value per line except the factor list. Numeric outputs use uppercase reversed hex. Boolean outputs are `1` or `0`.

All four programs accept an input path and an output path:

```sh
./build/primitive_root input.inp result.out
./build/diffie_hellman input.inp result.out
./build/elgamal_decrypt input.inp result.out
./build/elgamal_verify input.inp result.out
```

In PowerShell, use paths such as `.\build\primitive_root.exe input.inp result.out`.

### Primitive root check

Input order: `p`, factor count `n`, the `n` distinct prime factors of `p - 1`, then candidate `g`. Supply an odd prime `p`, the complete factor list, and `1 <= g < p`. These preconditions are not fully validated by the program.

Example: `p = 23`, factors `2, 11`, candidate `g = 5`.

```text
71
2
2 B
5
```

Expected output: `1`.

### Diffie-Hellman

Input order: `p`, `g`, private exponent `a`, private exponent `b`. Output order: `A = g^a mod p`, `B = g^b mod p`, `K = A^b mod p`.

Example: `p = 23`, `g = 5`, `a = 6`, `b = 15`.

```text
71
5
6
F
```

Expected output, representing decimal `8`, `19`, and `2`:

```text
8
31
2
```

### ElGamal decryption

Input order: `p`, `g`, private exponent `x`, ciphertext `c1`, ciphertext `c2`. Output order: public value `h = g^x mod p`, decrypted message `m = c2 * (c1^x)^-1 mod p`.

Example: `p = 23`, `g = 5`, `x = 6`, `c1 = 10`, `c2 = 19`.

```text
71
5
6
A
31
```

Expected output:

```text
8
7
```

### ElGamal signature verification

Input order: `p`, `g`, public value `y`, message representative `m`, signature component `r`, signature component `h`. The name `h` here denotes the second signature component, not the public value used in the decryption program.

The program requires `0 < r < p`, `0 < h < p - 1`, and checks `g^m mod p == (y^r * r^h) mod p`.

Example: `p = 23`, `g = 5`, `y = 8`, `m = 13`, `r = 17`, `h = 3`.

```text
71
5
8
D
11
3
```

Expected output: `1`.

## Scope and limitations

This is an academic implementation. The Diffie-Hellman program is a local arithmetic simulation; it does not communicate over a network. ElGamal inputs are supplied in files, and there is no secure key generation, message hashing pipeline, or complete messaging application.

Use valid mathematical inputs: an odd prime modulus, a suitable generator, valid exponents, and ciphertext residues in range with nonzero `c1`. The programs do not fully validate these assumptions. Hex parsing silently skips invalid characters.

The original arithmetic has not undergone comprehensive correctness testing. Source review identifies borrow propagation and unsigned division-search boundary cases that need further testing. Reorganization preserves these implementations without algorithm changes; small examples do not establish correctness for arbitrary-size values.

## Report

The original lab report is available at [docs/231274448_Report.pdf](docs/231274448_Report.pdf).
