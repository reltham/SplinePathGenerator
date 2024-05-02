
// code to generate path data from a given bezier curve

#define IMM2D_WIDTH 320
#define IMM2D_HEIGHT 240
#define IMM2D_SCALE 4
#define IMM2D_WINDOW_TITLE "Bezier Path Generator"

#define IMM2D_IMPLEMENTATION
#include "immediate2d.h"

#define _CRT_SECURE_NO_WARNINGS    
#include <corecrt_math.h>
#include <cstdio>

#include "DrawText.h"

struct Point
{
    double x, y;

    Point()
        : x(0.0), y(0.0)
    {}

    Point(const double NewX, const double NewY)
        : x(NewX), y(NewY)
    {}

    Point operator += (const Point& other)
    {
        x += other.x;
        y += other.y;
        return { x,y };
    }
};

double Length(const Point v)
{
    return sqrt(v.x * v.x + v.y * v.y);
}

Point Normalize(const Point v)
{
    Point Result;

    const double ThisLength = Length(v);

    Result.x = v.x / ThisLength;
    Result.y = v.y / ThisLength;

    return Result;
}

Point cubic_bezier(Point p0, Point p1, Point p2, Point p3, double t) {
    Point result;
    double u = 1 - t;
    double tt = t * t;
    double uu = u * u;
    double uuu = uu * u;
    double ttt = tt * t;

    result.x = uuu * p0.x; //beginning influence
    result.x += 3 * uu * t * p1.x; // first middle point influence
    result.x += 3 * u * tt * p2.x; // second middle point influence
    result.x += ttt * p3.x; // ending influence

    result.y = uuu * p0.y;
    result.y += 3 * uu * t * p1.y;
    result.y += 3 * u * tt * p2.y;
    result.y += ttt * p3.y;

    return result;
}

Point cubic_bezier_derivative(Point p0, Point p1, Point p2, Point p3, double t) {
    Point result;
    double u = 1 - t;
    double uu = u * u;
    double tt = t * t;

    result.x = 3 * uu * (p1.x - p0.x) +
        6 * u * t * (p2.x - p1.x) +
        3 * tt * (p3.x - p2.x);

    result.y = 3 * uu * (p1.y - p0.y) +
        6 * u * t * (p2.y - p1.y) +
        3 * tt * (p3.y - p2.y);

    return result;
}


void GeneratePath(const Point p1, const Point c1, const Point c2, const Point p2, const double StepT, const double MinDelta)
{
    Point LastPoint;
    Point LastUsedPoint;
    Point TotalDelta;

    for (double t = 0.0f; t <= 1.00001f; t += StepT)
    {
        const Point CurrentPoint = cubic_bezier(p1, c1, c2, p2, t);
        const Point PathTangent = cubic_bezier_derivative(p1, c1, c2, p2, t);

        double PathDirection = (atan2(PathTangent.y, PathTangent.x) * 3.82f); // DotProduct(Point(0.0f, 1.0f), PathTangent);
        if (PathDirection > 12.0f)
        {
            PathDirection -= 24.0f;
        }
        if (t == 0.0f)
        {
            LastPoint = CurrentPoint;
            LastUsedPoint = CurrentPoint;
        }
        const Point Delta = Point(CurrentPoint.x - LastPoint.x, CurrentPoint.y - LastPoint.y);

        TotalDelta += Delta;
        if (Length(TotalDelta) > MinDelta)
        {
            printf("    1, 1, %3d, %3d, %2d,    %3d, %3d, %3d %3d, %3d\n", static_cast<int>(TotalDelta.y * 50.0f), -static_cast<int>(TotalDelta.x * 50.0f), (static_cast<int>(PathDirection) + 12),
                static_cast<int>(CurrentPoint.x * 1.0f), static_cast<int>(CurrentPoint.y * 1.0f), static_cast<int>(PathTangent.x * 100.0f), static_cast<int>(PathTangent.y * 100.0f), static_cast<int>(PathDirection));
            TotalDelta = { 0.0f, 0.0f };

            DrawLine((int)(LastUsedPoint.y * 10.0f) + Width / 8, -(int)(LastUsedPoint.x * 10.0f) + Height - Height / 8, (int)(CurrentPoint.y * 10.0f) + Width / 8, -(int)(CurrentPoint.x * 10.0f) + Height - Height / 8, 1, White);
            //printf("%3d, %3d, %3d %3d\n",  static_cast<int>(CurrentPoint.x * 2.0f), static_cast<int>(CurrentPoint.y * 2.0f), static_cast<int>(LastPoint.x * 2.0f), static_cast<int>(LastPoint.y * 2.0f));
            
            LastUsedPoint = CurrentPoint;
        }

        LastPoint = CurrentPoint;
    }
}


