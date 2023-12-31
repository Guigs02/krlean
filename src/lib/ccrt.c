//
// CC runtime library
//
#if defined(__GNUC__) || defined(__TINYC__)

#define W_TYPE_SIZE 32
#define BITS_PER_UNIT 8

typedef int Wtype;
typedef unsigned int UWtype;
typedef unsigned int USItype;
typedef long long DWtype;
typedef unsigned long long UDWtype;

struct DWstruct
{
  Wtype low, high;
};

typedef union
{
  struct DWstruct s;
  DWtype ll;
} DWunion;

typedef long double XFtype;

#define WORD_SIZE (sizeof(Wtype) * BITS_PER_UNIT)
#define HIGH_WORD_COEFF (((UDWtype)1) << WORD_SIZE)

// The following deals with IEEE single-precision numbers

#define EXCESS 126
#define SIGNBIT 0x80000000
#define HIDDEN (1 << 23)
#define SIGN(fp) ((fp) & SIGNBIT)
#define EXP(fp) (((fp) >> 23) & 0xFF)
#define MANT(fp) (((fp) & 0x7FFFFF) | HIDDEN)
#define PACK(s, e, m) ((s) | ((e) << 23) | (m))

// The following deal with IEEE double-precision numbers

#define EXCESSD 1022
#define HIDDEND (1 << 20)
#define EXPD(fp) (((fp.l.upper) >> 20) & 0x7FF)
#define SIGND(fp) ((fp.l.upper) & SIGNBIT)
#define MANTD(fp) (((((fp.l.upper) & 0xFFFFF) | HIDDEND) << 10) | (fp.l.lower >> 22))
#define HIDDEND_LL ((long long)1 << 52)
#define MANTD_LL(fp) ((fp.ll & (HIDDEND_LL - 1)) | HIDDEND_LL)
#define PACKD_LL(s, e, m) (((long long)((s) + ((e) << 20)) << 32) | (m))

// The following deal with x86 long double-precision numbers

#define EXCESSLD 16382
#define EXPLD(fp) (fp.l.upper & 0x7fff)
#define SIGNLD(fp) ((fp.l.upper) & 0x8000)

// Only for x86

union ldouble_long
{
  long double ld;
  struct
  {
    unsigned long long lower;
    unsigned short upper;
  } l;
};

union double_long
{
  double d;
  struct
  {
    unsigned long lower;
    long upper;
  } l;
  long long ll;
};

union float_long
{
  float f;
  long l;
};

#if defined(__i386__)

#define sub_ddmmss(sh, sl, ah, al, bh, bl) \
  __asm__("subl %5,%1\n\tsbbl %3,%0"       \
          : "=r"((USItype)(sh)),           \
            "=&r"((USItype)(sl))           \
          : "0"((USItype)(ah)),            \
            "g"((USItype)(bh)),            \
            "1"((USItype)(al)),            \
            "g"((USItype)(bl)))

#define umul_ppmm(w1, w0, u, v)  \
  __asm__("mull %3"              \
          : "=a"((USItype)(w0)), \
            "=d"((USItype)(w1))  \
          : "%0"((USItype)(u)),  \
            "rm"((USItype)(v)))

#define udiv_qrnnd(q, r, n1, n0, dv) \
  __asm__("divl %4"                  \
          : "=a"((USItype)(q)),      \
            "=d"((USItype)(r))       \
          : "0"((USItype)(n0)),      \
            "1"((USItype)(n1)),      \
            "rm"((USItype)(dv)))

#define count_leading_zeros(count, x)              \
  do                                               \
  {                                                \
    USItype __cbtmp;                               \
    __asm__("bsrl %1,%0"                           \
            : "=r"(__cbtmp) : "rm"((USItype)(x))); \
    (count) = __cbtmp ^ 31;                        \
  } while (0)

#else

#error unsupported CPU type

#endif

// Most of this code is taken from libgcc2.c from gcc

