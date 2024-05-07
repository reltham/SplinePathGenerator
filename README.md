# SplinePathGenerator
 Utility for creating paths (points and directions) along spline curves.
 
 I renamed this because I intend to change it to use Cattmull-Rom Splines and support more control points to get fancier paths all in one go.

![image](https://github.com/reltham/SplinePathGenerator/assets/3689101/9a5dbaa1-a0e9-4b55-a562-0e3d55b08d00)

 Paths consist of 5 values per entry.
|command|command_data|deltaX|deltaY|rotation|description|
|-------|------------|------|------|--------|-----------|
| 1 |repeat|X|Y|0-23|move entity by X,Y repreat times, with rotation 0-23|
| 2 |path index| 0 | 0 | 0 |gosub to another path|
| 0 | 1 | 0 | 0 | 0 |return from a gosub|
| 0 | 0 | 0 | 0 | 0 |end of path|
