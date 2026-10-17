#include <iostream>  // Cho cout, cerr, endl, ios::sync_with_stdio, cin.tie
#include <fstream>   // Cho ifstream, ofstream
#include <string>    // Cho std::string, getline
#include <vector>    // Cho std::vector
#include <algorithm> // Cho std::reverse, std::max
#include <cstdint>   // Cho uint64_t, uint32_t, __uint128_t
#include <cmath>
#include <sstream>
#include <iomanip>

using namespace std;

/***  BigInt  ***/
struct Big
{
    /*
    mỗi v[i] là 1 limb theo base 2^64
    ví dụ:
    2^64: v[0] = 0, v[1] = 1. => 0x(2^64)^0 + 1x(2^64)^1
    */
    vector<uint64_t> v;

    Big() {}
    explicit Big(uint64_t x)
    {
        if (x)
            v.push_back(x);
    }

    bool isZero() const { return v.empty(); }
    bool isEven() const { return isZero() || ((v[0] & 1ULL) == 0ULL); }

    void normalize()
    {
        while (!v.empty() && v.back() == 0ULL)
            v.pop_back();
    }

    // đi từ cuối mảng về đầu (vì lớn nhất ở cuối)
    // nếu limb của thằng nào lớn hơn thằng kia thì thằng đó lớn hơn
    static int cmp(const Big &a, const Big &b)
    {
        if (a.v.size() != b.v.size())
            return (a.v.size() < b.v.size()) ? -1 : 1;
        for (int i = (int)a.v.size() - 1; i >= 0; --i)
        {
            if (a.v[i] != b.v[i])
                return (a.v[i] < b.v[i]) ? -1 : 1;
        }
        return 0;
    }

    // a + b
    static Big add(const Big &a, const Big &b)
    {
        Big r;
        r.v.resize(max(a.v.size(), b.v.size()) + 1, 0);
        uint64_t carry = 0;

        for (size_t i = 0; i < r.v.size(); ++i)
        {
            uint64_t ai = (i < a.v.size() ? a.v[i] : 0ULL);
            uint64_t bi = (i < b.v.size() ? b.v[i] : 0ULL);

            // Thực hiện cộng thủ công 64-bit có carry
            uint64_t sum = ai + bi;
            uint64_t newCarry = (sum < ai) ? 1 : 0; // nếu tràn thì carry = 1

            sum += carry;
            if (carry && sum == 0)
                newCarry = 1; // nếu cộng carry gây tràn thêm 1 lần nữa

            r.v[i] = sum;
            carry = newCarry;
        }

        r.normalize();
        return r;
    }

    // a += small (0..15)
    void addSmall(uint32_t d)
    {
        uint64_t carry = d;
        size_t i = 0;

        while (carry)
        {
            if (i == v.size())
                v.push_back(0);

            uint64_t prev = v[i];
            uint64_t sum = prev + carry;

            // Kiểm tra tràn 64-bit
            carry = (sum < prev) ? 1 : 0;

            v[i] = sum;
            ++i;
        }
    }

    // r = a - b, requires a >= b
    static Big sub(const Big &a, const Big &b)
    {
        Big r;
        r.v.resize(a.v.size(), 0);
        uint64_t borrow = 0;

        for (size_t i = 0; i < a.v.size(); ++i)
        {
            uint64_t ai = a.v[i];
            uint64_t bi = (i < b.v.size() ? b.v[i] : 0ULL);
            uint64_t tmp = bi + borrow;

            // Nếu ai < bi + borrow → cần mượn
            if (ai < tmp)
            {
                r.v[i] = (uint64_t)((~0ULL) - (tmp - ai - 1)); // cộng thêm 2^64
                borrow = 1;
            }
            else
            {
                r.v[i] = ai - tmp;
                borrow = 0;
            }
        }

        r.normalize();
        return r;
    }

    // shift left by 4 bits (multiply by 16)
    void mul16()
    {
        if (v.empty())
            return;
        uint64_t carry = 0;
        for (size_t i = 0; i < v.size(); ++i)
        {
            uint64_t nv = (v[i] << 4) | carry;
            carry = (v[i] >> 60);
            v[i] = nv;
        }
        if (carry)
            v.push_back(carry);
    }

