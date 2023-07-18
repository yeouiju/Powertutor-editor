set terminal png
set output "spectrum-analyzer-wifi-11a-20MHz-2-0.png"
file = 'spectrum-analyzer-wifi-11a-20MHz-2-0'
unset surface
set key off
set term png
set output file . '.png'
set pm3d at s
set palette
set view 50,50
set xlabel "time (ms)"
set ylabel "freq (MHz)" offset 15,0,0
set zlabel "PSD (dBW/Hz)" offset 15,0,0
set ytics
set mytics 2
set ztics
set mztics 5
set grid ytics mytics ztics mztics
filename = file . '.tr'
stats filename using 3
refW = STATS_max
splot filename using ($1*1000.0):($2/1e6):(10*log10($3/refW))
