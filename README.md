# BezierPathGenerator
 Utility for creating paths (points and directions) along Bezier curves.

![image](https://github.com/reltham/BezierPathGenerator/assets/3689101/5bd06203-5b70-4658-bf53-0c4dfbe5d737)
 
 Paths consist of 5 values per entry.
|command|command_data|deltaX|deltaY|rotation|description|
|-------|------------|------|------|--------|-----------|
| 1 |repeat|X|Y|0-23|move entity by X,Y repreat times, with rotation 0-23|
| 2 |path index| 0 | 0 | 0 |gosub to another path|
| 0 | 1 | 0 | 0 | 0 |return from a gosub|
| 0 | 0 | 0 | 0 | 0 |end of path|