    // Nhân 2 số 64-bit trả về (low, high)
    static void mul64(uint64_t a, uint64_t b, uint64_t &low, uint64_t &high)
    {
        const uint64_t MASK32 = 0xFFFFFFFFULL;
        uint64_t a_lo = a & MASK32;
        uint64_t a_hi = a >> 32;
        uint64_t b_lo = b & MASK32;
        uint64_t b_hi = b >> 32;

        uint64_t lo_lo = a_lo * b_lo;
        uint64_t hi_lo = a_hi * b_lo;
        uint64_t lo_hi = a_lo * b_hi;
        uint64_t hi_hi = a_hi * b_hi;

        uint64_t carry = ((lo_lo >> 32) + (hi_lo & MASK32) + (lo_hi & MASK32)) >> 32;
        uint64_t mid = (hi_lo >> 32) + (lo_hi >> 32) + carry;
        high = hi_hi + mid;
        low = lo_lo + ((hi_lo & MASK32) << 32) + ((lo_hi & MASK32) << 32);
    }

    /*** Nhân hai BigInt (base 2^64) ***/
    static Big mul(const Big &a, const Big &b)
    {
        Big r;
        r.v.assign(a.v.size() + b.v.size(), 0);

        for (size_t i = 0; i < a.v.size(); ++i)
        {
            uint64_t carry = 0;
            for (size_t j = 0; j < b.v.size(); ++j)
            {
                uint64_t low, high;
                mul64(a.v[i], b.v[j], low, high); // gọi trực tiếp static mul64

                uint64_t sum = r.v[i + j] + low + carry;
                carry = (sum < low || sum < r.v[i + j]) ? 1 : 0;
                r.v[i + j] = sum;

                uint64_t next = r.v[i + j + 1] + high + carry;
                carry = (next < high) ? 1 : 0;
                r.v[i + j + 1] = next;
            }

            size_t pos = i + b.v.size();
            while (carry)
            {
                if (pos == r.v.size())
                    r.v.push_back(0);
                uint64_t sum = r.v[pos] + carry;
                carry = (sum < r.v[pos]) ? 1 : 0;
                r.v[pos++] = sum;
            }
        }

        r.normalize();
        return r;
    }

    // shift right by 1 bit (divide by 2, floor)
    void shr1()
    {
        uint64_t carry = 0;
        for (int i = (int)v.size() - 1; i >= 0; --i)
        {
            uint64_t newCarry = v[i] & 1ULL;
            v[i] = (v[i] >> 1) | (carry << 63);
            carry = newCarry;
        }
        normalize();
    }

    // compare to small (uint64)
    int cmpSmall(uint64_t x) const
    {
        if (v.size() > 1)
            return 1;
        uint64_t val = v.empty() ? 0ULL : v[0];
        if (val < x)
            return -1;
        if (val > x)
            return 1;
        return 0;
    }

    // set to n - 1 (assumes n > 0)
    static Big dec1(const Big &n)
    {
        Big r = n;
        if (r.v.empty())
            return r;
        size_t i = 0;
        while (true)
        {
            if (r.v[i] > 0)
            {
                r.v[i] -= 1;
                break;
            }
            r.v[i] = ~0ULL; // borrow
            ++i;
        }
        r.normalize();
        return r;
    }

    // res = (a + b) % mod, assuming 0 <= a,b < mod
    static Big addMod(const Big &a, const Big &b, const Big &mod)
    {
        Big s = add(a, b);
        if (cmp(s, mod) >= 0)
            s = sub(s, mod);
        return s;
    }

    // res = (2*a) % mod
    static Big doubleMod(const Big &a, const Big &mod)
    {
        Big s = add(a, a);
        if (cmp(s, mod) >= 0)
            s = sub(s, mod);
        return s;
    }

    // res = (a * b) % mod using double-and-add (no big multiplication)
    static Big mulMod(Big a, Big b, const Big &mod)
    {
        Big res; // zero
        // ensure a,b < mod (in this design they should be)
        while (!b.isZero())
        {
            if (!b.isEven())
            {
                res = addMod(res, a, mod);
            }
            a = doubleMod(a, mod);
            b.shr1();
        }
        return res;
    }

    // res = (base^exp) % mod, exp is Big
    static Big powMod(Big base, Big exp, const Big &mod)
    {
        Big res(1);
        // reduce base if needed (not strictly necessary if base < mod by design)
        if (cmp(base, mod) >= 0)
        {
            // reduce by repeated subtraction (rare path). Use simple long subtraction ladder:
            // bring base into [0, mod)
            while (cmp(base, mod) >= 0)
            {
                // subtract the largest shift of mod <= base
                // find shift
                int shift = (int)base.v.size() - (int)mod.v.size();
                if (shift < 0)
                    break;
                // make tmp = mod << (shift*64)
                Big tmp;
                tmp.v.assign(shift, 0);
                tmp.v.insert(tmp.v.end(), mod.v.begin(), mod.v.end());
                if (cmp(base, tmp) < 0)
                { // step back one limb
                    --shift;
                    if (shift < 0)
                        break;
                    tmp.v.clear();
                    tmp.v.assign(shift, 0);
                    tmp.v.insert(tmp.v.end(), mod.v.begin(), mod.v.end());
                }
                base = sub(base, tmp);
            }
        }
        while (!exp.isZero())
        {
            if (!exp.isEven())
                res = mulMod(res, base, mod);
            base = mulMod(base, base, mod);
            exp.shr1();
        }
        return res;
    }

