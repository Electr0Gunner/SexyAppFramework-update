#include "gpuimage.hpp"

using namespace PopLib;

void GPUImage::FillScanLinesWithCoverage(Span*, int, const Color&, int,
                                         const BYTE*, int, int, int, int) {}

void GPUImage::Create(int theWidth, int theHeight) {}

bool GPUImage::PolyFill3D(const Point[], int, const Rect*,
                          const Color&, int, int, int) {
	return false;
}

void GPUImage::FillRect(const Rect&, const Color&, int) {}

void GPUImage::DrawLine(double, double, double, double,
                        const Color&, int) {}

void GPUImage::DrawLineAA(double, double, double, double,
                          const Color&, int) {}

void GPUImage::Blt(Image*, int, int, const Rect&, const Color&, int) {}

void GPUImage::BltF(Image*, float, float, const Rect&, const Rect&,
                    const Color&, int) {}

void GPUImage::BltRotated(Image*, float, float, const Rect&, const Rect&,
                          const Color&, int, double, float, float) {}

void GPUImage::StretchBlt(Image*, const Rect&, const Rect&, const Rect&,
                          const Color&, int, bool) {}

void GPUImage::BltMatrix(Image*, float, float, const Matrix3&, const Rect&,
                         const Color&, int, const Rect&, bool) {}

void GPUImage::BltTrianglesTex(Image*, const TriVertex[][3], int,
                               const Rect&, const Color&, int, float, float,
                               bool) {}

void GPUImage::BltMirror(Image*, int, int, const Rect&, const Color&, int) {}

void GPUImage::StretchBltMirror(Image*, const Rect&, const Rect&, const Rect&,
                                const Color&, int, bool) {}

void GPUImage::PurgeBits() {}