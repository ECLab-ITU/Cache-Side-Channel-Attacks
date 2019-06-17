clear
reset
set key off
set border 3
set auto
 
set xrange[0:1000]
set xtics 100
 
# Make some suitable labels.
set title "Prime+Probe Threshold" font ",18"
set xlabel "Cycles" font ",18"
set ylabel "Samples" font ",18"
 
set terminal png enhanced font "arial 14" size 1200, 800
ft="png"
# Set the output-file name.
set output "threshold.png"
 
set style histogram clustered gap 1
set style fill solid border -1
 
binwidth=5
set boxwidth binwidth
bin(x,width)=width*floor(x/width) + binwidth/2.0
 
plot 'values.dat' using (bin($1,binwidth)):0 smooth freq with boxes lc "red", 'values.dat' using (bin($2,binwidth)):0 smooth freq with boxes lc "red"