    static void divmod(const Big &a, const Big &b, Big &q, Big &r)
    {
        if (b.isZero())
            throw std::runtime_error("Division by zero");

        // Nếu a < b thì q = 0, r = a
        if (cmp(a, b) < 0)
        {
            q = Big(0);
            r = a;
            return;
        }

        q.v.assign(a.v.size(), 0);
        r = Big(0);

        // Duyệt từng limb từ cao xuống thấp
        for (int i = (int)a.v.size() - 1; i >= 0; --i)
        {
            // r = (r << 64) + a.v[i]
            if (!r.v.empty())
                r.v.insert(r.v.begin(), 0);
            if (r.v.empty())
                r.v.push_back(a.v[i]);
            else
                r.v[0] = a.v[i];
            r.normalize();

            // Nếu r < b thì q limb = 0
            if (cmp(r, b) < 0)
            {
                q.v[i] = 0;
                continue;
            }

            // Binary search để tìm t sao cho b*t <= r < b*(t+1)
            uint64_t low = 0, high = 0xFFFFFFFFFFFFFFFFULL, best = 0;
            while (low <= high)
            {
                uint64_t mid = low + ((high - low) >> 1);
                Big prod = mul(b, Big(mid));
                int cmpRes = cmp(prod, r);
                if (cmpRes <= 0)
                {
                    best = mid;
                    low = mid + 1;
                }
                else
                    high = mid - 1;
            }

            q.v[i] = best;
            if (best != 0)
            {
                Big prod = mul(b, Big(best));
                r = sub(r, prod);
            }
        }

        q.normalize();
        r.normalize();
    }
};

/*** Parse HEX (uppercase) little-endian string into Big ***/
Big parse_hex_le(const string &s_raw)
{
    // 1) trim spaces/newlines
    auto trim = [](string s)
    {
        size_t a = s.find_first_not_of(" \t\r\n");
        size_t b = s.find_last_not_of(" \t\r\n");
        if (a == string::npos)
            return string();
        return s.substr(a, b - a + 1);
    };
    string s = trim(s_raw);
    // 2) reverse (little-endian hex to big-endian hex)
    reverse(s.begin(), s.end());
    // 3) parse: value = value*16 + digit
    Big val; // 0
    auto hexVal = [](char c) -> int
    {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10; // just in case
        return -1;
    };
    for (char c : s)
    {
        int d = hexVal(c);
        if (d < 0)
            continue;              // ignore non-hex (shouldn't happen in tests)
        val.mul16();               // *16
        val.addSmall((uint32_t)d); // +digit
    }
    val.normalize();
    return val;
}

/*** Primality test using Miller-Rabin ***/
bool isPrime(const Big &n)
{
    // handle small cases
    if (n.isZero())
        return false;
    if (n.cmpSmall(2) < 0)
        return false; // n < 2
    if (n.cmpSmall(2) == 0)
        return true; // n == 2
    if (n.isEven())
        return false; // even > 2
    if (n.cmpSmall(3) == 0)
        return true; // n == 3

    // write n-1 = d * 2^s with d odd
    Big n1 = Big::dec1(n);
    Big d = n1;
    int s = 0;
    while (d.isEven())
    {
        d.shr1();
        ++s;
    }

    // fixed bases (12 bases). Good probabilistic security for up to 512-bit inputs in contest settings.
    static const uint32_t bases_arr[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};
    for (uint32_t a_small : bases_arr)
    {
        // if a >= n-2, skip
        if (n.cmpSmall(a_small) <= 0)
            continue; // when n <= a
        // x = a^d mod n
        Big a(a_small);
        Big x = Big::powMod(a, d, n);
        // x == 1 or x == n-1 ?
        if (Big::cmp(x, Big(1)) == 0 || Big::cmp(x, n1) == 0)
            continue;

        bool witness = true;
        Big x2 = x;
        for (int r = 1; r < s; ++r)
        {
            x2 = Big::mulMod(x2, x2, n);
            if (Big::cmp(x2, n1) == 0)
            {
                witness = false;
                break;
            }
            if (Big::cmp(x2, Big(1)) == 0)
                return false; // non-trivial square root of 1 → composite
        }
        if (witness)
            return false; // strong witness found
    }
    return true; // probably prime
}

