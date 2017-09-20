#!/bin/bash

CMD="./bin/clang-linux-6.0.0/release/comparable-distance-test"
count=0
for i in `seq -20 30`; do
    let x=0
    let y=i*2
    echo $CMD data/polygon.json $x $y
    $CMD data/polygon.json $x $y
    svg2png point_to_geometry_distance.svg
    printf -v frame "%03d" $count
    convert -flatten -background white point_to_geometry_distance.png output/point_to_geometry_distance-$frame.png
    let count+=1
done

convert -delay 25 -loop 0 output/point_to_geometry_distance*.png point_to_geometry_distance.gif
rm output/*