char path0[] = {
    1, 30, 10,   0,  6,
    1,  1,  0,   0,  7,
    1,  1,  0,   0,  8,
    1,  1,  0,   0,  9,
    1,  1,  0,   0, 10,
    1,  1,  0,   0, 11,
    1, 30,  0,  10, 12,
    1,  1,  0,   0, 13,
    1,  1,  0,   0, 14,
    1,  1,  0,   0, 15,
    1,  1,  0,   0, 16,
    1,  1,  0,   0, 17,
    1, 30,-10,   0, 18,
    1,  1,  0,   0, 19,
    1,  1,  0,   0, 20,
    1,  1,  0,   0, 21,
    1,  1,  0,   0, 22,
    1,  1,  0,   0, 23,
    1, 30,  0, -10,  0,
    1,  1,  0,   0,  1,
    1,  1,  0,   0,  2,
    1,  1,  0,   0,  3,
    1,  1,  0,   0,  4,
    1,  1,  0,   0,  5,
    2,  0,  1,   0,  0,
    0,  0,  0,   0,  0 }; // 26

char path1[] = {
    1, 1,   0,   0,  7,
    1, 1,   0,   0,  8,
    2, 0,   2,   0,  0,
    2, 0,   3,   0,  0,
    2, 0,   4,   0,  0,
    1, 1,   0,   0,  8,
    1, 1,   0,   0,  7,
    0, 1,   0,   0,  0 }; // 6

char path2[] = {
    1, 3, 10,   0,  7,
    1, 3,  9,   0,  7,
    1, 3,  9,   0,  7,
    1, 3,  9,   1,  7,
    1, 3,  9,   1,  8,
    1, 3,  9,   2,  8,
    1, 3,  9,   3,  8,
    1, 3,  8,   4,  9,
    1, 3,  7,   5, 10,
    1, 3,  5,   6, 10,
    1, 2,  4,   6, 11,
    1, 2,  2,   7, 12,
    1, 2,  1,   7, 12,
    1, 2,  1,   7, 12,
    1, 2,  0,   7, 12,
    1, 2,  0,   7, 12,
    1, 2,  0,   7, 12,
    1, 2,  0,   7, 12,
    1, 2,  0,   7, 12,
    1, 2,  0,   7, 12,
    1, 2,  1,   7, 12,
    1, 2,  2,   7, 11,
    1, 2,  3,   7, 11,
    1, 3,  5,   6, 10,
    1, 3,  6,   5,  9,
    1, 3,  7,   4,  9,
    1, 3,  8,   3,  8,
    1, 3,  9,   2,  8,
    1, 3,  9,   2,  7,
    1, 3,  9,   1,  7,
    1, 3,  9,   1,  7,
    1, 3,  9,   0,  7,
    1, 3,  9,   0,  7,
    0, 1,  0,   0,  0 };  // 34

char path3[] = {
    1, 1, -25,   0, 17,
    1, 1, -24,   0, 17,
    1, 1, -24,   0, 17,
    1, 1, -24,   1, 17,
    1, 1, -24,   3, 17,
    1, 1, -24,   5, 17,
    1, 1, -23,   7, 16,
    1, 1, -22,  11, 15,
    1, 1, -18,  16, 14,
    1, 1, -14,  20, 13,
    1, 1,  -8,  23, 12,
    1, 1,  -3,  24, 12,
    1, 1,   1,  24, 12,
    1, 1,   6,  24, 11,
    1, 1,  11,  22, 10,
    1, 1,  15,  19,  9,
    1, 1,  19,  15,  9,
    1, 1,  22,  10,  8,
    1, 1,  24,   5,  7,
    1, 1,  24,   1,  6,
    1, 1,  24,  -3,  6,
    1, 1,  23,  -8,  5,
    1, 1,  21, -13,  4,
    1, 1,  17, -17,  3,
    1, 1,  13, -20,  2,
    1, 1,   8, -23,  1,
    1, 1,   4, -24,  1,
    1, 1,  -1, -24, 23,
    1, 1,  -6, -24, 22,
    1, 1, -11, -22, 21,
    1, 1, -16, -18, 20,
    1, 1, -20, -14, 19,
    1, 1, -22,  -9, 19,
    1, 1, -24,  -6, 18,
    1, 1, -24,  -4, 18,
    1, 1, -24,  -2, 18,
    1, 1, -24,  -1, 18,
    1, 1, -24,   0, 18,
    1, 1, -25,   0, 18,
    0, 1,   0,   0,  0 }; // 39

