clear
reset
set key off
set border 3
set auto

set xrange[0:300]
set xtics 10
 
# Make some suitable labels.
set title "Flush+Flush Threshold" font ",18"
set xlabel "Cycles" font ",18"
set ylabel "Samples" font ",18"


set terminal png enhanced font "arial 14" size 1200, 800
ft="png"
# Set the output-file name.
set output "threshold.png"
 
set style histogram clustered gap 1
set style fill solid border -1
 
binwidth=0.8
set boxwidth binwidth
bin(x,width)=width*floor(x/width) + binwidth/2.0


plot 'values.dat' using 0:1 smooth freq with boxes lc "red", 'values.dat' using 0:2 smooth freq with boxes lc "green"
