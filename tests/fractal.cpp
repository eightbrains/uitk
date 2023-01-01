//-----------------------------------------------------------------------------
// Copyright 2021 Eight Brains Studios, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "fractal.h"

using namespace uitk;

// Returns 1 if point is on the edge of the mandelbrot set, 0 otherwise
int calcMandelbrot(float coeffReal, float coeffImag)
{
    const int kMaxIterations = 25;

    float z_real = 0.0f;
    float z_imag = 0.0f;
    int nIt = 0;
    float mag2 = 0.0f;
    for (int k = 0;  k < kMaxIterations;  ++k) {
        mag2 = z_real * z_real + z_imag * z_imag;
        if (mag2 > 4.0f) {
            break;
        }
        float newZr = z_real * z_real - z_imag * z_imag + coeffReal;
        float newZi = 2.0f * z_imag * z_real + coeffImag;
        z_real = newZr;
        z_imag = newZi;
        nIt++;
    }
    if (nIt == kMaxIterations) {
      return 1;
    } else {
      return 0;
    }
}

float findFirstMandelbrotEdge(float coeffReal)
{
    float low = 0.0f;
    float high = 1.0f;
    float coeffImag = 0.5f;
    int val = calcMandelbrot(coeffReal, coeffImag);
    float lastOutside = high;
    for (int i = 0;  i < 5;  ++i) { // 5:  binary search converges quickly
        if (val == 1) {
            low = coeffImag;
        } else {
            high = coeffImag;
            lastOutside = high;
        }
        coeffImag = low + (high - low) / 2.0f;
        val = calcMandelbrot(coeffReal, coeffImag);
    }

    if (lastOutside < 1) {
        return lastOutside;
    } else {
        return coeffImag;
    }
}

Image calcFractalImage(const DrawContext& dc, uint32_t seed, int width, int height, float dpi,
                       FractalColor color /*= FractalColor::kColor*/)
{
    const float kJuliaMin = -1.6f;
    const float kJuliaMax = 1.6f;
    const int kJuliaMaxIterations = 100;

    // Find a point that is on the Mandelbrot set, those make more interesting
    // Julia sets. The Mandelbrot set is best where real is [-1.4, 0.5).
    float minCoeff = -1.4f;
    float maxCoeff = 0.5;
    float coeffReal = (float(seed & 0x0fffffff) / float(0x0fffffff));
    coeffReal = (maxCoeff - minCoeff) * coeffReal + minCoeff;
    float rand2 = float(seed & 0x0000ffff) / float(0xffff);
    float rand3 = float((seed & 0x00ffff00) >> 8) / float(0xffff);
    float coeffImag = findFirstMandelbrotEdge(coeffReal);
    coeffImag += float(rand2) * 0.205f - 0.05f;
    if (seed & 0x2) { coeffImag = -coeffImag; }
    float hueOffset = 360.0f * rand3;

    // Calculate the Julia set:  iterations will contain the number of iterations
    std::vector<float> iterations(width * height, 0);
    auto dx = (kJuliaMax - kJuliaMin) / float(width);
    auto dy = (kJuliaMax - kJuliaMin) / float(height);
    int pos = 0;
    int maxValue = 0;
    for (int j = 0;  j < height;  ++j) {
        for (int i = 0;  i < width;  ++i) {
            float z_real = float(i) * dx + kJuliaMin;
            float z_imag = float(j) * dy + kJuliaMin;
            int nIt = 0;
            float mag2 = 0.0f;
            for (int k = 0;  k < kJuliaMaxIterations;  ++k) {
                mag2 = z_real * z_real + z_imag * z_imag;
                if (mag2 > 4.0f) {
                    break;
                }
                float newZr = z_real * z_real - z_imag * z_imag + coeffReal;
                float newZi = 2.0f * z_imag * z_real + coeffImag;
                z_real = newZr;
                z_imag = newZi;
                nIt += 1;
            }
            iterations[pos++] = float(nIt);
            if (nIt > maxValue) {
                maxValue = nIt;
            }
        }
    }

    // Normalize the value
    for (size_t i = 0;  i < iterations.size();  ++i) {
        iterations[i] /= float(maxValue);
    }

    // Now we can create the image data
    Image img(width, height, kImageBGRX32, dpi);
    uint8_t *bgrx = img.data();
    for (int i = 0;  i < width * height;  ++i) {
        auto normalizedIterations = iterations[i];
        // This gives a more or less solid background
        if (normalizedIterations < 0.015f) { normalizedIterations = 0.015f; }

        // For saturation, we want the background desaturated, but want it to rapidly
        // become saturated, which happens much more rapidly with tanh than sqrt.
        float h = 360.0f * (1.0f - std::sqrt(normalizedIterations)) + hueOffset;
        if (h > 360.0f) {  h -= 360.0f; }
        float s = std::tanh(2.0f * 3.141592f * normalizedIterations);
        uint32_t pixel = HSVColor(h, s, 1.0f).toColor().toRGBA();
        bgrx[4 * i    ] = (pixel & 0x0000ff00) >>  8;  // blue
        bgrx[4 * i + 1] = (pixel & 0x00ff0000) >> 16;  // green
        bgrx[4 * i + 2] = (pixel & 0xff000000) >> 24;  // red
        bgrx[4 * i + 3] = 0xff;
    }
    if (color == FractalColor::kGrey) {
        for (int i = 0;  i < width * height;  ++i) {
            Color c = Color(bgrx[4 * i + 2], bgrx[4 * i + 1], bgrx[4 * i]).toGrey();
            bgrx[4 * i    ] = uint8_t(c.blue() * 255.0f);
            bgrx[4 * i + 1] = uint8_t(c.green() * 255.0f);
            bgrx[4 * i + 2] = uint8_t(c.red() * 255.0f);
        }
    }

    return img;
}
