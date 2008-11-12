#include <emmintrin.h>

int main(int, char**)
{
    __m128i a = _mm_setzero_si128();
    _mm_maskmoveu_si128(a, _mm_setzero_si128(), 0);
    return 0;
}