static UDWtype __udivmoddi4(UDWtype n, UDWtype d, UDWtype *rp)
{
  DWunion ww;
  DWunion nn, dd;
  DWunion rr;
  UWtype d0, d1, n0, n1, n2;
  UWtype q0, q1;
  UWtype b, bm;

  nn.ll = n;
  dd.ll = d;

  d0 = dd.s.low;
  d1 = dd.s.high;
  n0 = nn.s.low;
  n1 = nn.s.high;

#if !UDIV_NEEDS_NORMALIZATION
  if (d1 == 0)
  {
    if (d0 > n1)
    {
      // 0q = nn / 0D
      udiv_qrnnd(q0, n0, n1, n0, d0);
      q1 = 0;
      // Remainder in n0.
    }
    else
    {
      // qq = NN / 0d
      if (d0 == 0)
        d0 = 1 / d0; // Divide intentionally by zero
      udiv_qrnnd(q1, n1, 0, n1, d0);
      udiv_qrnnd(q0, n0, n1, n0, d0);
      // Remainder in n0.
    }

    if (rp != 0)
    {
      rr.s.low = n0;
      rr.s.high = 0;
      *rp = rr.ll;
    }
  }

#else  // UDIV_NEEDS_NORMALIZATION

  if (d1 == 0)
  {
    if (d0 > n1)
    {
      // 0q = nn / 0D
      count_leading_zeros(bm, d0);
      if (bm != 0)
      {
        // Normalize, i.e. make the most significant bit of the denominator set.
        d0 = d0 << bm;
        n1 = (n1 << bm) | (n0 >> (W_TYPE_SIZE - bm));
        n0 = n0 << bm;
      }

      udiv_qrnnd(q0, n0, n1, n0, d0);
      q1 = 0;
      // Remainder in n0 >> bm.
    }
    else
    {
      // qq = NN / 0d
      if (d0 == 0)
        d0 = 1 / d0; // Divide intentionally by zero.
      count_leading_zeros(bm, d0);
      if (bm == 0)
      {
        // From (n1 >= d0) /\ (the most significant bit of d0 is set),
        // conclude (the most significant bit of n1 is set) /\ (the
        // leading quotient digit q1 = 1).
        //
        // This special case is necessary, not an optimization.
        // (Shifts counts of W_TYPE_SIZE are undefined.)  */

        n1 -= d0;
        q1 = 1;
      }
      else
      {
        // Normalize
        b = W_TYPE_SIZE - bm;

        d0 = d0 << bm;
        n2 = n1 >> b;
        n1 = (n1 << bm) | (n0 >> b);
        n0 = n0 << bm;

        udiv_qrnnd(q1, n1, n2, n1, d0);
      }

      // n1 != d0...
      udiv_qrnnd(q0, n0, n1, n0, d0);

      // Remainder in n0 >> bm.
    }

    if (rp != 0)
    {
      rr.s.low = n0 >> bm;
      rr.s.high = 0;
      *rp = rr.ll;
    }
  }
#endif // UDIV_NEEDS_NORMALIZATION

  else
  {
    if (d1 > n1)
    {
      // 00 = nn / DD
      q0 = 0;
      q1 = 0;

      // Remainder in n1n0.
      if (rp != 0)
      {
        rr.s.low = n0;
        rr.s.high = n1;
        *rp = rr.ll;
      }
    }
    else
    {
      // 0q = NN / dd
      count_leading_zeros(bm, d1);
      if (bm == 0)
      {
        // From (n1 >= d1) /\ (the most significant bit of d1 is set),
        // conclude (the most significant bit of n1 is set) /\ (the
        // quotient digit q0 = 0 or 1).
        //
        // This special case is necessary, not an optimization.
        //
        // The condition on the next line takes advantage of that
        // n1 >= d1 (true due to program flow).

        if (n1 > d1 || n0 >= d0)
        {
          q0 = 1;
          sub_ddmmss(n1, n0, n1, n0, d1, d0);
        }
        else
        {
          q0 = 0;
        }

        q1 = 0;

        if (rp != 0)
        {
          rr.s.low = n0;
          rr.s.high = n1;
          *rp = rr.ll;
        }
      }
      else
      {
        UWtype m1, m0;
        // Normalize.
        b = W_TYPE_SIZE - bm;

        d1 = (d1 << bm) | (d0 >> b);
        d0 = d0 << bm;
        n2 = n1 >> b;
        n1 = (n1 << bm) | (n0 >> b);
        n0 = n0 << bm;

        udiv_qrnnd(q0, n1, n2, n1, d1);
        umul_ppmm(m1, m0, q0, d0);

        if (m1 > n1 || (m1 == n1 && m0 > n0))
        {
          q0--;
          sub_ddmmss(m1, m0, m1, m0, d1, d0);
        }

        q1 = 0;

        // Remainder in (n1n0 - m1m0) >> bm.
        if (rp != 0)
        {
          sub_ddmmss(n1, n0, n1, n0, m1, m0);
          rr.s.low = (n1 << b) | (n0 >> bm);
          rr.s.high = n1 >> bm;
          *rp = rr.ll;
        }
      }
    }
  }

  ww.s.low = q0;
  ww.s.high = q1;
  return ww.ll;
}

#define __negdi2(a) (-(a))

long long __divdi3(long long u, long long v)
{
  int c = 0;
  DWunion uu, vv;
  DWtype w;

  uu.ll = u;
  vv.ll = v;

  if (uu.s.high < 0)
  {
    c = ~c;
    uu.ll = __negdi2(uu.ll);
  }

  if (vv.s.high < 0)
  {
    c = ~c;
    vv.ll = __negdi2(vv.ll);
  }

  w = __udivmoddi4(uu.ll, vv.ll, (UDWtype *)0);
  if (c)
    w = __negdi2(w);
  return w;
}

