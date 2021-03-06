#ifndef IMAGE_TYPE_HPP_
#define IMAGE_TYPE_HPP_


#include <algorithm>
#include "Type.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// Enable C++ PPL Support
// Define it in the source file before #include "Image_Type.h" for fast compiling
//#define ENABLE_PPL

// Enable C++ AMP Support
// Define it in the source file before #include "Image_Type.h" for fast compiling
//#define ENABLE_AMP


#define LOOP_V _Loop_V
#define LOOP_H _Loop_H
#define LOOP_Hinv _Loop_Hinv
#define LOOP_VH _Loop_VH
#define FOR_EACH _For_each
#define TRANSFORM _Transform
#define CONVOLUTE _Convolute

#ifdef ENABLE_PPL
#include <ppl.h>
#include <ppltasks.h>
#define LOOP_V_PPL _Loop_V_PPL
#define LOOP_H_PPL _Loop_H_PPL
#define LOOP_Hinv_PPL _Loop_Hinv_PPL
#define LOOP_VH_PPL _Loop_VH_PPL
#define FOR_EACH_PPL _For_each_PPL
#define TRANSFORM_PPL _Transform_PPL
#define CONVOLUTE_PPL _Convolute_PPL
const PCType PPL_HP = 256; // Height piece size for PPL
const PCType PPL_WP = 256; // Width piece size for PPL
#else
#define LOOP_V_PPL LOOP_V
#define LOOP_H_PPL LOOP_H
#define LOOP_Hinv_PPL LOOP_Hinv
#define LOOP_VH_PPL LOOP_VH
#define FOR_EACH_PPL FOR_EACH
#define TRANSFORM_PPL TRANSFORM
#define CONVOLUTE_PPL CONVOLUTE
#endif

#ifdef ENABLE_AMP
#include <amp.h>
#include <amp_math.h>
#define FOR_EACH_AMP _For_each_AMP
#define TRANSFORM_AMP _Transform_AMP
#else
#define FOR_EACH_AMP FOR_EACH
#define TRANSFORM_AMP TRANSFORM
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


template < typename _Ty >
void _GetQuanPara(_Ty &Floor, _Ty &Ceil, int bps, bool full, const std::false_type &)
{
    if (full)
    {
        Floor = static_cast<_Ty>(0);
        Ceil = static_cast<_Ty>((1 << bps) - 1);
    }
    else
    {
        Floor = static_cast<_Ty>(16 << (bps - 8));
        Ceil = static_cast<_Ty>(235 << (bps - 8));
    }
}

template < typename _Ty >
void _GetQuanPara(_Ty &Floor, _Ty &Ceil, int bps, bool full, const std::true_type &)
{
    Floor = static_cast<_Ty>(0);
    Ceil = static_cast<_Ty>(1);
}

template < typename _Ty >
void GetQuanPara(_Ty &Floor, _Ty &Ceil, int bps, bool full)
{
    _GetQuanPara(Floor, Ceil, bps, full, _IsFloat<_Ty>());
}


template < typename _Ty >
void _GetQuanPara(_Ty &Floor, _Ty &Neutral, _Ty &Ceil, int bps, bool full, bool chroma, const std::false_type &)
{
    if (full)
    {
        Floor = static_cast<_Ty>(0);
        Neutral = chroma ? static_cast<_Ty>(1 << (bps - 1)) : Floor;
        Ceil = static_cast<_Ty>((1 << bps) - 1);
    }
    else
    {
        Floor = static_cast<_Ty>(16 << (bps - 8));
        Neutral = chroma ? static_cast<_Ty>(1 << (bps - 1)) : Floor;
        Ceil = static_cast<_Ty>(chroma ? 240 << (bps - 8) : 235 << (bps - 8));
    }
}

template < typename _Ty >
void _GetQuanPara(_Ty &Floor, _Ty &Neutral, _Ty &Ceil, int bps, bool full, bool chroma, const std::true_type &)
{
    Floor = static_cast<_Ty>(chroma ? -0.5 : 0);
    Neutral = chroma ? static_cast<_Ty>(0) : Floor;
    Ceil = static_cast<_Ty>(chroma ? 0.5 : 1);
}