char path4[] = {
    1, 1,   0,  12, 12,
    1, 1,  -1,  12, 12,
    1, 1,  -1,  12, 12,
    1, 1,  -2,  12, 13,
    1, 1,  -3,  11, 13,
    1, 1,  -4,  11, 13,
    1, 1,  -5,  11, 13,
    1, 1,  -6,  10, 14,
    1, 1,  -7,  10, 14,
    1, 1,  -7,   9, 14,
    1, 1,  -8,   9, 14,
    1, 1,  -8,   9, 14,
    1, 1,  -8,   8, 14,
    1, 1,  -8,   8, 14,
    1, 1,  -8,   8, 14,
    1, 1,  -8,   8, 14,
    1, 1,  -8,   9, 14,
    1, 1,  -8,   9, 14,
    1, 1,  -7,   9, 14,
    1, 1,  -6,  10, 14,
    1, 1,  -6,  10, 13,
    1, 1,  -5,  11, 13,
    1, 1,  -4,  11, 13,
    1, 1,  -3,  11, 12,
    1, 1,  -2,  12, 12,
    1, 1,  -1,  12, 12,
    1, 1,   0,  12, 12,
    0, 1,   0,   0,  0 };

char path5[] = {
    1, 1,   4,  11, 22,
    1, 2,   3,  11, 22,
    1, 1,   3,  12, 22,
    1, 1,   3,  12, 23,
    1, 3,   2,  12, 23,
    1, 3,   1,  12, 23,
    1, 2,   0,  12, 23,
    1, 2,   0,  12,  1,
    1, 2,  -1,  12,  1,
    1, 2,  -2,  12,  1,
    1, 1,  -3,  12,  2,
    1, 1,  -3,  11,  2,
    1, 2,  -4,  11,  2,
    1, 2,  -5,  11,  2,
    1, 2,  -6,  10,  3,
    1, 1,  -7,  10,  3,
    1, 1,  -7,   9,  3,
    1, 1,  -8,   9,  3,
    1, 1,  -9,   8,  4,
    1, 1,  -9,   7,  4,
    1, 1, -10,   7,  4,
    1, 1, -10,   6,  5,
    1, 1, -11,   5,  5,
    1, 1, -11,   4,  5,
    1, 1, -12,   2,  6,
    1, 1, -12,   0,  7,
    1, 1, -12,  -1,  8,
    1, 1, -11,  -4,  8,
    1, 1,  -9,  -7,  9,
    1, 1,  -7,  -9, 10,
    1, 1,  -5, -11, 11,
    1, 1,  -3, -11, 12,
    1, 1,  -1, -12, 12,
    1, 2,   0, -12, 12,
    1, 2,   1, -12, 12,
    1, 1,   2, -12, 12,
    1, 2,   3, -12, 13,
    1, 1,   3, -11, 13,
    1, 3,   4, -11, 13,
    0, 0,   0,   0,  0 };

unsigned short num_paths = 6;
//char* paths[] = { &path2[0], &path3[0], &path1[0], &path4[0] };


void run()
{
#if 0
    FILE* output = fopen("PATHS.BIN", "wb");
    if (output != nullptr)
    {
        int bytesWritten = fwrite(&num_paths, 2, 1, output);

        unsigned short offset = 0xA800 + 2 + (num_paths * 2);
        bytesWritten += fwrite(&offset, 2, 1, output);
        offset += sizeof(path0);
        bytesWritten += fwrite(&offset, 2, 1, output);
        offset += sizeof(path1);
        bytesWritten += fwrite(&offset, 2, 1, output);
        offset += sizeof(path2);
        bytesWritten += fwrite(&offset, 2, 1, output);
        offset += sizeof(path3);
        bytesWritten += fwrite(&offset, 2, 1, output);
        offset += sizeof(path4);
        bytesWritten += fwrite(&offset, 2, 1, output);

        bytesWritten += fwrite(&path0, 1, sizeof(path0), output);
        bytesWritten += fwrite(&path1, 1, sizeof(path1), output);
        bytesWritten += fwrite(&path2, 1, sizeof(path2), output);
        bytesWritten += fwrite(&path3, 1, sizeof(path3), output);
        bytesWritten += fwrite(&path4, 1, sizeof(path4), output);
        bytesWritten += fwrite(&path5, 1, sizeof(path5), output);

        int result = fclose(output);
        printf("Bytes Written: %d  fclose result: %d \n", bytesWritten, result);
    }
#else

    // const Point p1 = Point(0.0, 1.0);
    // const Point c1 = Point(3.0, 1.0);
    // const Point c2 = Point(3.0, 4.0);
    // const Point p2 = Point(6.0, 4.0);
    //const Point p1 = Point(6.0, 3.0);
    //const Point c1 = Point(2.0, 5.0);
    //const Point c2 = Point(-1.0, 1.0);
    //const Point p2 = Point(4.0, 2.0);
    const Point p1 = Point(10.0, 6.0);
    const Point c1 = Point(2.0, 9.0);
    const Point c2 = Point(-1.0, 0.0);
    const Point p2 = Point(6.0, 3.0);
    GeneratePath(p1, c1, c2, p2, 0.00001, 0.25);

    // Draw our axes
    DrawLine(0, Height - Height / 8, Width, Height - Height / 8, 1, Yellow);
    DrawLine(Width / 8, 0, Width / 8, Height, 1, Yellow);
    DrawString(Width / 8 + 10,1, "Bezier Path Generator", LightBlue);
#endif

}