long long __moddi3(long long u, long long v)
{
  int c = 0;
  DWunion uu, vv;
  DWtype w;

  uu.ll = u;
  vv.ll = v;

  if (uu.s.high < 0)
  {
    c = ~c;
    uu.ll = __negdi2(uu.ll);
  }

  if (vv.s.high < 0)
    vv.ll = __negdi2(vv.ll);
  __udivmoddi4(uu.ll, vv.ll, (UDWtype *)&w);
  if (c)
    w = __negdi2(w);
  return w;
}

unsigned long long __udivdi3(unsigned long long u, unsigned long long v)
{
  return __udivmoddi4(u, v, (UDWtype *)0);
}

unsigned long long __umoddi3(unsigned long long u, unsigned long long v)
{
  UDWtype w;

  __udivmoddi4(u, v, &w);
  return w;
}

long long __sardi3(long long a, int b)
{
  DWunion u;

  u.ll = a;
  if (b >= 32)
  {
    u.s.low = u.s.high >> (b - 32);
    u.s.high = u.s.high >> 31;
  }
  else if (b != 0)
  {
    u.s.low = ((unsigned)u.s.low >> b) | (u.s.high << (32 - b));
    u.s.high = u.s.high >> b;
  }

  return u.ll;
}

unsigned long long __shrdi3(unsigned long long a, int b)
{
  DWunion u;

  u.ll = a;
  if (b >= 32)
  {
    u.s.low = (unsigned)u.s.high >> (b - 32);
    u.s.high = 0;
  }
  else if (b != 0)
  {
    u.s.low = ((unsigned)u.s.low >> b) | (u.s.high << (32 - b));
    u.s.high = (unsigned)u.s.high >> b;
  }

  return u.ll;
}

long long __shldi3(long long a, int b)
{
  DWunion u;

  u.ll = a;

  if (b >= 32)
  {
    u.s.high = (unsigned)u.s.low << (b - 32);
    u.s.low = 0;
  }
  else if (b != 0)
  {
    u.s.high = ((unsigned)u.s.high << b) | ((unsigned)u.s.low >> (32 - b));
    u.s.low = (unsigned)u.s.low << b;
  }

  return u.ll;
}

#if defined(__i386__)
// FPU control word for rounding to nearest mode
unsigned short __cc_fpu_control = 0x137f;
// FPU control word for round to zero mode for int conversion
unsigned short __cc_int_fpu_control = 0x137f | 0x0c00;
#endif

float __ulltof(unsigned long long a)
{
  DWunion uu;
  XFtype r;

  uu.ll = a;
  if (uu.s.high >= 0)
  {
    return (float)uu.ll;
  }
  else
  {
    r = (XFtype)uu.ll;
    r += 18446744073709551616.0;
    return (float)r;
  }
}

double __ulltod(unsigned long long a)
{
  DWunion uu;
  XFtype r;

  uu.ll = a;
  if (uu.s.high >= 0)
  {
    return (double)uu.ll;
  }
  else
  {
    r = (XFtype)uu.ll;
    r += 18446744073709551616.0;
    return (double)r;
  }
}

long double __ulltold(unsigned long long a)
{
  DWunion uu;
  XFtype r;

  uu.ll = a;
  if (uu.s.high >= 0)
  {
    return (long double)uu.ll;
  }
  else
  {
    r = (XFtype)uu.ll;
    r += 18446744073709551616.0;
    return (long double)r;
  }
}

unsigned long long __fixunssfdi(float a1)
{
  register union float_long fl1;
  register int exp;
  register unsigned long l;

  fl1.f = a1;

  if (fl1.l == 0)
    return 0;

  exp = EXP(fl1.l) - EXCESS - 24;

  l = MANT(fl1.l);
  if (exp >= 41)
  {
    return (unsigned long long)-1;
  }
  else if (exp >= 0)
  {
    return (unsigned long long)l << exp;
  }
  else if (exp >= -23)
  {
    return l >> -exp;
  }
  else
  {
    return 0;
  }
}

unsigned long long __fixunsdfdi(double a1)
{
  register union double_long dl1;
  register int exp;
  register unsigned long long l;

  dl1.d = a1;

  if (dl1.ll == 0)
    return 0;

  exp = EXPD(dl1) - EXCESSD - 53;

  l = MANTD_LL(dl1);

  if (exp >= 12)
  {
    return (unsigned long long)-1;
  }
  else if (exp >= 0)
  {
    return l << exp;
  }
  else if (exp >= -52)
  {
    return l >> -exp;
  }
  else
  {
    return 0;
  }
}

unsigned long long __fixunsxfdi(long double a1)
{
  register union ldouble_long dl1;
  register int exp;
  register unsigned long long l;

  dl1.ld = a1;

  if (dl1.l.lower == 0 && dl1.l.upper == 0)
    return 0;

  exp = EXPLD(dl1) - EXCESSLD - 64;

  l = dl1.l.lower;

  if (exp > 0)
  {
    return (unsigned long long)-1;
  }
  else if (exp >= -63)
  {
    return l >> -exp;
  }
  else
  {
    return 0;
  }
}

#endif
