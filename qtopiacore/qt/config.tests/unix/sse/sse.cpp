#include <xmmintrin.h>

int main(int, char**)
{
    __m64 a = _mm_setzero_si64();
    a = _mm_shuffle_pi16(a, 0);
    return _m_to_int(a);
}