/*** Chia Big cho số nhỏ (1 <= m < 2^32), trả về phần dư ***/
/*** Chia Big cho số nhỏ (1 <= m < 2^32), trả về phần dư ***/
uint32_t divSmall(Big &a, uint32_t m)
{
    uint64_t rem = 0;
    const uint64_t BASE32 = (1ULL << 32);

    for (int i = (int)a.v.size() - 1; i >= 0; --i)
    {
        // chia limb 64-bit bằng 2 lần chia 32-bit để không cần 128-bit
        uint64_t hi = a.v[i] >> 32;
        uint64_t lo = a.v[i] & 0xFFFFFFFFULL;

        // kết hợp với rem để tạo số lớn nhưng vẫn nằm trong 64-bit từng bước
        uint64_t part = rem;
        uint64_t qh = part / m;
        rem = part % m;

        part = rem * BASE32 + hi;
        uint64_t q1 = part / m;
        rem = part % m;

        part = rem * BASE32 + lo;
        uint64_t q0 = part / m;
        rem = part % m;

        a.v[i] = (q1 << 32) | q0;
    }
    a.normalize();
    return (uint32_t)rem;
}

// ================== CHUYỂN BIG -> HEX ==================

// Big -> chuỗi hex big-endian (không có số 0 dư ở đầu)
string to_hex_be(const Big &x)
{
    if (x.isZero())
        return "0";

    string s;
    int i = (int)x.v.size() - 1;

    // limb cao nhất
    uint64_t limb = x.v[i];
    bool started = false;
    for (int shift = 60; shift >= 0; shift -= 4)
    {
        unsigned digit = (unsigned)((limb >> shift) & 0xFULL);
        if (!started)
        {
            if (digit == 0)
                continue; // bỏ 0 ở đầu
            started = true;
        }
        char c = (digit < 10) ? char('0' + digit) : char('A' + (digit - 10));
        s.push_back(c);
    }
    --i;

    // các limb còn lại in đủ 16 hex digit
    for (; i >= 0; --i)
    {
        limb = x.v[i];
        for (int shift = 60; shift >= 0; shift -= 4)
        {
            unsigned digit = (unsigned)((limb >> shift) & 0xFULL);
            char c = (digit < 10) ? char('0' + digit) : char('A' + (digit - 10));
            s.push_back(c);
        }
    }

    return s;
}

// Big -> chuỗi hex little-endian đúng format đề
string to_hex_le(const Big &x)
{
    string be = to_hex_be(x);
    reverse(be.begin(), be.end()); // đảo chuỗi để thành little-endian
    return be;
}

// ================== DIFFIE–HELLMAN ==================

// Hàm mô phỏng giao thức trao đổi khóa Diffie–Hellman
// Input: p, g, a, b
// Output: A = g^a mod p, B = g^b mod p, K = g^(ab) mod p
void diffieHellman(const Big &p, const Big &g,
                   const Big &a, const Big &b,
                   Big &A, Big &B, Big &K)
{
    // A = g^a mod p
    A = Big::powMod(g, a, p);

    // B = g^b mod p
    B = Big::powMod(g, b, p);

    // K = g^(ab) mod p = A^b mod p (hoặc B^a mod p cũng được)
    K = Big::powMod(A, b, p);
}

// ================== MAIN: ./main.exe test.inp ==================

int main(int argc, char *argv[])
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    // Chạy: ./main.exe test.inp test.out
    if (argc != 3)
    {
        cerr << "Usage: " << argv[0] << " input_file output_file\n";
        return 1;
    }

    ifstream fin(argv[1]);
    if (!fin)
    {
        cerr << "Cannot open input file\n";
        return 1;
    }

    ofstream fout(argv[2]);
    if (!fout)
    {
        cerr << "Cannot open output file\n";
        return 1;
    }

    // ----- Đọc p, g, a, b (hex little-endian) -----
    string sp, sg, sa, sb;
    if (!(fin >> sp >> sg >> sa >> sb))
    {
        cerr << "Invalid input format\n";
        return 1;
    }

    Big p = parse_hex_le(sp); // số nguyên tố lẻ
    Big g = parse_hex_le(sg); // căn nguyên thủy modulo p
    Big a = parse_hex_le(sa); // bí mật của Alice
    Big b = parse_hex_le(sb); // bí mật của Bob

    // ----- Gọi hàm Diffie–Hellman -----
    Big A, B, K;
    diffieHellman(p, g, a, b, A, B, K);

    // ----- Ghi ra file output dạng HEX little-endian -----
    fout << to_hex_le(A) << '\n';
    fout << to_hex_le(B) << '\n';
    fout << to_hex_le(K) << '\n';

    return 0;
}
