#!/bin/sh

rm -f software/interface/log.txt $dir

cd software/vision && ./init_webcam.sh
cd ../..

cd software/interface && ./aquatux aquatux_submarine.csv | tee aquatux.txt
cd ../..

dir=tests/`date +%b_%d,%H:%M`
mkdir -p $dir
cp  software/interface/aquatux.txt $dir
cp  software/interface/log.txt $dir
cp  software/interface/sub.avi $dir
cp  software/interface/sub_dwn.avi $dir