template < typename _Ty >
void GetQuanPara(_Ty &Floor, _Ty &Neutral, _Ty &Ceil, int bps, bool full, bool chroma)
{
    _GetQuanPara(Floor, Neutral, Ceil, bps, full, chroma, _IsFloat<_Ty>());
}


template < typename _Ty >
void GetQuanPara(_Ty &FloorY, _Ty &CeilY, _Ty &FloorC, _Ty &NeutralC, _Ty &CeilC, int bps, bool full)
{
    GetQuanPara(FloorY, CeilY, bps, full);
    GetQuanPara(FloorC, NeutralC, CeilC, bps, full, true);
}


template < typename _Ty >
void Quantize_Value(_Ty &Floor, _Ty &Neutral, _Ty &Ceil, _Ty BitDepth, QuantRange _QuantRange, bool Chroma)
{
    GetQuanPara(Floor, Neutral, Ceil, BitDepth, _QuantRange != QuantRange::TV, Chroma);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


template < typename _Ty >
bool isChroma(_Ty Floor, _Ty Neutral)
{
    return Floor < Neutral;
}


template < typename _Ty >
bool _IsPCChroma(_Ty Floor, _Ty Neutral, _Ty Ceil, const std::false_type &)
{
    return Floor < Neutral && (Floor + Ceil) % 2 == 1;
}

template < typename _Ty >
bool _IsPCChroma(_Ty Floor, _Ty Neutral, _Ty Ceil, const std::true_type &)
{
    return false;
}

template < typename _Ty >
bool isPCChroma(_Ty Floor, _Ty Neutral, _Ty Ceil)
{
    return _IsPCChroma(Floor, Neutral, Ceil, _IsFloat<_Ty>());
}


template < typename _Ty >
void _ReSetChroma(_Ty &Floor, _Ty &Neutral, _Ty &Ceil, bool Chroma, const std::false_type &)
{
    const bool srcChroma = isChroma(Floor, Neutral);

    if (Chroma && !srcChroma)
    {
        Neutral = (Floor + Ceil + 1) / 2;
    }
    else if (!Chroma && srcChroma)
    {
        Neutral = Floor;
    }
}

template < typename _Ty >
void _ReSetChroma(_Ty &Floor, _Ty &Neutral, _Ty &Ceil, bool Chroma, const std::true_type &)
{
    const bool srcChroma = isChroma(Floor, Neutral);

    const _Ty Range = Ceil - Floor;

    if (Chroma && !srcChroma)
    {
        Floor = -Range / 2;
        Neutral = 0;
        Ceil = Range / 2;
    }
    else if (!Chroma && srcChroma)
    {
        Floor = 0;
        Neutral = 0;
        Ceil = Range;
    }
}

template < typename _Ty >
void ReSetChroma(_Ty &Floor, _Ty &Neutral, _Ty &Ceil, bool Chroma = false)
{
    _ReSetChroma(Floor, Neutral, Ceil, Chroma, _IsFloat<_Ty>());
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// Template functions of algorithm
template < typename _Fn1 >
void _Loop_V(const PCType height, _Fn1 &&_Func)
{
    for (PCType j = 0; j < height; ++j)
    {
        _Func(j);
    }
}

template < typename _Fn1 >
void _Loop_H(const PCType height, const PCType width, const PCType stride, _Fn1 &&_Func)
{
    const PCType offset = 0;
    const PCType range = width;

    for (PCType j = 0; j < height; ++j)
    {
        const PCType lower = j * stride + offset;
        const PCType upper = lower + range;

        _Func(j, lower, upper);
    }
}

template < typename _Fn1 >
void _Loop_Hinv(const PCType height, const PCType width, const PCType stride, _Fn1 &&_Func)
{
    const PCType offset = 0;
    const PCType range = width;

    for (PCType j = height - 1; j >= 0; --j)
    {
        const PCType lower = j * stride + offset;
        const PCType upper = lower + range;

        _Func(j, lower, upper);
    }
}

template < typename _Fn1 >
void _Loop_VH(const PCType height, const PCType width, const PCType stride, _Fn1 &&_Func)
{
    for (PCType j = 0; j < height; ++j)
    {
        PCType i = j * stride;

        for (const PCType upper = i + width; i < upper; ++i)
        {
            _Func(i);
        }
    }
}

template < typename _Fn1 >
void _Loop_VH(const PCType height, const PCType width, const PCType stride0, const PCType stride1, _Fn1 &&_Func)
{
    for (PCType j = 0; j < height; ++j)
    {
        PCType i0 = j * stride0;
        PCType i1 = j * stride1;

        for (const PCType upper = i0 + width; i0 < upper; ++i0, ++i1)
        {
            _Func(i0, i1);
        }
    }
}


template < typename _St1, typename _Fn1 >
void _For_each(_St1 &data, _Fn1 &&_Func)
{
    LOOP_VH(data.Height(), data.Width(), data.Stride(), [&](PCType i)
    {
        _Func(data[i]);
    });
}

template < typename _St1, typename _Fn1 >
void _Transform(_St1 &data, _Fn1 &&_Func)
{
    LOOP_VH(data.Height(), data.Width(), data.Stride(), [&](PCType i)
    {
        data[i] = _Func(data[i]);
    });
}

template < typename _Dt1, typename _St1, typename _Fn1 >
void _Transform(_Dt1 &dst, const _St1 &src, _Fn1 &&_Func)
{
    if (dst.Width() != src.Width() || dst.Height() != src.Height())
    {
        DEBUG_FAIL("_Transform: Width() and Height() of dst and src must be the same.");
    }

    LOOP_VH(dst.Height(), dst.Width(), dst.Stride(), [&](PCType i)
    {
        dst[i] = _Func(src[i]);
    });
}

template < typename _Dt1, typename _St1, typename _St2, typename _Fn1 >
void _Transform(_Dt1 &dst, const _St1 &src1, const _St2 &src2, _Fn1 &&_Func)
{
    if (dst.Width() != src1.Width() || dst.Height() != src1.Height()
        || dst.Width() != src2.Width() || dst.Height() != src2.Height())
    {
        DEBUG_FAIL("_Transform: Width() and Height() of dst, src1 and src2 must be the same.");
    }

    LOOP_VH(dst.Height(), dst.Width(), dst.Stride(), [&](PCType i)
    {
        dst[i] = _Func(src1[i], src2[i]);
    });
}

template < typename _Dt1, typename _St1, typename _St2, typename _St3, typename _Fn1 >
void _Transform(_Dt1 &dst, const _St1 &src1, const _St2 &src2, const _St3 &src3, _Fn1 &&_Func)
{
    if (dst.Width() != src1.Width() || dst.Height() != src1.Height()
        || dst.Width() != src2.Width() || dst.Height() != src2.Height()
        || dst.Width() != src3.Width() || dst.Height() != src3.Height())
    {
        DEBUG_FAIL("_Transform: Width() and Height() of dst, src1, src2 and src3 must be the same.");
    }

    LOOP_VH(dst.Height(), dst.Width(), dst.Stride(), [&](PCType i)
    {
        dst[i] = _Func(src1[i], src2[i], src3[i]);
    });
}

template < typename _Dt1, typename _St1, typename _St2, typename _St3, typename _St4, typename _Fn1 >
void _Transform(_Dt1 &dst, const _St1 &src1, const _St2 &src2, const _St3 &src3, const _St4 &src4, _Fn1 &&_Func)
{
    if (dst.Width() != src1.Width() || dst.Height() != src1.Height()
        || dst.Width() != src2.Width() || dst.Height() != src2.Height()
        || dst.Width() != src3.Width() || dst.Height() != src3.Height()
        || dst.Width() != src4.Width() || dst.Height() != src4.Height())
    {
        DEBUG_FAIL("_Transform: Width() and Height() of dst, src1, src2, src3 and src4 must be the same.");
    }

    LOOP_VH(dst.Height(), dst.Width(), dst.Stride(), [&](PCType i)
    {
        dst[i] = _Func(src1[i], src2[i], src3[i], src4[i]);
    });
}

template < PCType VRad, PCType HRad, typename _Dt1, typename _St1, typename _Fn1 >
void _Convolute(_Dt1 &dst, const _St1 &src, _Fn1 &&_Func)
{
    if (dst.Width() != src.Width() || dst.Height() != src.Height())
    {
        DEBUG_FAIL("_Convolute: Width() and Height() of dst and src must be the same.");
    }

    LOOP_V(dst.Height(), [&](PCType j)
    {
        auto dstp = dst.data() + j * dst.Stride();

        typename _St1::const_pointer srcpV[VRad * 2 + 1];
        typename _St1::value_type srcb2D[VRad * 2 + 1][HRad * 2 + 1];

        srcpV[VRad] = src.data() + j * src.Stride();
        for (PCType y = 1; y <= VRad; ++y)
        {
            srcpV[VRad - y] = j < y ? srcpV[VRad - y + 1] : srcpV[VRad - y + 1] - src.Stride();
            srcpV[VRad + y] = j >= src.Height() - y ? srcpV[VRad + y - 1] : srcpV[VRad + y - 1] + src.Stride();
        }

        for (PCType y = 0; y < VRad * 2 + 1; ++y)
        {
            PCType x = 0;
            for (; x < HRad + 2 && x < HRad * 2 + 1; ++x)
            {
                srcb2D[y][x] = srcpV[y][0];
            }
            for (; x < HRad * 2 + 1; ++x)
            {
                srcb2D[y][x] = srcpV[y][x - HRad - 1];
            }
        }

        PCType i = HRad;
        for (; i < dst.Width(); ++i)
        {
            for (PCType y = 0; y < VRad * 2 + 1; ++y)
            {
                PCType x = 0;
                for (; x < HRad * 2; ++x)
                {
                    srcb2D[y][x] = srcb2D[y][x + 1];
                }
                srcb2D[y][x] = srcpV[y][i];
            }

            dstp[i - HRad] = _Func(srcb2D);
        }
        for (; i < dst.Width() + HRad; ++i)
        {
            for (PCType y = 0; y < VRad * 2 + 1; ++y)
            {
                PCType x = 0;
                for (; x < HRad * 2; ++x)
                {
                    srcb2D[y][x] = srcb2D[y][x + 1];
                }
            }

            dstp[i - HRad] = _Func(srcb2D);
        }
    });
}


// Template functions of algorithm with PPL
#ifdef ENABLE_PPL
template < typename _Fn1 >
void _Loop_V_PPL(const PCType height, _Fn1 &&_Func)
{
    const PCType pNum = (height + PPL_HP - 1) / PPL_HP;

    concurrency::parallel_for(PCType(0), pNum, [&](PCType p)
    {
        const PCType lower = p * PPL_HP;
        const PCType upper = Min(height, lower + PPL_HP);

        for (PCType j = lower; j < upper; ++j)
        {
            _Func(j);
        }
    });
}

template < typename _Fn1 >
void _Loop_H_PPL(const PCType height, const PCType width, const PCType stride, _Fn1 &&_Func)
{
    const PCType pNum = (width + PPL_WP - 1) / PPL_WP;

    concurrency::parallel_for(PCType(0), pNum, [&](PCType p)
    {
        const PCType offset = p * PPL_WP;
        const PCType range = Min(width - offset, PPL_WP);

        for (PCType j = 0; j < height; ++j)
        {
            const PCType lower = j * stride + offset;
            const PCType upper = lower + range;

            _Func(j, lower, upper);
        }
    });
}

template < typename _Fn1 >
void _Loop_Hinv_PPL(const PCType height, const PCType width, const PCType stride, _Fn1 &&_Func)
{
    const PCType pNum = (width + PPL_WP - 1) / PPL_WP;

    concurrency::parallel_for(PCType(0), pNum, [&](PCType p)
    {
        const PCType offset = p * PPL_WP;
        const PCType range = Min(width - offset, PPL_WP);

        for (PCType j = height - 1; j >= 0; --j)
        {
            const PCType lower = j * stride + offset;
            const PCType upper = lower + range;

            _Func(j, lower, upper);
        }
    });
}

template < typename _Fn1 >
void _Loop_VH_PPL(const PCType height, const PCType width, const PCType stride, _Fn1 &&_Func)
{
    const PCType pNum = (height + PPL_HP - 1) / PPL_HP;

    concurrency::parallel_for(PCType(0), pNum, [&](PCType p)
    {
        const PCType lower = p * PPL_HP;
        const PCType upper = Min(height, lower + PPL_HP);

        for (PCType j = lower; j < upper; ++j)
        {
            PCType i = j * stride;

            for (const PCType upper = i + width; i < upper; ++i)
            {
                _Func(i);
            }
        }
    });
}

template < typename _Fn1 >
void _Loop_VH_PPL(const PCType height, const PCType width, const PCType dst_stride, const PCType src_stride, _Fn1 &&_Func)
{
    const PCType pNum = (height + PPL_HP - 1) / PPL_HP;

    concurrency::parallel_for(PCType(0), pNum, [&](PCType p)
    {
        const PCType lower = p * PPL_HP;
        const PCType upper = Min(height, lower + PPL_HP);

        for (PCType j = lower; j < upper; ++j)
        {
            PCType i0 = j * dst_stride;
            PCType i1 = j * src_stride;

            for (const PCType upper = i0 + width; i0 < upper; ++i0, ++i1)
            {
                _Func(i0, i1);
            }
        }
    });
}


template < typename _St1, typename _Fn1 >
void _For_each_PPL(_St1 &data, _Fn1 &&_Func)
{
    LOOP_VH_PPL(data.Height(), data.Width(), data.Stride(), [&](PCType i)
    {
        _Func(data[i]);
    });
}

template < typename _St1, typename _Fn1 >
void _Transform_PPL(_St1 &data, _Fn1 &&_Func)
{
    LOOP_VH_PPL(data.Height(), data.Width(), data.Stride(), [&](PCType i)
    {
        data[i] = _Func(data[i]);
    });
}

template < typename _Dt1, typename _St1, typename _Fn1 >
void _Transform_PPL(_Dt1 &dst, const _St1 &src, _Fn1 &&_Func)
{
    if (dst.Width() != src.Width() || dst.Height() != src.Height())
    {
        DEBUG_FAIL("_Transform_PPL: Width() and Height() of dst and src must be the same.");
    }

    LOOP_VH_PPL(dst.Height(), dst.Width(), dst.Stride(), [&](PCType i)
    {
        dst[i] = _Func(src[i]);
    });
}

template < typename _Dt1, typename _St1, typename _St2, typename _Fn1 >
void _Transform_PPL(_Dt1 &dst, const _St1 &src1, const _St2 &src2, _Fn1 &&_Func)
{
    if (dst.Width() != src1.Width() || dst.Height() != src1.Height() || dst.Width() != src2.Width() || dst.Height() != src2.Height())
    {
        DEBUG_FAIL("_Transform_PPL: Width() and Height() of dst, src1 and src2 must be the same.");
    }

    LOOP_VH_PPL(dst.Height(), dst.Width(), dst.Stride(), [&](PCType i)
    {
        dst[i] = _Func(src1[i], src2[i]);
    });
}

template < typename _Dt1, typename _St1, typename _St2, typename _St3, typename _Fn1 >
void _Transform_PPL(_Dt1 &dst, const _St1 &src1, const _St2 &src2, const _St3 &src3, _Fn1 &&_Func)
{
    if (dst.Width() != src1.Width() || dst.Height() != src1.Height()
        || dst.Width() != src2.Width() || dst.Height() != src2.Height()
        || dst.Width() != src3.Width() || dst.Height() != src3.Height())
    {
        DEBUG_FAIL("_Transform_PPL: Width() and Height() of dst, src1, src2 and src3 must be the same.");
    }

    LOOP_VH_PPL(dst.Height(), dst.Width(), dst.Stride(), [&](PCType i)
    {
        dst[i] = _Func(src1[i], src2[i], src3[i]);
    });
}

template < typename _Dt1, typename _St1, typename _St2, typename _St3, typename _St4, typename _Fn1 >
void _Transform_PPL(_Dt1 &dst, const _St1 &src1, const _St2 &src2, const _St3 &src3, const _St4 &src4, _Fn1 &&_Func)
{
    if (dst.Width() != src1.Width() || dst.Height() != src1.Height()
        || dst.Width() != src2.Width() || dst.Height() != src2.Height()
        || dst.Width() != src3.Width() || dst.Height() != src3.Height()
        || dst.Width() != src4.Width() || dst.Height() != src4.Height())
    {
        DEBUG_FAIL("_Transform_PPL: Width() and Height() of dst, src1, src2, src3 and src4 must be the same.");
    }

    LOOP_VH_PPL(dst.Height(), dst.Width(), dst.Stride(), [&](PCType i)
    {
        dst[i] = _Func(src1[i], src2[i], src3[i], src4[i]);
    });
}

template < PCType VRad, PCType HRad, typename _Dt1, typename _St1, typename _Fn1 >
void _Convolute_PPL(_Dt1 &dst, const _St1 &src, _Fn1 &&_Func)
{
    if (dst.Width() != src.Width() || dst.Height() != src.Height())
    {
        DEBUG_FAIL("_Convolute_PPL: Width() and Height() of dst and src must be the same.");
    }

    LOOP_V_PPL(dst.Height(), [&](PCType j)
    {
        auto dstp = dst.data() + j * dst.Stride();

        typename _St1::const_pointer srcpV[VRad * 2 + 1];
        typename _St1::value_type srcb2D[VRad * 2 + 1][HRad * 2 + 1];

        srcpV[VRad] = src.data() + j * src.Stride();
        for (PCType y = 1; y <= VRad; ++y)
        {
            srcpV[VRad - y] = j < y ? srcpV[VRad - y + 1] : srcpV[VRad - y + 1] - src.Stride();
            srcpV[VRad + y] = j >= src.Height() - y ? srcpV[VRad + y - 1] : srcpV[VRad + y - 1] + src.Stride();
        }

        for (PCType y = 0; y < VRad * 2 + 1; ++y)
        {
            PCType x = 0;
            for (; x < HRad + 2 && x < HRad * 2 + 1; ++x)
            {
                srcb2D[y][x] = srcpV[y][0];
            }
            for (; x < HRad * 2 + 1; ++x)
            {
                srcb2D[y][x] = srcpV[y][x - HRad - 1];
            }
        }

        PCType i = HRad;
        for (; i < dst.Width(); ++i)
        {
            for (PCType y = 0; y < VRad * 2 + 1; ++y)
            {
                PCType x = 0;
                for (; x < HRad * 2; ++x)
                {
                    srcb2D[y][x] = srcb2D[y][x + 1];
                }
                srcb2D[y][x] = srcpV[y][i];
            }

            dstp[i - HRad] = _Func(srcb2D);
        }
        for (; i < dst.Width() + HRad; ++i)
        {
            for (PCType y = 0; y < VRad * 2 + 1; ++y)
            {
                PCType x = 0;
                for (; x < HRad * 2; ++x)
                {
                    srcb2D[y][x] = srcb2D[y][x + 1];
                }
            }

            dstp[i - HRad] = _Func(srcb2D);
        }
    });
}
#endif


// Template functions of algorithm with AMP
#ifdef ENABLE_AMP
template < typename _St1, typename _Fn1 >
void _For_each_AMP(_St1 &data, _Fn1 &&_Func)
{
    concurrency::array_view<decltype(data.value(0)), 1> datav(data.PixelCount(), datap);

    concurrency::parallel_for_each(datav.extent, [=](concurrency::index<1> idx) restrict(amp)
    {
        _Func(datav[idx]);
    });
}

template < typename _St1, typename _Fn1 >
void _Transform_AMP(_St1 &data, _Fn1 &&_Func)
{
    concurrency::array_view<decltype(data.value(0)), 1> datav(data.PixelCount(), data);

    concurrency::parallel_for_each(datav.extent, [=](concurrency::index<1> idx) restrict(amp)
    {
        datav[idx] = _Func(datav[idx]);
    });
}

template < typename _Dt1, typename _St1, typename _Fn1 >
void _Transform_AMP(_Dt1 &dst, const _St1 &src, _Fn1 &&_Func)
{
    if (dst.Width() != src.Width() || dst.Height() != src.Height())
    {
        DEBUG_FAIL("_Transform_AMP: Width() and Height() of dst and src must be the same.");
    }

    concurrency::array_view<decltype(dst.value(0)), 1> dstv(dst.PixelCount(), dst);
    concurrency::array_view<const decltype(src.value(0)), 1> srcv(src.PixelCount(), src);
    dstv.discard_data();

    concurrency::parallel_for_each(dstv.extent, [=](concurrency::index<1> idx) restrict(amp)
    {
        dstv[idx] = _Func(srcv[idx]);
    });
}

template < typename _Dt1, typename _St1, typename _St2, typename _Fn1 >
void _Transform_AMP(_Dt1 &dst, const _St1 &src1, const _St2 &src2, _Fn1 &&_Func)
{
    if (dst.Width() != src1.Width() || dst.Height() != src1.Height()
        || dst.Width() != src2.Width() || dst.Height() != src2.Height())
    {
        DEBUG_FAIL("_Transform_AMP: Width() and Height() of dst, src1 and src2 must be the same.");
    }

    concurrency::array_view<decltype(dst.value(0)), 1> dstv(dst.PixelCount(), dst);
    concurrency::array_view<const decltype(src1.value(0)), 1> src1v(src1.PixelCount(), src1);
    concurrency::array_view<const decltype(src2.value(0)), 1> src2v(src2.PixelCount(), src2);
    dstv.discard_data();

    concurrency::parallel_for_each(dstv.extent, [=](concurrency::index<1> idx) restrict(amp)
    {
        dstv[idx] = _Func(src1v[idx], src2v[idx]);
    });
}

template < typename _Dt1, typename _St1, typename _St2, typename _St3, typename _Fn1 >
void _Transform_AMP(_Dt1 &dst, const _St1 &src1, const _St2 &src2, const _St3 &src3, _Fn1 &&_Func)
{
    if (dst.Width() != src1.Width() || dst.Height() != src1.Height()
        || dst.Width() != src2.Width() || dst.Height() != src2.Height()
        || dst.Width() != src3.Width() || dst.Height() != src3.Height())
    {
        DEBUG_FAIL("_Transform_AMP: Width() and Height() of dst, src1, src2 and src3 must be the same.");
    }

    concurrency::array_view<decltype(dst.value(0)), 1> dstv(dst.PixelCount(), dst);
    concurrency::array_view<const decltype(src1.value(0)), 1> src1v(src1.PixelCount(), src1);
    concurrency::array_view<const decltype(src2.value(0)), 1> src2v(src2.PixelCount(), src2);
    concurrency::array_view<const decltype(src3.value(0)), 1> src3v(src3.PixelCount(), src3);
    dstv.discard_data();

    concurrency::parallel_for_each(dstv.extent, [=](concurrency::index<1> idx) restrict(amp)
    {
        dstv[idx] = _Func(src1v[idx], src2v[idx], src3v[idx]);
    });
}

template < typename _Dt1, typename _St1, typename _St2, typename _St3, typename _St4, typename _Fn1 >
void _Transform_AMP(_Dt1 &dst, const _St1 &src1, const _St2 &src2, const _St3 &src3, const _St4 &src4, _Fn1 &&_Func)
{
    if (dst.Width() != src1.Width() || dst.Height() != src1.Height()
        || dst.Width() != src2.Width() || dst.Height() != src2.Height()
        || dst.Width() != src3.Width() || dst.Height() != src3.Height()
        || dst.Width() != src4.Width() || dst.Height() != src4.Height())
    {
        DEBUG_FAIL("_Transform_AMP: Width() and Height() of dst, src1, src2, src3 and src4 must be the same.");
    }

    concurrency::array_view<decltype(dst.value(0)), 1> dstv(dst.PixelCount(), dst);
    concurrency::array_view<const decltype(src1.value(0)), 1> src1v(src1.PixelCount(), src1);
    concurrency::array_view<const decltype(src2.value(0)), 1> src2v(src2.PixelCount(), src2);
    concurrency::array_view<const decltype(src3.value(0)), 1> src3v(src3.PixelCount(), src3);
    concurrency::array_view<const decltype(src4.value(0)), 1> src4v(src4.PixelCount(), src4);
    dstv.discard_data();

    concurrency::parallel_for_each(dstv.extent, [=](concurrency::index<1> idx) restrict(amp)
    {
        dstv[idx] = _Func(src1v[idx], src2v[idx], src3v[idx], src4v[idx]);
    });
}
#endif


#endif
