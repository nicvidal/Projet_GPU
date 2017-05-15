#!/usr/bin/gnuplot

set term pdf #enhanced monochrome dashed
set output "exemple.pdf"

set key right bottom

set xlabel "N"
set ylabel "Performance (#items/s)"

plot "exemple1.data" with linespoints title "Exemple 1",\
     "exemple2.data" with linespoints title "Exemple 2"
